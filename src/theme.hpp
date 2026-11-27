#pragma once
#include <QToolButton>
#include <QPainter>
#include <QPainterPath>
#include <QSvgRenderer>
#include <QHash>
#include <QStyleOptionToolButton>
#include <QPalette>
#include <QVariantAnimation>
#include <QEnterEvent>
#include <QPixmapCache>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QLayout>
#include "appearance.hpp"
#include "scene-palette.hpp"

inline QPixmap glyphPixmap(const QString &id,const QColor&color,int size,qreal dpr=1){
 static const QHash<QString,QString> paths={
 {"note","<circle cx='12' cy='12' r='9'/><path d='M12 11v5m0-8v.1'/>"},
 {"close","<path d='m6 6 12 12M18 6 6 18'/>"},
 {"pin","<path d='m15 3 6 6-4 1-3 5v4l-4-4-6 6m6-6-5-5h4l5-3Z'/>"},
 {"pin-filled","<path d='m15 3 6 6-4 1-3 5v4l-4-4-5-5h4l5-3Z' fill='currentColor'/><path d='m10 15-6 6'/>"},
 {"save","<path d='M4 3h13l3 3v15H4ZM8 3v6h8V3M8 21v-8h8v8'/>"},
 {"load","<path d='M4 13v7h16v-7M12 3v12m-5-5 5 5 5-5'/>"},
 {"trash","<path d='M4 6h16M9 6V3h6v3M6 6l1 15h10l1-15M10 10v7M14 10v7'/>"},
 {"edit","<path d='m15 4 5 5-11 11H4v-5ZM13 6l5 5'/>"},
 {"transition-in","<path d='M3 12h12m-5-5 5 5-5 5M17 4h4v16h-4'/>"},
 {"transition-out","<path d='M9 12h12m-5-5 5 5-5 5M7 4H3v16h4'/>"},
 {"blend","<circle cx='9' cy='12' r='6'/><circle cx='15' cy='12' r='6'/>"},
 {"transform","<rect x='5' y='5' width='14' height='14' rx='1'/><path d='M3 3h4M3 3v4M21 3h-4M21 3v4M3 21h4M3 21v-4M21 21h-4M21 21v-4'/>"},
 {"inspector","<rect x='3' y='4' width='18' height='16' rx='2'/><path d='M3 11h18M8 15h8M8 7h4'/>"},
 {"minus","<path d='M5 12h14'/>"},
 {"plus","<path d='M12 5v14M5 12h14'/>"},
 {"more","<circle cx='12' cy='5' r='1'/><circle cx='12' cy='12' r='1'/><circle cx='12' cy='19' r='1'/>"},
 {"eye","<path d='M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7-10-7-10-7Z'/><circle cx='12' cy='12' r='3'/>"},
 {"eye-off","<path d='M3 3l18 18M10.6 5.1 12 5c6.5 0 10 7 10 7a19 19 0 0 1-3 3.7M6.2 6.2C3.4 8.4 2 12 2 12s3.5 7 10 7c1.7 0 3.1-.4 4.3-1.1'/>"},
 {"lock","<rect x='5' y='10' width='14' height='11' rx='2'/><path d='M8 10V7a4 4 0 0 1 8 0v3M12 14v3'/>"},
 {"unlock","<rect x='5' y='10' width='14' height='11' rx='2'/><path d='M8 10V7a4 4 0 0 1 7.5-2M12 14v3'/>"},
 {"live","<circle cx='12' cy='12' r='2.3' fill='currentColor'/><path d='M8 8a5.7 5.7 0 0 0 0 8m8-8a5.7 5.7 0 0 1 0 8M5 5a10 10 0 0 0 0 14m14-14a10 10 0 0 1 0 14'/>"},
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
inline int themeSpacing(const QWidget*w,Qt::Orientation orientation){
 const int value=w->style()->pixelMetric(orientation==Qt::Horizontal?QStyle::PM_LayoutHorizontalSpacing:QStyle::PM_LayoutVerticalSpacing,nullptr,w);
 return qBound(1,value<0?3:value,16);
}
inline QSize nativeItemSize(const QWidget*w,const QSize&content){
 QStyleOptionViewItem opt;opt.initFrom(w);opt.features=QStyleOptionViewItem::HasDisplay;
 opt.text="M";opt.font=w->font();opt.fontMetrics=w->fontMetrics();
 return w->style()->sizeFromContents(QStyle::CT_ItemViewItem,&opt,content,w).expandedTo(content);
}
class GlyphButton:public QToolButton{
 QString glyph;
 QColor tonalSurface,tonalInk;
 QVariantAnimation hoverMotion,angleMotion,alphaMotion;
 qreal hover=0,angle=0,alpha=1;
 int duration=0;
 bool actionWanted=true,circular=false,indicatorOnly=false;
 void animate(QVariantAnimation&a,qreal from,qreal to){
  a.stop();a.setStartValue(from);a.setEndValue(to);a.setDuration(qMax(1,duration));a.start();
 }
public:
 explicit GlyphButton(QString id,QWidget*parent=nullptr):QToolButton(parent),glyph(std::move(id)){
  setAutoRaise(true);setCursor(Qt::PointingHandCursor);setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);setAttribute(Qt::WA_Hover);
  setToolButtonStyle(Qt::ToolButtonIconOnly);
  // A nonempty icon allows QToolButton to calculate its native content size.
  setIcon(QIcon(glyphPixmap(glyph,palette().color(QPalette::ButtonText),16)));
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
 void setTonalColors(const QColor&surface,const QColor&ink){
  if(tonalSurface==surface&&tonalInk==ink)return;
  tonalSurface=surface;tonalInk=ink;update();
 }
 void setIndicatorOnly(bool value){
  indicatorOnly=value;
  setCursor(value?Qt::ArrowCursor:Qt::PointingHandCursor);
  setFocusPolicy(value?Qt::NoFocus:Qt::StrongFocus);
  update();
 }
 void setCircular(bool value){if(circular!=value){circular=value;updateGeometry();update();}}
 void setGlyph(const QString&id){if(glyph!=id){glyph=id;update();}}
 QSize themedIconSize()const{
  int side=style()->pixelMetric(QStyle::PM_ToolBarIconSize,nullptr,this);
  if(side<=0)side=style()->pixelMetric(QStyle::PM_ButtonIconSize,nullptr,this);
  return QSize(qBound(8,side,64),qBound(8,side,64));
 }
 QSize sizeHint()const override{
  const_cast<GlyphButton*>(this)->ensurePolished();
  QStyleOptionToolButton opt;initStyleOption(&opt);opt.iconSize=themedIconSize();
  auto size=style()->sizeFromContents(QStyle::CT_ToolButton,&opt,opt.iconSize,this).expandedTo(opt.iconSize);
  if(circular){int side=qMax(size.width(),size.height());return {side,side};}return size;
 }
 QSize minimumSizeHint()const override{return sizeHint();}
protected:
 bool hitButton(const QPoint&p)const override{return !indicatorOnly&&QToolButton::hitButton(p);}
 void changeEvent(QEvent*e)override{QToolButton::changeEvent(e);if(e->type()==QEvent::StyleChange||e->type()==QEvent::FontChange||e->type()==QEvent::PaletteChange){updateGeometry();update();}}
 void enterEvent(QEnterEvent*e)override{if(duration)animate(hoverMotion,hover,1);else{hover=1;update();}QToolButton::enterEvent(e);}
 void leaveEvent(QEvent*e)override{if(duration)animate(hoverMotion,hover,0);else{hover=0;update();}QToolButton::leaveEvent(e);}
 void paintEvent(QPaintEvent*)override{
  QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.setOpacity(alpha);
  if(circular&&!tonalSurface.isValid()){QPainterPath clip;const qreal side=qMin(width(),height());clip.addEllipse(QRectF((width()-side)/2,(height()-side)/2,side,side));p.setClipPath(clip);}
  QStyleOptionToolButton opt;initStyleOption(&opt);opt.iconSize=themedIconSize();
  QColor fg=palette().color(isEnabled()?QPalette::Active:QPalette::Disabled,QPalette::ButtonText);
  const bool tonal=tonalSurface.isValid()&&tonalInk.isValid();
  if(tonal){
   const bool live=glyph=="live";
   const auto baseInk=contrastInk(tonalSurface,tonalInk,3);
   const auto fill=live?mixColor(tonalSurface,baseInk,.76):tonalSurface;
   fg=contrastInk(fill,live?tonalSurface:baseInk,3);
   const qreal side=qMin(width(),height());
   const qreal inset=qMin(2.0,side*.08);
   QRectF shape=QRectF(rect()).adjusted(inset,inset,-inset,-inset);
   if(circular)shape=QRectF((width()-side)/2+inset,(height()-side)/2+inset,side-2*inset,side-2*inset);
   const qreal radius=circular?shape.height()/2:qMin(shape.height()/2,6.0);
   if(live){p.setPen(Qt::NoPen);p.setBrush(fill);p.drawRoundedRect(shape,radius,radius);}
   if(!indicatorOnly){
    auto state=fg;state.setAlphaF(isDown()?.18:.12*hover);
    if(state.alpha()>0){p.setPen(Qt::NoPen);p.setBrush(state);p.drawRoundedRect(shape,radius,radius);}
    if(hasFocus()&&focusPolicy()!=Qt::NoFocus){QPen pen(fg);pen.setWidthF(1);p.setPen(pen);p.setBrush(Qt::NoBrush);p.drawRoundedRect(shape.adjusted(.5,.5,-.5,-.5),radius,radius);}
   }
   if(!isEnabled())p.setOpacity(alpha*.55);
  }
  const int size=opt.iconSize.width();
  auto pix=glyphPixmap(glyph,fg,size,devicePixelRatioF());
  if(qAbs(angle)>0.01&&qAbs(angle-360)>0.01){
   QPixmap rotated(pix.size());rotated.setDevicePixelRatio(pix.devicePixelRatio());rotated.fill(Qt::transparent);
   QPainter painter(&rotated);painter.setRenderHint(QPainter::SmoothPixmapTransform);painter.translate(size/2.0,size/2.0);painter.rotate(angle);painter.drawPixmap(QPointF(-size/2.0,-size/2.0),pix);painter.end();pix=rotated;
  }
  if(tonal){p.drawPixmap(QPointF((width()-size)/2.0,(height()-size)/2.0),pix);return;}
  opt.icon=QIcon(pix);opt.icon.addPixmap(pix,QIcon::Disabled);
  if(indicatorOnly){
   opt.state&=~(QStyle::State_MouseOver|QStyle::State_Sunken|QStyle::State_HasFocus);
   style()->drawComplexControl(QStyle::CC_ToolButton,&opt,&p,this);return;
  }
  // Delegate the complete button surface and glyph placement to the active style.
  // Blend only the hover layer; press/focus feedback stays immediate.
  if(!isDown()&&!hasFocus()&&hover>0&&hover<1){
   auto base=opt;base.state&=~QStyle::State_MouseOver;
   style()->drawComplexControl(QStyle::CC_ToolButton,&base,&p,this);
   p.setOpacity(alpha*hover);opt.state|=QStyle::State_MouseOver;
  }else if(!isDown()&&!hasFocus()){if(hover>0)opt.state|=QStyle::State_MouseOver;else opt.state&=~QStyle::State_MouseOver;}
  style()->drawComplexControl(QStyle::CC_ToolButton,&opt,&p,this);
 }
};
