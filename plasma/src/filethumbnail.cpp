// SPDX-License-Identifier: MIT
#include "filethumbnail.h"
#include <KFileItem>
#include <KIO/PreviewJob>
#include <QFileInfo>
#include <QPainter>
#include <QQuickWindow>

FileThumbnail::FileThumbnail(QQuickItem *parent) : QQuickPaintedItem(parent) {
  connect(this, &QQuickItem::windowChanged, this, [this] { reload(); });
}

FileThumbnail::~FileThumbnail() { cancel(); }

void FileThumbnail::cancel() {
  if (m_job) {
    m_job->disconnect(this);
    m_job->kill();
    m_job = nullptr;
  }
}

void FileThumbnail::setSource(const QUrl &source) {
  if (m_source == source)
    return;
  m_source = source;
  emit sourceChanged();
  reload();
}

void FileThumbnail::componentComplete() {
  QQuickPaintedItem::componentComplete();
  reload();
}

void FileThumbnail::reload() {
  cancel();
  m_preview = {};
  emit readyChanged();
  update();
  if (!isComponentComplete() || !m_source.isLocalFile() ||
      !QFileInfo(m_source.toLocalFile()).isReadable())
    return;
  // KIO applies KDE's enabled preview plugins, size limits and thumbnail cache.
  // A bounded preview keeps full-resolution photos out of Plasma's scene graph.
  m_job = KIO::filePreview({KFileItem(m_source)}, QSize(128, 128));
  m_job->setDevicePixelRatio(window() ? window()->devicePixelRatio() : 1.0);
  connect(m_job, &KIO::PreviewJob::gotPreview, this,
          [this](const KFileItem &, const QPixmap &preview) {
            m_preview = preview;
            emit readyChanged();
            update();
          });
}

void FileThumbnail::paint(QPainter *painter) {
  if (m_preview.isNull())
    return;
  QSizeF size = m_preview.size();
  size.scale(boundingRect().size(), Qt::KeepAspectRatio);
  const QRectF target((width() - size.width()) / 2,
                      (height() - size.height()) / 2, size.width(),
                      size.height());
  painter->setRenderHint(QPainter::SmoothPixmapTransform);
  painter->drawPixmap(target, m_preview, m_preview.rect());
}
