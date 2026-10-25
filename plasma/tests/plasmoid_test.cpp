// SPDX-License-Identifier: MIT
// Load the whole package through Plasma, including its native QML plugin.
#include <KPackage/Package>
#include <KPackage/PackageLoader>
#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/PluginLoader>
#include <PlasmaQuick/AppletQuickItem>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPainter>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTimer>
#include <cstdio>

class TestCorona : public Plasma::Corona {
public:
  QRect screenGeometry(uint) const override { return QRect(0, 0, 1024, 768); }
};

static int countReadyThumbnails(QQuickItem *item) {
  if (!item)
    return 0;
  int count = item->objectName() == "historyThumbnail" &&
                      item->property("ready").toBool()
                  ? 1
                  : 0;
  for (auto *child : item->childItems())
    count += countReadyThumbnails(child);
  return count;
}

int main(int argc, char **argv) {
  QTemporaryDir config;
  qputenv("XDG_CONFIG_HOME", config.path().toUtf8());
  QGuiApplication app(argc, argv);
  app.setApplicationName("Owly Plasma Test");
  const auto arguments = app.arguments();
  QTemporaryDir transfers;
  const auto photoPath = transfers.filePath("Foto de vacaciones.png");
  const auto documentPath = transfers.filePath("Notas del viaje.bin");
  if (arguments.contains("--history")) {
    QImage photo(256, 160, QImage::Format_RGB32);
    QPainter painter(&photo);
    QLinearGradient sky(0, 0, 0, 160);
    sky.setColorAt(0, QColor("#5bb9e1"));
    sky.setColorAt(1, QColor("#e8d7ab"));
    painter.fillRect(photo.rect(), sky);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#fff4ce"));
    painter.drawEllipse(QPoint(195, 40), 18, 18);
    painter.setBrush(QColor("#34796e"));
    painter.drawPolygon(QPolygon{{0, 160}, {80, 60}, {160, 160}});
    painter.setBrush(QColor("#21575f"));
    painter.drawPolygon(QPolygon{{80, 160}, {175, 80}, {256, 160}});
    painter.end();
    photo.save(photoPath);
    QFile document(documentPath);
    if (document.open(QIODevice::WriteOnly))
      document.write("Owly preview fixture");
    qputenv("OWLY_TEST_INCOMING", "1");
    qputenv(
        "OWLY_TEST_RECEIVED",
        QJsonDocument(QJsonArray{photoPath}).toJson(QJsonDocument::Compact));
  }
  const bool dark = arguments.contains("--dark");
  QFile themeConfig(config.filePath("plasmarc"));
  if (themeConfig.open(QIODevice::WriteOnly))
    themeConfig.write(dark ? "[Theme]\nname=breeze-dark\n"
                           : "[Theme]\nname=breeze-light\n");
  QFile iconConfig(config.filePath("kdeglobals"));
  if (iconConfig.open(QIODevice::WriteOnly))
    iconConfig.write(dark ? "[Icons]\nTheme=breeze-dark\n"
                          : "[Icons]\nTheme=breeze\n");
  themeConfig.close();
  iconConfig.close();
  TestCorona corona;
  corona.setKPackage(KPackage::PackageLoader::self()->loadPackage(
      "Plasma/Shell", "org.kde.plasma.desktop"));
  auto *containment = corona.createContainment("org.kde.desktopcontainment");
  if (!containment)
    return 1;
  if (arguments.contains("--compact")) {
    containment->setFormFactor(Plasma::Types::Horizontal);
    containment->setLocation(Plasma::Types::BottomEdge);
  }
  auto *applet = containment->createApplet(PACKAGE_PATH);
  if (!applet)
    return 1;
  applet->config().writeEntry("executable", QString(FIXTURE_PATH));
  auto *item = PlasmaQuick::AppletQuickItem::itemForApplet(applet);
  if (!item)
    return 2;
  QQuickWindow window;
  window.setColor(dark ? QColor("#232629") : QColor("#eff0f1"));
  const QSize size = arguments.contains("--compact") ? QSize(48, 48)
                     : arguments.contains("--small") ? QSize(360, 470)
                                                     : QSize(432, 650);
  window.resize(size);
  item->setParentItem(window.contentItem());
  item->setSize(size);
  item->setPreferredRepresentation(arguments.contains("--compact")
                                       ? item->compactRepresentation()
                                       : item->fullRepresentation());
  window.show();
  QTimer::singleShot(700, &app, [&] {
    auto *backend = item->property("backend").value<QObject *>();
    if (backend) {
      QMetaObject::invokeMethod(
          backend, "configure", Q_ARG(QString, QString(FIXTURE_PATH)),
          Q_ARG(QString, QString("/tmp/owly-preview")),
          Q_ARG(QString, QString("Mi equipo")), Q_ARG(int, 10));
      QTimer::singleShot(500, backend, [backend] {
        QMetaObject::invokeMethod(backend, "start");
      });
    }
  });
  QElapsedTimer elapsed;
  elapsed.start();
  QTimer verification;
  verification.setInterval(100);
  QObject::connect(&verification, &QTimer::timeout, &app, [&] {
    if (elapsed.elapsed() < 3500)
      return;
    auto *backend = item->property("backend").value<QObject *>();
    const auto state =
        backend ? backend->property("state").toMap() : QVariantMap{};
    const bool activityLoaded =
        !arguments.contains("--history") ||
        (state["history"].toList().size() == 2 &&
         countReadyThumbnails(item->fullRepresentationItem()) == 2);
    const bool loaded =
        activityLoaded &&
        (arguments.contains("--compact") ? item->compactRepresentationItem()
                                         : item->fullRepresentationItem()) &&
        state["active"].toBool() && state["devices"].toList().size() == 2;
    if (!loaded && elapsed.elapsed() < 15000)
      return;
    std::fprintf(stderr, "Native plasmoid loaded: %s\n", loaded ? "yes" : "no");
    const auto args = app.arguments();
    const int capture = args.indexOf("--capture");
    if (capture > 0 && capture + 1 < args.size())
      window.grabWindow().save(args[capture + 1]);
    app.exit(loaded ? 0 : 3);
  });
  verification.start();
  if (arguments.contains("--history")) {
    QTimer::singleShot(2200, &app, [&] {
      auto *backend = item->property("backend").value<QObject *>();
      if (backend) {
        QMetaObject::invokeMethod(
            backend, "answer", Q_ARG(QString, QString("7")), Q_ARG(bool, true));
        const QVariantList files{QUrl::fromLocalFile(photoPath),
                                 QUrl::fromLocalFile(documentPath)};
        QMetaObject::invokeMethod(backend, "sendFiles",
                                  Q_ARG(QString, QString("aa:bb:cc:dd:ee:ff")),
                                  Q_ARG(QVariantList, files));
      }
      if (item->fullRepresentationItem())
        item->fullRepresentationItem()->setProperty("page", 1);
    });
  }
  return app.exec();
}
