// SPDX-License-Identifier: MIT
#pragma once

#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QVariantMap>

class Backend : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
public:
  explicit Backend(QObject *parent = nullptr);
  ~Backend() override;
  QVariantMap state() const { return m_state; }
  Q_INVOKABLE void configure(const QString &executable, const QString &folder,
                             const QString &name, int minutes);
  Q_INVOKABLE void diagnose();
  Q_INVOKABLE void start();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void scan();
  Q_INVOKABLE void setReceiving(bool enabled);
  Q_INVOKABLE void sendFiles(const QString &target, const QVariantList &urls);
  Q_INVOKABLE void cancelSend();
  Q_INVOKABLE void answer(const QString &id, bool accept);
  Q_INVOKABLE void dismissError();
  Q_INVOKABLE void openDownloads();
signals:
  void changed();
  void incoming();

private:
  QProcess *launch(const QString &role, const QStringList &args,
                   int timeout = 0);
  void event(const QString &role, const QJsonObject &object);
  void finish(QProcess *process, const QString &role, int code);
  void terminate(QProcess *process);
  void set(const QString &key, const QVariant &value);
  void fail(const QString &message);
  void record(const QString &title, const QString &detail, const QString &icon);
  void notify(const QString &title, const QString &body);
  void removeRequest(const QString &id);
  QStringList identityArgs() const;
  QProcess *m_radio = nullptr;
  QProcess *m_receiver = nullptr;
  QProcess *m_browser = nullptr;
  QProcess *m_sender = nullptr;
  QProcess *m_doctor = nullptr;
  QVariantMap m_state;
  QString m_executable;
  QString m_folder;
  QString m_name;
  QString m_interface = QStringLiteral("awdl0");
  int m_minutes = 10;
  bool m_destroying = false;
  QVariantList m_pendingConfig;
};
