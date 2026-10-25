// SPDX-License-Identifier: MIT
#include "../src/backend.h"
#include <QDesktopServices>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

class BackendTest : public QObject {
  Q_OBJECT
  QList<QUrl> openedUrls;
private slots:
  void captureUrl(const QUrl &url) { openedUrls << url; }
  void init() {
    openedUrls.clear();
    QDesktopServices::setUrlHandler("file", this, "captureUrl");
  }
  void cleanup() { QDesktopServices::unsetUrlHandler("file"); }
private slots:
  void lifecycleAndTransfers() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto logPath = dir.filePath("calls.jsonl");
    qputenv("OWLY_TEST_LOG", logPath.toUtf8());
    qputenv("OWLY_TEST_INCOMING", "1");
    Backend backend;
    QSignalSpy incoming(&backend, &Backend::incoming);
    backend.configure(FIXTURE_PATH, dir.path(), "Equipo de prueba", 1);
    QTRY_VERIFY(backend.state()["available"].toBool());
    backend.start();
    QTRY_VERIFY(backend.state()["active"].toBool());
    QTRY_VERIFY(backend.state()["receiving"].toBool());
    QTRY_VERIFY(!backend.state()["scanning"].toBool());
    QCOMPARE(backend.state()["devices"].toList().size(), 2);
    QTRY_COMPARE(incoming.count(), 1);
    QCOMPARE(backend.state()["requests"].toList().size(), 1);
    backend.answer("7", true);
    QTRY_COMPARE(backend.state()["history"].toList().size(), 1);
    QVERIFY(backend.state()["requests"].toList().isEmpty());
    const auto received =
        backend.state()["history"].toList().first().toMap()["files"].toList();
    QCOMPARE(received.size(), 1);
    QCOMPARE(received.first().toMap()["url"].toUrl(),
             QUrl::fromLocalFile("/tmp/foto de vacaciones.jpg"));

    // Leading dashes, spaces and shell metacharacters stay one argument.
    const auto filePath = dir.filePath("--foto $(touch nope);.jpg");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("test");
    file.close();
    const auto fileUrl = QUrl::fromLocalFile(filePath);
    const auto secondPath = dir.filePath("otra foto #2.png");
    QVERIFY(QFile::copy(filePath, secondPath));
    const auto secondUrl = QUrl::fromLocalFile(secondPath);
    backend.sendFiles("aa:bb:cc:dd:ee:ff", {fileUrl, secondUrl});
    QTRY_COMPARE(backend.state()["history"].toList().size(), 2);
    QTRY_VERIFY(!backend.state()["sending"].toBool());
    QVERIFY(backend.state()["error"].toString().isEmpty());
    const auto sent =
        backend.state()["history"].toList().first().toMap()["files"].toList();
    QCOMPARE(sent.size(), 2);
    QCOMPARE(sent.first().toMap()["url"].toUrl(), fileUrl);
    QCOMPARE(sent.last().toMap()["url"].toUrl(), secondUrl);
    QVERIFY(openedUrls.isEmpty());
    backend.openHistoryFile(fileUrl);
    backend.openHistoryFile(secondUrl, true);
    QCOMPARE(openedUrls,
             (QList<QUrl>{fileUrl, QUrl::fromLocalFile(dir.path())}));
    backend.openHistoryFile(QUrl("https://example.invalid"));
    backend.openHistoryFile(QUrl::fromLocalFile("/etc/passwd"));
    QCOMPARE(openedUrls.size(), 2);
    QVERIFY(QFile::remove(filePath));
    backend.openHistoryFile(fileUrl);
    QCOMPARE(openedUrls.size(), 2);
    QVERIFY(backend.state()["error"].toString().contains("ubicación original"));
    QFile log(logPath);
    QVERIFY(log.open(QIODevice::ReadOnly));
    const auto calls = log.readAll();
    QVERIFY(calls.contains("--to"));
    QVERIFY(calls.contains("iPhone de prueba"));
    QVERIFY(!calls.contains("--mac"));
    // A cached discovery MAC is only a row ID, never a pinned endpoint.
    QVERIFY(!calls.contains("aa:bb:cc:dd:ee:ff"));
    QVERIFY(calls.contains("$(touch nope)"));
    QVERIFY(calls.contains("\"--\""));

