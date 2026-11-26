#pragma once
#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QStyle>
#include <QVector>
#include <QPainter>
#include <QContextMenuEvent>
#include <QListView>
#include <QStyleOptionViewItem>
#include <QPointer>
#include <QApplication>
#include <functional>
#include "theme.hpp"
#include "motion.hpp"

class ElidedLabel:public QLabel{
 QString full; QColor ink;
public:
 explicit ElidedLabel(const QString&s,QWidget*p):QLabel(p),full(s){
  setToolTip(s);setAccessibleName(s);setMinimumWidth(0);setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);setTextFormat(Qt::PlainText);updateText();
 }
 void setInk(const QColor&c){ink=c;update();}
 void setFullText(const QString&s){full=s;setAccessibleName(s);updateText();}
 void updateText(){setText(fontMetrics().elidedText(full,Qt::ElideRight,qMax(0,width())));}
protected:
 void paintEvent(QPaintEvent*)override{
  QPainter p(this);p.setFont(font());p.setPen(ink.isValid()?ink:palette().color(QPalette::WindowText));
  p.drawText(rect(),Qt::AlignVCenter|Qt::AlignLeft|Qt::TextSingleLine,text());
 }
 void resizeEvent(QResizeEvent*e)override{QLabel::resizeEvent(e);updateText();}
 void changeEvent(QEvent*e)override{QLabel::changeEvent(e);if(e->type()==QEvent::FontChange)updateText();}
};
class Row : public QWidget {
public:
 ElidedLabel *name=nullptr;
 QLabel *thumb;
 QWidget *secondary;
 QVector<QToolButton*> hoverButtons, secondaryButtons;
 std::function<void()> onBody,onAppearance;
 std::function<bool(QPoint)> onDragMove;
 std::function<void(QPoint)> onDrop;
 std::function<void()> onDragCancel;
 void cancelGesture(){pressed=dragging=false;}
 void setDropIndicator(int edge){if(dropEdge!=edge){dropEdge=edge;update();}}
 void setAccent(const QColor &color){accentColor=color;update();}
 QColor accent()const{return accentColor;}
 Row(const QString &title,bool scene,bool open,int depth,QWidget *parent=nullptr):QWidget(parent),isScene(scene),nestDepth(depth){
  setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);setAttribute(Qt::WA_Hover);
  rowHover.setEasingCurve(QEasingCurve::OutCubic);
  connect(&rowHover,&QVariantAnimation::valueChanged,this,[this](const QVariant&v){hoverAmount=v.toReal();update();});
  auto *outer=new QVBoxLayout(this);outer->setContentsMargins(0,0,0,0);
  top=new QWidget(this);first=new QHBoxLayout(top);first->setContentsMargins(0,0,0,0);outer->addWidget(top);
  disclosure=new GlyphButton("right",top);disclosure->setFocusPolicy(Qt::NoFocus);
  connect(disclosure,&QToolButton::clicked,this,[this]{if(onBody)onBody();});first->addWidget(disclosure);disclosure->hide();
  thumb=new QLabel(top);thumb->setFixedSize(scene?48:36,scene?27:22);thumb->setAlignment(Qt::AlignCenter);first->addWidget(thumb);
  name=new ElidedLabel(title,top);if(scene){auto f=name->font();f.setWeight(QFont::DemiBold);name->setFont(f);}first->addWidget(name,1);
  name->installEventFilter(this);thumb->installEventFilter(this);top->installEventFilter(this);
  status=new ElidedLabel("",top);status->hide();first->addWidget(status);status->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
  secondary=new QWidget(this);extra=new QHBoxLayout(secondary);extra->setContentsMargins(0,0,0,0);extra->addStretch();secondaryReveal=new Reveal(secondary,this);outer->addWidget(secondaryReveal);
  applyGeometry();setExpanded(open);
 }
 void setDisclosure(bool enabled,bool open){disclosure->setVisible(enabled);disclosure->setAngle(open?90:0);disclosure->setToolTip(open?"Collapse":"Expand");}
 void setSelected(bool value){selected=value;applyInk();update();}
 void setNativeList(QListView *v){if(nativeList!=v){nativeList=v;applyGeometry();update();}}
 void setStatus(const QString&s){status->setFullText(s);auto f=name->font();f.setWeight(QFont::Normal);if(f.pointSizeF()>2)f.setPointSizeF(f.pointSizeF()*.82);status->setFont(f);status->setFixedWidth(status->fontMetrics().horizontalAdvance(s)+2*themeSpacing(this,Qt::Horizontal));status->setVisible(!s.isEmpty());status->setToolTip(s);}
 QColor inkColor()const{return palette().color(selected?QPalette::HighlightedText:QPalette::WindowText);}
 void setMotionDuration(int ms){duration=ms;if(!ms){rowHover.stop();hoverAmount=underMouse()?1:0;update();}if(!ms){secondaryReveal->finish();}for(auto*b:hoverButtons)static_cast<GlyphButton*>(b)->setMotionDuration(ms);for(auto*b:secondaryButtons)static_cast<GlyphButton*>(b)->setMotionDuration(ms);disclosure->setMotionDuration(ms);}
 bool motionActive()const{return secondaryReveal->moving();}
 void setExpanded(bool value){opened=value;secondaryReveal->setOpen(value&&!isScene,duration);}
 void settleMotion(){secondaryReveal->finish();}
 bool expanded()const{return opened;}
 void addHover(const QString &tip,const QString&icon,std::function<void()> action,bool always=false){
  auto *b=makeButton(tip,icon,std::move(action));first->addWidget(b);hoverButtons.append(b);
  b->setProperty("always",always);auto p=b->sizePolicy();p.setRetainSizeWhenHidden(true);b->setSizePolicy(p);static_cast<GlyphButton*>(b)->revealAction(always||underMouse());
 }
 void addSecondary(const QString &tip,const QString&icon,std::function<void()> action){
  auto *b=makeButton(tip,icon,std::move(action));extra->addWidget(b);secondaryButtons.append(b);
 }
