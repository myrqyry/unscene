#pragma once
#include <QTabBar>
#include <QMouseEvent>
#include <QPainter>
#include <QSignalBlocker>
#include <functional>

// Native scroll/reorder behavior with one commit per completed mouse gesture.
class SceneTabBar:public QTabBar {
 QStringList before;
 bool held=false;
 int target=-1;
public:
 explicit SceneTabBar(QWidget*parent=nullptr):QTabBar(parent){setMovable(true);}
 std::function<void(const QStringList&)> onOrder;
 bool gestureActive()const{return held;}
 QStringList rootOrder()const{
  QStringList ids;for(int i=0;i<count();++i){auto id=tabData(i).toString();if(!id.isEmpty())ids.append(id);}return ids;
 }
 void setDropTab(int i){if(target!=i){target=i;update();}}
protected:
 void mousePressEvent(QMouseEvent*e)override{
  if(e->button()==Qt::LeftButton){before=rootOrder();held=true;int i=tabAt(e->position().toPoint());setMovable(i>=0&&!tabData(i).toString().isEmpty());}
  QTabBar::mousePressEvent(e);
 }
 void mouseReleaseEvent(QMouseEvent*e)override{
  QTabBar::mouseReleaseEvent(e);
  if(e->button()!=Qt::LeftButton||!held)return;
  held=false;setMovable(true);
  {QSignalBlocker block(this);for(int i=1;i<count();++i)if(tabData(i).toString().isEmpty()){moveTab(i,0);break;}}
  const auto after=rootOrder();if(after!=before&&onOrder)onOrder(after);
 }
 void paintEvent(QPaintEvent*e)override{
  QTabBar::paintEvent(e);
  if(target<0||target>=count())return;
  QPainter p(this);QPen pen(palette().color(QPalette::Highlight));pen.setWidth(2);p.setPen(pen);p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(tabRect(target).adjusted(1,1,-2,-2),3,3);
 }
};
