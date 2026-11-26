#pragma once
#include <QSplitter>
#include <QVariantAnimation>
#include <QResizeEvent>
#include <QWidget>

// Animate the actual split, clipping the inspector at its full content height
// until it settles. No repeated form relayout, captured frames or graphics API.
class InspectorDrawer:public QWidget{
 QWidget*content;
 QSplitter*split;
 QVariantAnimation animation;
 bool wanted=false,adjusting=false;
 int remembered=0,fullHeight=0;
 int available()const{return qMax(0,split->height()-split->handleWidth());}
 int preferred()const{
  int total=available();
  return qBound(0,remembered?remembered:qMax(260,total/2),qMax(0,total-100));
 }
 void place(){content->setGeometry(0,0,width(),qMax(height(),fullHeight));}
 void apply(int h){
  adjusting=true;
  split->setSizes({qMax(0,available()-h),qMax(0,h)});
  adjusting=false;place();
 }
public:
 InspectorDrawer(QWidget*w,QSplitter*s):QWidget(s),content(w),split(s){
  content->setParent(this);content->show();hide();
  setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  animation.setEasingCurve(QEasingCurve::OutCubic);
  connect(&animation,&QVariantAnimation::valueChanged,this,[this](const QVariant&v){apply(v.toInt());});
  connect(&animation,&QVariantAnimation::finished,this,[this]{finish();});
  connect(s,&QSplitter::splitterMoved,this,[this]{if(!adjusting){if(moving())finish();rememberSize();}});
 }
 QSize sizeHint()const override{return content->sizeHint();}
 QSize minimumSizeHint()const override{return {content->minimumSizeHint().width(),0};}
 bool isOpen()const{return wanted;}
 bool moving()const{return animation.state()==QAbstractAnimation::Running;}
 void rememberSize(){if(wanted&&!moving()&&height()>0)remembered=height();}
 void finish(){
  animation.stop();
  if(wanted){
   show();if(fullHeight>0)apply(preferred());
   fullHeight=0;setMinimumHeight(qMin(qMax(1,content->minimumSizeHint().height()),qMax(1,available()-100)));place();
  }else{hide();fullHeight=0;setMinimumHeight(0);}
  content->setEnabled(wanted);
 }
 void setOpen(bool open,int ms){
  if(wanted==open){if(ms<=0&&moving())finish();return;}
  const bool wasMoving=moving();
  if(wanted&&!wasMoving)rememberSize();
  const int start=isHidden()?0:height();
  wanted=open;animation.stop();
  setMinimumHeight(0);fullHeight=qMax(start,preferred());
  if(!open)content->setEnabled(false);else content->setEnabled(true);
  if(ms<=0||!split->isVisible()){finish();return;}
  show();apply(start);
  const int end=open?preferred():0;
  if(start==end){finish();return;}
  animation.setDuration(ms);animation.setStartValue(start);animation.setEndValue(end);animation.start();
 }
protected:
 void resizeEvent(QResizeEvent*e)override{QWidget::resizeEvent(e);place();}
};