protected:
 void paintEvent(QPaintEvent*event) override{
  QWidget::paintEvent(event);QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
  const auto surface=rect();
  if(selected||hoverAmount>0){
   p.save();if(!selected)p.setOpacity(hoverAmount);
   auto*view=nativeList?static_cast<QWidget*>(nativeList.data()):this;
   QStyleOptionViewItem opt;opt.initFrom(view);opt.rect=surface;opt.state=QStyle::State_Enabled|QStyle::State_Active|(selected?QStyle::State_Selected:QStyle::State_MouseOver);opt.showDecorationSelected=true;
   view->style()->drawControl(QStyle::CE_ItemViewItem,&opt,&p,view);
   p.restore();
  }
  if(accentColor.isValid()){
   auto tint=accentColor;tint.setAlpha(selected?22:38);p.setPen(Qt::NoPen);p.setBrush(tint);p.fillRect(surface,tint);
   p.setBrush(accentColor);p.fillRect(QRect(0,0,qMax(2,themeSpacing(this,Qt::Horizontal)/2),height()),accentColor);
  }
  if(dropEdge){QPen pen(palette().color(QPalette::Highlight));pen.setWidth(2);p.setPen(pen);int y=dropEdge<0?1:height()-2;p.drawLine(outerLeft(),y,width()-2,y);}
  if(opened&&!isScene){auto divider=inkColor();divider.setAlpha(32);p.setPen(divider);const int y=secondaryReveal->geometry().top()-1;p.drawLine(outerLeft(),y,width()-5,y);}
 }
 void contextMenuEvent(QContextMenuEvent*e)override{if(onAppearance){onAppearance();e->accept();}else QWidget::contextMenuEvent(e);}
 void enterEvent(QEnterEvent *e)override{animateHover(1);for(auto*b:hoverButtons)static_cast<GlyphButton*>(b)->revealAction(true);update();QWidget::enterEvent(e);}
 void leaveEvent(QEvent *e)override{animateHover(0);for(auto*b:hoverButtons)static_cast<GlyphButton*>(b)->revealAction(b->property("always").toBool());update();QWidget::leaveEvent(e);}
 void changeEvent(QEvent*e)override{QWidget::changeEvent(e);if(ready&&(e->type()==QEvent::PaletteChange||e->type()==QEvent::StyleChange||e->type()==QEvent::FontChange)){applyGeometry();applyInk();}}
 bool eventFilter(QObject *o,QEvent *e)override{
  if(e->type()==QEvent::ContextMenu&&onAppearance){onAppearance();return true;}
  if(e->type()==QEvent::MouseButtonPress){
   auto*m=static_cast<QMouseEvent*>(e);if(m->button()==Qt::LeftButton){pressed=true;dragging=false;pressPoint=m->globalPosition().toPoint();return true;}
  }
  if(e->type()==QEvent::MouseMove&&pressed){
   auto*m=static_cast<QMouseEvent*>(e);auto pos=m->globalPosition().toPoint();
   if(m->buttons()&Qt::LeftButton){
    if(dragging||(onDragMove&&(pos-pressPoint).manhattanLength()>=QApplication::startDragDistance())){
     dragging=true;if(onDragMove)onDragMove(pos);return true;
    }
   }
  }
  if(e->type()==QEvent::MouseButtonRelease&&static_cast<QMouseEvent*>(e)->button()==Qt::LeftButton){
   auto*m=static_cast<QMouseEvent*>(e);bool wasDragging=dragging,wasPressed=pressed;pressed=dragging=false;
   if(wasDragging){if(onDrop)onDrop(m->globalPosition().toPoint());}
   else if(wasPressed&&onBody)onBody();return true;
  }
  if(e->type()==QEvent::UngrabMouse&&pressed){pressed=dragging=false;if(onDragCancel)onDragCancel();}
  return QWidget::eventFilter(o,e);
 }
