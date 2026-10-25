// SPDX-License-Identifier: MIT
#include "../src/backend.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

class BackendTest : public QObject {
  Q_OBJECT
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

    // Leading dashes, spaces and shell metacharacters stay one argument.
    const auto filePath = dir.filePath("--foto $(touch nope);.jpg");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("test");
    file.close();
    backend.sendFiles("aa:bb:cc:dd:ee:ff", {QUrl::fromLocalFile(filePath)});
    QTRY_COMPARE(backend.state()["history"].toList().size(), 2);
    QTRY_VERIFY(!backend.state()["sending"].toBool());
    QVERIFY(backend.state()["error"].toString().isEmpty());
    QFile log(logPath);
    QVERIFY(log.open(QIODevice::ReadOnly));
    const auto calls = log.readAll();
    QVERIFY(calls.contains("--mac"));
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
    backend.sendFiles("target", {QUrl("https://example.com/test.jpg")});
    QVERIFY(!backend.state()["sending"].toBool());
    QVERIFY(!backend.state()["error"].toString().isEmpty());
    QFile file(dir.filePath("photo.jpg"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("test");
    file.close();
    backend.sendFiles("target", {QUrl::fromLocalFile(file.fileName())});
    QVERIFY(backend.state()["sending"].toBool());
    backend.cancelSend(); // Also exercises cancellation before QProcess::start.
    QTRY_VERIFY(!backend.state()["sending"].toBool());
    QVERIFY(backend.state()["history"].toList().isEmpty());
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
};

QTEST_MAIN(BackendTest)
#include "backend_test.moc"
