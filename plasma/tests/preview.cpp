// SPDX-License-Identifier: MIT
// A real Qt Quick render of the shipped view, with an explicit fixture mode.
#include "../src/backend.h"
#include <KLocalizedQmlContext>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QPalette>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QTimer>

int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  app.setApplicationName("Owly Preview");
  const auto args = app.arguments();
  QIcon::setThemeName(args.contains("--dark") ? "breeze-dark" : "breeze");
  if (args.contains("--dark")) {
    QPalette palette = app.palette();
    palette.setColor(QPalette::Window, QColor("#232629"));
    palette.setColor(QPalette::WindowText, QColor("#eff0f1"));
    palette.setColor(QPalette::Base, QColor("#1b1e20"));
    palette.setColor(QPalette::Text, QColor("#eff0f1"));
    palette.setColor(QPalette::Button, QColor("#31363b"));
    palette.setColor(QPalette::ButtonText, QColor("#eff0f1"));
    palette.setColor(QPalette::Highlight, QColor("#3daee9"));
    palette.setColor(QPalette::HighlightedText, QColor("#eff0f1"));
    app.setPalette(palette);
  }
  Backend backend;
  QQuickView window;
  window.setTitle("Owly · AirDrop — Vista previa");
  window.setResizeMode(QQuickView::SizeRootObjectToView);
  KLocalization::setupLocalizedContext(window.engine());
  window.setInitialProperties({{"backend", QVariant::fromValue(&backend)}});
  window.setSource(QUrl::fromLocalFile(VIEW_PATH));
  if (window.status() == QQuickView::Error)
    return 1;
  window.setColor(app.palette().color(QPalette::Window));
  window.resize(args.contains("--small") ? QSize(360, 470) : QSize(432, 650));
  window.show();
  bool autoStarted = false;
  if (args.contains("--fixture")) {
    backend.configure(FIXTURE_PATH, "/tmp/owly-preview", "Mi equipo", 10);
    QObject::connect(&backend, &Backend::changed, &app, [&] {
      if (!autoStarted && backend.state()["available"].toBool() &&
          !backend.state()["active"].toBool() &&
          !backend.state()["starting"].toBool() &&
          !backend.state()["stopping"].toBool()) {
        autoStarted = true;
        backend.start();
      }
    });
  } else {
    backend.configure("", "", "", 10);
  }
  const int capture = args.indexOf("--capture");
  if (capture > 0 && capture + 1 < args.size()) {
    QTimer::singleShot(3500, &app, [&] {
      const bool saved = window.grabWindow().save(args[capture + 1]);
      app.exit(saved ? 0 : 2);
    });
  }
  return app.exec();
}
