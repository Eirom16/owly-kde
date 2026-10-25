// SPDX-License-Identifier: MIT
// Load the whole package through Plasma, including its native QML plugin.
#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/PluginLoader>
#include <PlasmaQuick/AppletQuickItem>
#include <KPackage/PackageLoader>
#include <KPackage/Package>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTimer>
#include <cstdio>

class TestCorona : public Plasma::Corona {
public:
  QRect screenGeometry(uint) const override { return QRect(0, 0, 1024, 768); }
};

int main(int argc, char **argv) {
  QTemporaryDir config;
  qputenv("XDG_CONFIG_HOME", config.path().toUtf8());
  QGuiApplication app(argc, argv);
  app.setApplicationName("Owly Plasma Test");
  const auto arguments = app.arguments();
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
  corona.setKPackage(KPackage::PackageLoader::self()->loadPackage("Plasma/Shell", "org.kde.plasma.desktop"));
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
  QTimer::singleShot(3500, &app, [&] {
    auto *backend = item->property("backend").value<QObject *>();
    const auto state =
        backend ? backend->property("state").toMap() : QVariantMap{};
    const bool loaded = (arguments.contains("--compact") ? item->compactRepresentationItem() : item->fullRepresentationItem()) &&
                        state["active"].toBool() &&
                        state["devices"].toList().size() == 2;
    std::fprintf(stderr, "Native plasmoid loaded: %s\n", loaded ? "yes" : "no");
    const auto args = app.arguments();
    const int capture = args.indexOf("--capture");
    if (capture > 0 && capture + 1 < args.size())
      window.grabWindow().save(args[capture + 1]);
    app.exit(loaded ? 0 : 3);
  });
  return app.exec();
}
