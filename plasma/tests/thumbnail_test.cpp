// SPDX-License-Identifier: MIT
#include "../src/filethumbnail.h"
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QTest>

class TestThumbnail : public FileThumbnail {
public:
  void complete() { componentComplete(); }
};

class ThumbnailTest : public QObject {
  Q_OBJECT
private slots:
  void localPreviewAndFallback() {
    QTemporaryDir dir;
    const auto path = dir.filePath("foto con espacios #1.png");
    QImage image(160, 80, QImage::Format_ARGB32);
    image.fill(Qt::green);
    QVERIFY(image.save(path));
    TestThumbnail thumbnail;
    thumbnail.setSize(QSizeF(48, 48));
    thumbnail.setSource(QUrl::fromLocalFile(path));
    thumbnail.complete();
    QTRY_VERIFY_WITH_TIMEOUT(thumbnail.ready(), 15000);
    QImage rendered(48, 48, QImage::Format_ARGB32);
    rendered.fill(Qt::transparent);
    QPainter painter(&rendered);
    thumbnail.paint(&painter);
    painter.end();
    QCOMPARE(rendered.pixelColor(24, 24), QColor(Qt::green));
    QCOMPARE(rendered.pixelColor(24, 0).alpha(), 0); // Aspect ratio preserved.
    thumbnail.setSource(QUrl::fromLocalFile(dir.filePath("missing.pdf")));
    QVERIFY(!thumbnail.ready());
    thumbnail.setSource(QUrl("https://example.invalid/image.png"));
    QVERIFY(!thumbnail.ready());
    // Replacing a pending source must cancel the old job without stale content.
    thumbnail.setSource(QUrl::fromLocalFile(path));
    thumbnail.setSource({});
    QTest::qWait(300);
    QVERIFY(!thumbnail.ready());
  }
};
QTEST_MAIN(ThumbnailTest)
#include "thumbnail_test.moc"
