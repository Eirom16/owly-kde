// SPDX-License-Identifier: MIT
#include "backend.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

Backend::Backend(QObject *parent) : QObject(parent) {
  m_state = {{"active", false},
             {"starting", false},
             {"stopping", false},
             {"receiving", false},
             {"scanning", false},
             {"sending", false},
             {"checking", false},
             {"available", false},
             {"progress", -1.0},
             {"devices", QVariantList{}},
             {"requests", QVariantList{}},
             {"history", QVariantList{}},
             {"error", QString{}},
             {"notice", QString{}},
             {"report", QVariantMap{}},
             {"remaining", 0},
             {"destination", QString{}}};
  auto *clock = new QTimer(this);
  clock->setInterval(1000);
  connect(clock, &QTimer::timeout, this, [this] {
    if (m_state["active"].toBool()) {
      set("remaining", qMax(0, m_state["remaining"].toInt() - 1));
      if (m_state["remaining"].toInt() == 0)
        stop();
    }
  });
  clock->start();
}

Backend::~Backend() {
  m_destroying = true;
  // The radio's attached D-Bus lease is also released if its process
  // dies. Never leave a detached lease behind when the widget is removed.
  for (auto *process : findChildren<QProcess *>()) {
    process->disconnect(this);
    if (process->state() != QProcess::NotRunning) {
      process->terminate();
      if (!process->waitForFinished(1000)) {
        process->kill();
        process->waitForFinished(1000);
      }
    }
  }
}

void Backend::set(const QString &key, const QVariant &value) {
  if (m_state.value(key) != value) {
    m_state[key] = value;
    emit changed();
  }
}

void Backend::configure(const QString &executable, const QString &folder,
                        const QString &name, int minutes) {
  // Changing configuration never replaces a process with an active lease.
  if (m_radio || m_sender || m_receiver || m_browser || m_doctor) {
    m_pendingConfig = {executable, folder, name, minutes};
    return;
  }
  m_pendingConfig.clear();
  m_executable = executable.trimmed();
  if (m_executable.isEmpty()) {
    const QString local = QDir::homePath() + "/.local/bin/owly";
    m_executable = QFileInfo(local).isExecutable()
                       ? local
                       : QStandardPaths::findExecutable("owly");
  }
  m_folder =
      folder.isEmpty()
          ? QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
          : folder;
  m_name = name.trimmed();
  m_minutes = qBound(1, minutes, 60);
  set("destination", m_folder);
  diagnose();
}

void Backend::diagnose() {
  if (m_doctor || m_state["active"].toBool() || m_radio)
    return;
  dismissError();
  if (m_executable.isEmpty() || !QFileInfo(m_executable).isExecutable()) {
    set("available", false);
    fail(tr("No se encuentra el ejecutable de Owly. Instala la CLI de este "
            "proyecto o indica su ruta en la configuración."));
    return;
  }
  set("checking", true);
  m_doctor = launch("doctor", {"doctor", "--json"}, 15000);
}

QProcess *Backend::launch(const QString &role, const QStringList &args,
                          int timeout) {
  auto *process = new QProcess(this);
  process->setProgram(m_executable);
  process->setArguments(
      args); // No shell: file names and peer names are always arguments.
  process->setProperty("role", role);
  connect(process, &QProcess::readyReadStandardError, this, [process] {
    auto bytes = process->property("stderr").toByteArray();
    bytes += process->readAllStandardError();
    process->setProperty("stderr", bytes.right(4096));
  });
  connect(process, &QProcess::readyReadStandardOutput, this,
          [this, process, role] {
            auto bytes = process->property("stdout").toByteArray();
            bytes += process->readAllStandardOutput();
            if (role != "doctor") {
              int newline;
              while ((newline = bytes.indexOf('\n')) >= 0) {
                const auto object =
                    QJsonDocument::fromJson(bytes.left(newline)).object();
                bytes.remove(0, newline + 1);
                if (!object.isEmpty())
                  event(role, object);
              }
            }
            // A broken/old CLI must not grow Plasma's memory indefinitely.
            process->setProperty("stdout", bytes.right(262144));
          });
  connect(process, &QProcess::finished, this,
          [this, process, role](int code, QProcess::ExitStatus status) {
            finish(process, role, status == QProcess::NormalExit ? code : -1);
          });
  connect(process, &QProcess::errorOccurred, this,
          [this, process, role](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
              process->setProperty("stderr", process->errorString().toUtf8());
              finish(process, role, -1);
            }
          });
  if (timeout > 0) {
    QTimer::singleShot(timeout, process, [this, process] {
      if (process->state() != QProcess::NotRunning) {
        process->setProperty(
            "stderr",
            tr("La operación tardó demasiado. Puedes volver a intentarlo.")
                .toUtf8());
        process->kill();
      }
    });
  }
  // Defer so the caller has assigned its process pointer before signals.
  QTimer::singleShot(0, process, [process] {
    if (!process->property("finished").toBool())
      process->start();
  });
  return process;
}

