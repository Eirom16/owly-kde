// SPDX-License-Identifier: MIT
#pragma once

#include <QPixmap>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QUrl>

namespace KIO {
class PreviewJob;
}

class FileThumbnail : public QQuickPaintedItem {
  Q_OBJECT
  Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
  Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
public:
  explicit FileThumbnail(QQuickItem *parent = nullptr);
  ~FileThumbnail() override;
  QUrl source() const { return m_source; }
  void setSource(const QUrl &source);
  bool ready() const { return !m_preview.isNull(); }
  void paint(QPainter *painter) override;
signals:
  void sourceChanged();
  void readyChanged();

protected:
  void componentComplete() override;

private:
  void reload();
  void cancel();
  QUrl m_source;
  QPixmap m_preview;
  QPointer<KIO::PreviewJob> m_job;
};