    const auto nextDestination = dir.filePath("new-downloads");
    backend.configure(FIXTURE_PATH, nextDestination, "Nombre nuevo", 5);
    QCOMPARE(backend.state()["destination"].toString(), dir.path());
    backend.stop();
    QTRY_VERIFY(!backend.state()["active"].toBool());
    QTRY_VERIFY(!backend.state()["receiving"].toBool());
    QVERIFY(backend.state()["requests"].toList().isEmpty());
    QVERIFY(backend.state()["devices"].toList().isEmpty());
    QTRY_COMPARE(backend.state()["destination"].toString(), nextDestination);
    QVERIFY(log.seek(0));
    QVERIFY(log.readAll().contains("released"));
    qunsetenv("OWLY_TEST_INCOMING");
    qunsetenv("OWLY_TEST_LOG");
  }

  void cancellationAndInvalidFiles() {
    QTemporaryDir dir;
    qputenv("OWLY_TEST_SLOW", "1");
    Backend backend;
    backend.configure(FIXTURE_PATH, dir.path(), "", 10);
    QTRY_VERIFY(backend.state()["available"].toBool());
    backend.start();
    QTRY_VERIFY(backend.state()["active"].toBool());
    QTRY_VERIFY(!backend.state()["devices"].toList().isEmpty());
    backend.sendFiles("target", {QUrl("https://example.com/test.jpg")});
    QVERIFY(!backend.state()["sending"].toBool());
    QVERIFY(!backend.state()["error"].toString().isEmpty());
    QFile file(dir.filePath("photo.jpg"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("test");
    file.close();
    backend.sendFiles("aa:bb:cc:dd:ee:ff",
                      {QUrl::fromLocalFile(file.fileName())});
    QVERIFY(backend.state()["sending"].toBool());
    backend.cancelSend(); // Also exercises cancellation before QProcess::start.
    QTRY_VERIFY(!backend.state()["sending"].toBool());
    QCOMPARE(backend.state()["history"].toList().size(), 1);
    QCOMPARE(
        backend.state()["history"].toList().first().toMap()["icon"].toString(),
        QString("dialog-error"));
    backend.stop();
    QTRY_VERIFY(!backend.state()["active"].toBool());
    qunsetenv("OWLY_TEST_SLOW");
  }

  void missingExecutable() {
    Backend backend;
    backend.configure("/does/not/exist/owly", "/tmp", "", 10);
    QVERIFY(!backend.state()["available"].toBool());
    QVERIFY(!backend.state()["error"].toString().isEmpty());
    backend.start();
    QVERIFY(!backend.state()["starting"].toBool());
  }

  void failedSendRemainsInActivity() {
    QTemporaryDir dir;
    qputenv("OWLY_TEST_SEND_FAIL", "1");
    Backend backend;
    backend.configure(FIXTURE_PATH, dir.path(), "", 10);
    QTRY_VERIFY(backend.state()["available"].toBool());
    backend.start();
    QTRY_VERIFY(!backend.state()["devices"].toList().isEmpty());
    QFile file(dir.filePath("photo.jpg"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("test");
    file.close();
    backend.sendFiles("aa:bb:cc:dd:ee:ff",
                      {QUrl::fromLocalFile(file.fileName())});
    QTRY_VERIFY(!backend.state()["sending"].toBool());
    QVERIFY(
        backend.state()["error"].toString().contains("no AWDL peer answered"));
    const auto history = backend.state()["history"].toList();
    QCOMPARE(history.size(), 1);
    QVERIFY(history.first().toMap()["files"].toList().isEmpty());
    QVERIFY(history.first().toMap()["detail"].toString().contains(
        "no AWDL peer answered"));
    backend.dismissError();
    QVERIFY(backend.state()["error"].toString().isEmpty());
    QCOMPARE(backend.state()["history"].toList(), history);
    backend.stop();
    QTRY_VERIFY(!backend.state()["active"].toBool());
    qunsetenv("OWLY_TEST_SEND_FAIL");
  }
};

QTEST_MAIN(BackendTest)
#include "backend_test.moc"