QStringList Backend::identityArgs() const {
  QStringList args{"--json", "--iface", m_interface};
  if (!m_name.isEmpty())
    args << "--name" << m_name;
  return args;
}

void Backend::start() {
  if (m_radio || !m_state["available"].toBool() || m_state["checking"].toBool())
    return;
  dismissError();
  set("notice", QString{});
  set("starting", true);
  m_radio = launch("radio", {"radio", "up", "--json", "--for",
                             QString::number(m_minutes * 60)});
  QTimer::singleShot(60000, m_radio, [this] {
    if (m_state["starting"].toBool()) {
      fail(tr("No se pudo activar la radio a tiempo."));
      stop();
    }
  });
}

void Backend::terminate(QProcess *process) {
  if (!process)
    return;
  process->setProperty("cancelled", true);
  if (process->state() == QProcess::NotRunning) {
    // launch() may still be queued for this event-loop iteration.
    finish(process, process->property("role").toString(), 0);
    return;
  }
  process->terminate();
  QTimer::singleShot(35000, process, [process] {
    if (process->state() != QProcess::NotRunning)
      process->kill();
  });
}

void Backend::stop() {
  if (!m_radio || m_state["stopping"].toBool())
    return;
  set("stopping", true);
  terminate(m_sender);
  terminate(m_browser);
  terminate(m_receiver);
  set("requests", QVariantList{});
  terminate(m_radio);
}

void Backend::scan() {
  if (!m_state["active"].toBool() || m_state["stopping"].toBool() || m_browser)
    return;
  set("devices", QVariantList{});
  set("scanning", true);
  auto args = QStringList{"browse"} + identityArgs();
  args << "--seconds" << "12";
  m_browser = launch("browse", args, 18000);
}

void Backend::setReceiving(bool enabled) {
  if (!enabled) {
    terminate(m_receiver);
    return;
  }
  if (!m_state["active"].toBool() || m_state["stopping"].toBool() || m_receiver)
    return;
  if (!QDir().mkpath(m_folder)) {
    fail(tr("No se puede crear la carpeta de descargas."));
    return;
  }
  auto args = QStringList{"receive"} + identityArgs();
  args << "--dir" << m_folder;
  m_receiver = launch("receive", args);
  QTimer::singleShot(15000, m_receiver, [this] {
    if (m_receiver && !m_state["receiving"].toBool()) {
      fail(tr(
          "El receptor no respondió. Comprueba que la CLI esté actualizada."));
      terminate(m_receiver);
    }
  });
}

void Backend::sendFiles(const QString &target, const QVariantList &urls) {
  if (target.isEmpty() || urls.isEmpty() || m_sender ||
      !m_state["active"].toBool() || m_state["stopping"].toBool())
    return;
  QStringList paths;
  for (const auto &value : urls) {
    const auto url = value.toUrl();
    const QFileInfo file(url.toLocalFile());
    if (!url.isLocalFile() || !file.isFile() || !file.isReadable()) {
      fail(tr("Selecciona archivos locales que se puedan leer."));
      return;
    }
    paths << file.absoluteFilePath();
  }
  dismissError();
  QString receiverName;
  for (const auto &value : m_state["devices"].toList()) {
    const auto device = value.toMap();
    if (device["id"].toString() == target) {
      receiverName = device["name"].toString();
      break;
    }
  }
  if (receiverName.isEmpty()) {
    fail(tr("El dispositivo ya no está en la lista. Busca y selecciónalo de "
            "nuevo."));
    return;
  }
  set("progress", -1.0);
  set("sending", true);
  set("notice", tr("Localizando de nuevo el dispositivo…"));
  auto args = QStringList{"send"} + identityArgs();
  // Discovery IDs are snapshots. In particular an iPhone may change its
  // AWDL address between browsing and sending. Resolve its advertised name
  // through the CLI's normal mDNS/AWDL lookup instead of pinning the old MAC.
  args << "--to" << receiverName << "--";
  args += paths;
  m_sender = launch("send", args, 300000);
  m_sender->setProperty("paths", paths);
}

void Backend::cancelSend() { terminate(m_sender); }

