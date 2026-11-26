#pragma once
#include <QVBoxLayout>
#include <QVariantAnimation>
#include <QPointer>
#include <QHash>
#include <QWidget>
#include <cmath>

// Animate real widgets from their previous positions to their layout positions.
// Layout remains authoritative; only a finite visual offset is applied.
class ReorderLayout:public QVBoxLayout{
 struct Motion {QPointer<QWidget> widget;QPoint target,offset;};
 QVector<Motion> motions;
 QVariantAnimation animation;
 qreal progress=0;
 void place(bool final=false){
  for(auto&m:motions)if(m.widget){
   const auto delta=final?QPoint():QPoint(qRound(m.offset.x()*(1-progress)),qRound(m.offset.y()*(1-progress)));
   m.widget->move(m.target+delta);
  }
 }
public:
 using Positions=QHash<QWidget*,QPoint>;
 explicit ReorderLayout(QWidget*parent):QVBoxLayout(parent){
  animation.setEasingCurve(QEasingCurve::OutCubic);
  connect(&animation,&QVariantAnimation::valueChanged,this,[this](const QVariant&v){progress=v.toReal();place();});
  connect(&animation,&QVariantAnimation::finished,this,[this]{finish();});
 }
 bool moving()const{return animation.state()==QAbstractAnimation::Running;}
 Positions positions()const{
  Positions result;for(int i=0;i<count();++i)if(auto*w=itemAt(i)->widget())result.insert(w,w->pos());
  return result;
 }
 void finish(){
  animation.stop();place(true);
  for(auto&m:motions)if(m.widget){m.widget->setProperty("unsceneReordering",false);m.widget->update();}
  motions.clear();progress=0;
 }
 void animateFrom(const Positions&origins,int duration,QWidget*preferred=nullptr){
  finish();if(duration<=0||!parentWidget()->isVisible())return;
  QWidget*leading=nullptr;int distance=0;
  for(int i=0;i<count();++i)if(auto*w=itemAt(i)->widget()){
   if(!origins.contains(w))continue;
   auto delta=origins.value(w)-w->pos();
   if(delta.isNull())continue;
   motions.append({w,w->pos(),delta});
   if(delta.manhattanLength()>distance){distance=delta.manhattanLength();leading=w;}
  }
  // Avoid many simultaneous widget moves on huge collections.
  if(motions.size()>32){motions.clear();return;}
  if(motions.isEmpty())return;
  for(auto&m:motions)if(m.widget){m.widget->setProperty("unsceneReordering",true);m.widget->update();if(m.widget==preferred)leading=preferred;}
  if(leading)leading->raise();
  place();
  animation.setDuration(duration);animation.setStartValue(0.0);animation.setEndValue(1.0);animation.start();
 }
 void setGeometry(const QRect&rect)override{
  if(motions.isEmpty()){QVBoxLayout::setGeometry(rect);return;}
  place(true);QVBoxLayout::setGeometry(rect);
  for(auto&m:motions)if(m.widget)m.target=m.widget->pos();
  place();
 }
};
