#pragma once
#include <QToolButton>
#include <QPainter>
#include <QSvgRenderer>
#include <QHash>
#include <QStyleOptionToolButton>
#include <QPalette>
#include <QVariantAnimation>
#include <QEnterEvent>
#include <QPixmapCache>
#include "appearance.hpp"

inline QPixmap glyphPixmap(const QString &id,const QColor&color,int size,qreal dpr=1){
 static const QHash<QString,QString> paths={
 {"plus","<path d='M12 5v14M5 12h14'/>"},
 {"more","<circle cx='12' cy='5' r='1'/><circle cx='12' cy='12' r='1'/><circle cx='12' cy='19' r='1'/>"},
 {"eye","<path d='M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7-10-7-10-7Z'/><circle cx='12' cy='12' r='3'/>"},
 {"eye-off","<path d='M3 3l18 18M10.6 5.1 12 5c6.5 0 10 7 10 7a19 19 0 0 1-3 3.7M6.2 6.2C3.4 8.4 2 12 2 12s3.5 7 10 7c1.7 0 3.1-.4 4.3-1.1'/>"},
 {"lock","<rect x='5' y='10' width='14' height='11' rx='2'/><path d='M8 10V7a4 4 0 0 1 8 0v3M12 14v3'/>"},
 {"unlock","<rect x='5' y='10' width='14' height='11' rx='2'/><path d='M8 10V7a4 4 0 0 1 7.5-2M12 14v3'/>"},
 {"arrow-right","<path d='M4 12h15m-6-6 6 6-6 6'/>"},
 {"up","<path d='m6 15 6-6 6 6'/>"},
 {"down","<path d='m6 9 6 6 6-6'/>"},
 {"right","<path d='m9 6 6 6-6 6'/>"},
 {"refresh","<path d='M20 8a8 8 0 1 0 0 8M20 3v5h-5'/>"},
 {"filters","<path d='M3 6h18M7 12h10m-7 6h4'/><circle cx='8' cy='6' r='2' fill='currentColor'/><circle cx='15' cy='12' r='2' fill='currentColor'/>"},
 {"properties","<path d='M4 7h16M4 17h16'/><rect x='7' y='4' width='4' height='6' rx='1' fill='currentColor'/><rect x='14' y='14' width='4' height='6' rx='1' fill='currentColor'/>"},
 {"appearance","<path d='M12 3a9 9 0 1 0 0 18h2a2 2 0 0 0 0-4h-1a2 2 0 0 1 0-4h4a4 4 0 0 0 4-4c0-3.3-4-6-9-6Z'/><path d='M7 10h.1M10 6h.1M15 6h.1M6 15h.1' stroke-width='2.7'/>"},
 {"volume","<path d='M3 9h4l5-4v14l-5-4H3ZM16 9a5 5 0 0 1 0 6m3-9a9 9 0 0 1 0 12'/>"},
 {"mute","<path d='M3 9h4l5-4v14l-5-4H3Zm13 1 5 5m0-5-5 5'/>"},
 {"collapse-all","<path d='m7 10 5-5 5 5M7 19l5-5 5 5'/>"},
 {"expand-all","<path d='m7 5 5 5 5-5M7 14l5 5 5-5'/>"},
 {"media","<path d='m8 5 12 7-12 7Z'/>"},
 {"text","<path d='M4 5h16M12 5v14m-4 0h8'/>"},
 {"browser","<circle cx='12' cy='12' r='9'/><ellipse cx='12' cy='12' rx='4' ry='9'/><path d='M3 12h18'/>"},
 {"color","<path d='m13 4 7 7-9 9H4v-7Zm-5 5 7 7'/>"}
 };
 const auto key=QString("unscene:%1:%2:%3:%4").arg(id,color.name(QColor::HexArgb)).arg(size).arg(dpr);QPixmap cached;if(QPixmapCache::find(key,&cached))return cached;
 QString path=paths.value(id,iconPaths().value(id));if(path.isEmpty()||size<1)return {};
 path.replace("currentColor",color.name());
 auto svg=QString("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%1' stroke-width='1.7' stroke-linecap='round' stroke-linejoin='round'>%2</svg>").arg(color.name(),path);
 QSvgRenderer renderer(svg.toUtf8());QPixmap pix(qMax(1,qRound(size*dpr)),qMax(1,qRound(size*dpr)));pix.setDevicePixelRatio(dpr);pix.fill(Qt::transparent);
 {QPainter p(&pix);renderer.render(&p);}QPixmapCache::insert(key,pix);return pix;
}
inline bool hasVisiblePixels(const QImage&image){
 if(image.isNull())return false;
 if(!image.hasAlphaChannel())return true;
 for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(qAlpha(image.pixel(x,y))>8)return true;
 return false;
}
inline QColor mixColor(const QColor&a,const QColor&b,qreal t){
 return QColor::fromRgbF(a.redF()*(1-t)+b.redF()*t,a.greenF()*(1-t)+b.greenF()*t,a.blueF()*(1-t)+b.blueF()*t);
}
class GlyphButton:public QToolButton{
 QString glyph; QColor ink;
 QVariantAnimation hoverMotion,angleMotion,alphaMotion;
 qreal hover=0,angle=0,alpha=1;
 int duration=0;
 bool actionWanted=true;
 void animate(QVariantAnimation&a,qreal from,qreal to){
  a.stop();a.setStartValue(from);a.setEndValue(to);a.setDuration(qMax(1,duration));a.start();
 }
public:
 explicit GlyphButton(QString id,QWidget*parent=nullptr):QToolButton(parent),glyph(std::move(id)){
  setAutoRaise(true);setCursor(Qt::PointingHandCursor);setFixedSize(22,22);setIconSize(QSize(16,16));setAttribute(Qt::WA_Hover);
  for(auto*a:{&hoverMotion,&angleMotion,&alphaMotion})a->setEasingCurve(QEasingCurve::OutCubic);
  connect(&hoverMotion,&QVariantAnimation::valueChanged,this,[this](auto v){hover=v.toReal();update();});
  connect(&angleMotion,&QVariantAnimation::valueChanged,this,[this](auto v){angle=v.toReal();update();});
  connect(&alphaMotion,&QVariantAnimation::valueChanged,this,[this](auto v){alpha=v.toReal();update();});
  connect(&alphaMotion,&QVariantAnimation::finished,this,[this]{if(!actionWanted)hide();});
 }
 void setMotionDuration(int ms){duration=ms;if(!ms){hoverMotion.stop();angleMotion.stop();alphaMotion.stop();hover=underMouse()?1:0;angle=angleMotion.endValue().isValid()?angleMotion.endValue().toReal():angle;alpha=actionWanted?1:0;if(property("always").isValid()){setVisible(actionWanted);setAttribute(Qt::WA_TransparentForMouseEvents,!actionWanted);}update();}}
 void setAngle(qreal to){if(angle==to&&!angleMotion.state())return;if(!duration||!isVisible()){angleMotion.stop();angle=to;angleMotion.setEndValue(to);update();}else if(!angleMotion.state()||angleMotion.endValue().toReal()!=to)animate(angleMotion,angle,to);}
 void revealAction(bool showAction){
  actionWanted=showAction;
  if(!duration||!parentWidget()||!parentWidget()->isVisible()){alphaMotion.stop();alpha=showAction?1:0;setVisible(showAction);setAttribute(Qt::WA_TransparentForMouseEvents,!showAction);update();return;}
  setAttribute(Qt::WA_TransparentForMouseEvents,!showAction);
  if(showAction&&isHidden()){alpha=0;show();}
  if(!showAction&&isHidden())return;
  if(alpha==qreal(showAction)){alphaMotion.stop();if(!showAction)hide();return;}
  animate(alphaMotion,alpha,showAction?1:0);
 }
 void spin(){if(!duration)return;angleMotion.stop();angle=0;animate(angleMotion,0,360);}
 void setGlyph(const QString&id){if(glyph!=id){glyph=id;update();}}
 void setInk(const QColor&c){if(ink!=c){ink=c;update();}}
protected:
 void enterEvent(QEnterEvent*e)override{if(duration)animate(hoverMotion,hover,1);else{hover=1;update();}QToolButton::enterEvent(e);}
 void leaveEvent(QEvent*e)override{if(duration)animate(hoverMotion,hover,0);else{hover=0;update();}QToolButton::leaveEvent(e);}
 void paintEvent(QPaintEvent*)override{
  QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.setOpacity(alpha);
  const auto fg=ink.isValid()?ink:palette().color(isEnabled()?QPalette::Active:QPalette::Disabled,QPalette::WindowText);
  if(hover>0||isDown()||hasFocus()){auto fill=fg;fill.setAlpha(isDown()?52:qRound(24*qMax(hover,hasFocus()?1.0:0.0)));p.setPen(Qt::NoPen);p.setBrush(fill);p.drawRoundedRect(rect().adjusted(1,1,-1,-1),4,4);}
  if(hasFocus()){QPen pen(fg);pen.setStyle(Qt::DotLine);p.setPen(pen);p.setBrush(Qt::NoBrush);p.drawRoundedRect(rect().adjusted(1,1,-1,-1),4,4);}
  int size=iconSize().width();const auto pix=glyphPixmap(glyph,fg,size,devicePixelRatioF());p.translate(width()/2.0,height()/2.0);p.rotate(angle);p.drawPixmap(QPoint(-size/2,-size/2),pix);
 }
};