void Backend::answer(const QString &id, bool accept) {
  if (!m_receiver)
    return;
  bool valid = false;
  const auto number = id.toULongLong(&valid);
  if (!valid)
    return;
  // Only answer prompts still owned by this receiver process.
  bool pending = false;
  for (const auto &value : m_state["requests"].toList())
    pending |= value.toMap()["id"].toString() == id;
  if (!pending)
    return;
  m_receiver->write(
      QJsonDocument(QJsonObject{{"id", qint64(number)}, {"accept", accept}})
          .toJson(QJsonDocument::Compact) +
      '\n');
  removeRequest(id);
  set("notice",
      accept ? tr("Recibiendo transferencia…") : tr("Solicitud rechazada"));
}

void Backend::removeRequest(const QString &id) {
  auto requests = m_state["requests"].toList();
  for (qsizetype i = requests.size(); i > 0; --i)
    if (requests[i - 1].toMap()["id"].toString() == id)
      requests.removeAt(i - 1);
  set("requests", requests);
}

void Backend::event(const QString &role, const QJsonObject &object) {
  const auto type = object["event"].toString();
  if (role == "radio" && type == "ready") {
    m_interface = object["interface"].toString("awdl0");
    set("starting", false);
    set("active", true);
    set("remaining", m_minutes * 60);
    setReceiving(true);
    scan();
  } else if (role == "receive" && type == "ready") {
    set("receiving", true);
  } else if (role == "browse" && type == "device") {
    auto devices = m_state["devices"].toList();
    const auto id = object["id"].toString();
    const auto name = object["name"].toString();
    for (const auto &value : devices) {
      const auto device = value.toMap();
      if (device["id"] == id || device["name"] == name)
        return;
    }
    if (devices.size() < 100 && !id.isEmpty()) {
      devices << QVariantMap{{"id", id}, {"name", name}};
      set("devices", devices);
    }
  } else if (role == "send") {
    if (type == "stage") {
      const auto stage = object["stage"].toString();
      if (stage == "discovering")
        set("notice", tr("Localizando de nuevo el dispositivo…"));
      else if (stage == "preparing")
        set("notice",
            tr("Preparando archivos y solicitando la transferencia…"));
    } else if (type == "progress") {
      const double total = object["total"].toDouble();
      set("progress", total > 0
                          ? qBound(0.0, object["sent"].toDouble() / total, 1.0)
                          : -1.0);
    } else if (type == "accepted") {
      set("notice", tr("Enviando archivos…"));
    } else if (type == "delivered") {
      m_sender->setProperty("delivered", true);
      set("notice", tr("Archivos enviados"));
      record(tr("Archivos enviados"), tr("La transferencia se completó"),
             "document-send", m_sender->property("paths").toStringList());
      notify(tr("Archivos enviados"), tr("La transferencia se completó."));
    } else if (type == "declined") {
      m_sender->setProperty("handled", true);
      set("notice", tr("El dispositivo rechazó la transferencia"));
    } else if (type == "failed") {
      m_sender->setProperty("handled", true);
      fail(object["reason"].toString());
    }
  } else if (role == "receive") {
    const auto id = QString::number(object["id"].toInteger());
    if (type == "request") {
      auto request = object.toVariantMap();
      request["id"] = id;
      auto requests = m_state["requests"].toList();
      requests << request;
      set("requests", requests);
      notify(tr("Solicitud de AirDrop"),
             object["sender"].toString() +
                 tr(" quiere enviarte archivos o enlaces."));
      emit incoming();
    } else if (type == "withdrawn") {
      removeRequest(id);
    } else if (type == "received" || type == "urls") {
      removeRequest(id);
      const auto values =
          object[type == "received" ? "paths" : "urls"].toArray();
      QStringList items;
      for (const auto &value : values)
        items << value.toString();
      const auto title = type == "received" ? tr("Archivos recibidos")
                                            : tr("Enlaces recibidos");
      // Links are displayed as plain text, never opened automatically.
      record(title, items.join('\n'),
             type == "received" ? "folder-downloads" : "edit-link",
             type == "received" ? items : QStringList{});
      set("notice", title);
      notify(title, type == "received" ? tr("Guardados en %1").arg(m_folder)
                                       : items.join('\n'));
    } else if (type == "failed") {
      removeRequest(id);
      fail(object["reason"].toString());
    }
  }
}

