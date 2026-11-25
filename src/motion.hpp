#pragma once
#include <QWidget>
#include <QVariantAnimation>
#include <QResizeEvent>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QPointer>
#include <functional>

// An interruptible clipping viewport: its child retains its natural geometry.
// Only the viewport height changes; no nested event loop or blocking sleeps.
class Reveal:public QWidget{
 QWidget*child;
 QVariantAnimation animation;
 bool wanted=true;
 int extent=-1;
 bool fadeContents=false;QPointer<QGraphicsOpacityEffect> fade;
 void apply(int h){extent=qMax(0,h);setFixedHeight(extent);place();if(fade)fade->setOpacity(qBound(0.0,qreal(extent)/qMax(1,naturalHeight()),1.0));}
 void place(){child->setGeometry(0,0,width(),qMax(height(),naturalHeight()));}
 void settle(){
  animation.stop();
  if(fade){child->setGraphicsEffect(nullptr);fade=nullptr;}
  if(wanted){extent=-1;setMinimumHeight(0);setMaximumHeight(QWIDGETSIZE_MAX);child->show();show();updateGeometry();place();}
  else{apply(0);child->hide();hide();}
  if(onSettled)onSettled(wanted);
 }
public:
 std::function<void(bool)> onSettled;
 explicit Reveal(QWidget*c,QWidget*parent=nullptr,bool fadeRows=false):QWidget(parent),child(c),fadeContents(fadeRows){
  child->setParent(this);child->installEventFilter(this);child->show();
  setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
  animation.setEasingCurve(QEasingCurve::OutCubic);
  connect(&animation,&QVariantAnimation::valueChanged,this,[this](const QVariant&v){apply(v.toInt());});
  connect(&animation,&QVariantAnimation::finished,this,[this]{settle();});
 }
 int naturalHeight()const{return qMax(0,qMax(child->minimumHeight(),child->sizeHint().height()));}
 QSize sizeHint()const override{return {child->sizeHint().width(),extent<0?naturalHeight():extent};}
 QSize minimumSizeHint()const override{return {0,0};}
 bool moving()const{return animation.state()==QAbstractAnimation::Running;}
 bool isOpen()const{return wanted;}
 void setOpen(bool open,int ms){
  if(wanted==open){if(moving()&&ms<=0)settle();else if(!moving()&&!open&&!isHidden())settle();return;}
  const int start=isHidden()?0:height();
  wanted=open;animation.stop();
  if(ms<=0||!parentWidget()||!parentWidget()->isVisible()){settle();return;}
  const int end=open?naturalHeight():0;
  if(start==end){settle();return;}
  if(fadeContents&&!fade){fade=new QGraphicsOpacityEffect(child);child->setGraphicsEffect(fade);}
  child->show();apply(start);show();
  animation.setDuration(ms);animation.setStartValue(start);animation.setEndValue(end);animation.start();
 }
 void finish(){if(moving())settle();}
protected:
 void resizeEvent(QResizeEvent*e)override{QWidget::resizeEvent(e);place();}
 bool eventFilter(QObject*o,QEvent*e)override{
  if(o==child&&(e->type()==QEvent::LayoutRequest||e->type()==QEvent::FontChange)){updateGeometry();place();}
  return QWidget::eventFilter(o,e);
 }
};
