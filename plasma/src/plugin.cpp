// SPDX-License-Identifier: MIT
#include "backend.h"
#include "filethumbnail.h"
#include <QQmlExtensionPlugin>
#include <qqml.h>

class OwlyPlugin : public QQmlExtensionPlugin {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
  void registerTypes(const char *uri) override {
    qmlRegisterType<Backend>(uri, 1, 0, "Backend");
    qmlRegisterType<FileThumbnail>(uri, 1, 0, "FileThumbnail");
  }
};

#include "plugin.moc"