void Backend::finish(QProcess *process, const QString &role, int code) {
  if (process->property("finished").toBool() || m_destroying)
    return;
  process->setProperty("finished", true);
  const bool cancelled = process->property("cancelled").toBool();
  const auto detail =
      QString::fromUtf8(process->property("stderr").toByteArray()).trimmed();
  if (role == "doctor") {
    m_doctor = nullptr;
    set("checking", false);
    const auto report =
        QJsonDocument::fromJson(process->property("stdout").toByteArray())
            .object();
    set("report", report.toVariantMap());
    set("available", report["hardware"].toBool() &&
                         report["missing"].isArray() &&
                         report["missing"].toArray().isEmpty());
    if (report.isEmpty())
      fail(detail.isEmpty() ? tr("Owly no devolvió un diagnóstico válido.")
                            : detail);
  } else if (role == "radio") {
    m_radio = nullptr;
    terminate(m_receiver);
    terminate(m_browser);
    terminate(m_sender);
    set("active", false);
    set("starting", false);
    set("stopping", false);
    set("remaining", 0);
    set("requests", QVariantList{});
    set("devices", QVariantList{});
    if (!cancelled && code == 0)
      set("notice", tr("La sesión de AirDrop terminó"));
  } else if (role == "receive") {
    m_receiver = nullptr;
    set("receiving", false);
    set("requests", QVariantList{});
  } else if (role == "browse") {
    m_browser = nullptr;
    set("scanning", false);
  } else if (role == "send") {
    m_sender = nullptr;
    set("sending", false);
    set("progress", -1.0);
    if (cancelled)
      set("notice", tr("Envío cancelado"));
    // A CLI without --json must never look like a successful transfer.
    if (code == 0 && !cancelled && !process->property("delivered").toBool())
      fail(tr("Owly terminó sin confirmar la entrega. Comprueba la versión de "
              "la CLI."));
  }
  if (code != 0 && !cancelled && role != "doctor" &&
      !process->property("handled").toBool())
    fail(detail.isEmpty() ? tr("La operación de Owly terminó inesperadamente.")
                          : detail);
  process->deleteLater();
  if (!m_pendingConfig.isEmpty() && !m_radio && !m_sender && !m_receiver &&
      !m_browser && !m_doctor) {
    const auto config = m_pendingConfig;
    configure(config[0].toString(), config[1].toString(), config[2].toString(),
              config[3].toInt());
  }
}

void Backend::record(const QString &title, const QString &detail,
                     const QString &icon, const QStringList &paths) {
  QVariantList files;
  const QMimeDatabase mimeDatabase;
  for (const auto &path : paths) {
    const QFileInfo file(path);
    // Transfer events carry filesystem paths, never user-supplied URL schemes.
    if (!file.isAbsolute())
      continue;
    files << QVariantMap{
        {"url", QUrl::fromLocalFile(file.absoluteFilePath())},
        {"name", file.fileName()},
        {"path", file.absoluteFilePath()},
        {"icon", mimeDatabase.mimeTypeForFile(file).iconName()}};
  }
  auto history = m_state["history"].toList();
  history.prepend(
      QVariantMap{{"title", title},
                  {"detail", detail},
                  {"icon", icon},
                  {"files", files},
                  {"time", QDateTime::currentDateTime().toString("HH:mm")}});
  while (history.size() > 20)
    history.removeLast();
  set("history", history);
}

void Backend::notify(const QString &title, const QString &body) {
  auto message = QDBusMessage::createMethodCall(
      "org.freedesktop.Notifications", "/org/freedesktop/Notifications",
      "org.freedesktop.Notifications", "Notify");
  message.setArguments({QString("Owly"), uint(0), QString("network-wireless"),
                        title, body.toHtmlEscaped(), QStringList{},
                        QVariantMap{}, 8000});
  QDBusConnection::sessionBus().asyncCall(message);
}

void Backend::fail(const QString &message) {
  if (message.isEmpty())
    return;
  if (m_state["error"].toString() != message)
    record(tr("Error de Owly"), message, "dialog-error");
  set("error", message);
}
void Backend::dismissError() { set("error", QString{}); }
void Backend::openDownloads() {
  QDesktopServices::openUrl(QUrl::fromLocalFile(m_folder));
}

void Backend::openHistoryFile(const QUrl &url, bool folder) {
  // Only recorded local transfers can launch an application on an explicit
  // click.
  if (!url.isLocalFile())
    return;
  bool recorded = false;
  for (const auto &entry : m_state["history"].toList()) {
    for (const auto &item : entry.toMap()["files"].toList())
      recorded |= item.toMap()["url"].toUrl() == url;
  }
  if (!recorded)
    return;
  const QFileInfo file(url.toLocalFile());
  if (!file.exists() || !file.isReadable()) {
    fail(tr("El archivo ya no está disponible en su ubicación original: %1")
             .arg(file.absoluteFilePath()));
    return;
  }
  if (!QDesktopServices::openUrl(
          folder ? QUrl::fromLocalFile(file.absolutePath()) : url))
    fail(tr("No se pudo abrir %1 con la aplicación del sistema.")
             .arg(file.fileName()));
}
