#pragma once
#include <QWidget>
#include <QRect>
#include <QSize>
#include <QByteArray>
#include <cmath>
enum class ProjectorSize {Monitor,Review};
inline QRect projectorWindowRect(QSize source,const QRect &available,ProjectorSize preset){
 if(available.isEmpty())return {};
 if(source.isEmpty())source=QSize(16,9);
 const auto limit=(preset==ProjectorSize::Monitor?QSize(320,240):QSize(960,640)).boundedTo(QSize(qMax(1,available.width()-32),qMax(1,available.height()-48)));
 const double scale=qMin(double(limit.width())/source.width(),double(limit.height())/source.height());
 const QSize size(qBound(1,qRound(source.width()*scale),limit.width()),qBound(1,qRound(source.height()*scale),limit.height()));
 // Leave room for native window decorations. Monitor windows start at
 // the top-right; review windows are centered on the chosen display.
 const QPoint pos=preset==ProjectorSize::Monitor?
  QPoint(available.right()-size.width()+1-qMin(16,(available.width()-size.width())/2),available.top()+qMin(32,(available.height()-size.height())/2)):
  QPoint(available.x()+(available.width()-size.width())/2,available.y()+(available.height()-size.height())/2);
 return {pos,size};
}
// The public OBS API expects the same base64 Qt geometry used by its own
// saved projectors. This temporary widget never gets shown or renders.
inline QByteArray projectorGeometry(const QRect &rect){
 if(rect.isEmpty())return {};
 QWidget geometry(nullptr,Qt::Window);
 geometry.setGeometry(rect);
 return geometry.saveGeometry().toBase64();
}