private:
 int dropEdge=0;bool pressed=false,dragging=false;QPoint pressPoint;
 bool isScene,opened=false,selected=false;int nestDepth=0;bool ready=false;QWidget*top=nullptr;QColor accentColor; int duration=0;Reveal*secondaryReveal;
 QVariantAnimation rowHover;qreal hoverAmount=0;
 void animateHover(qreal target){rowHover.stop();if(!duration){hoverAmount=target;update();return;}rowHover.setDuration(duration);rowHover.setStartValue(hoverAmount);rowHover.setEndValue(target);rowHover.start();}
 QHBoxLayout *first,*extra;GlyphButton*disclosure;ElidedLabel*status;QPointer<QListView>nativeList;
 int outerLeft()const{return layout()->contentsMargins().left();}
 void applyGeometry(){
  const auto h=themeSpacing(this,Qt::Horizontal),v=themeSpacing(this,Qt::Vertical);
  auto*view=nativeList?static_cast<QWidget*>(nativeList.data()):this;
  const QSize content(fontMetrics().horizontalAdvance("M"),fontMetrics().height());
  const auto native=nativeItemSize(view,content);
  const auto insetX=qBound(2,(native.width()-content.width())/2,16);
  const auto insetY=qBound(1,(native.height()-content.height())/2,12);
  const int indent=disclosure->sizeHint().width()+h;
  layout()->setContentsMargins(insetX+nestDepth*indent,insetY,insetX,insetY);
  layout()->setSpacing(qMax(1,v/2));first->setSpacing(h);extra->setSpacing(h);
  top->setMinimumHeight(qMax(fontMetrics().height(),native.height()-2*insetY));
  ready=true;updateGeometry();
 }
 void applyInk(){
  const auto fg=inkColor();
  for(auto*l:{name,status}){auto p=l->palette();p.setColor(QPalette::WindowText,fg);p.setColor(QPalette::Text,fg);l->setPalette(p);l->setInk(fg);}
  disclosure->update();for(auto*b:hoverButtons)b->update();for(auto*b:secondaryButtons)b->update();
 }
 QToolButton*makeButton(const QString&tip,const QString&icon,std::function<void()>action){
  auto*b=new GlyphButton(icon,this);b->setMotionDuration(duration);b->setToolTip(tip);b->setAccessibleName(tip);connect(b,&QToolButton::clicked,this,[a=std::move(action)]{a();});return b;
 }
};
