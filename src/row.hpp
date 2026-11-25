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
#include <functional>

class Row : public QWidget {
public:
 QLabel *name, *thumb;
 QWidget *secondary;
 QVector<QToolButton*> hoverButtons, secondaryButtons;
 std::function<void()> onBody;
 Row(const QString &title,bool scene,bool open,int depth,QWidget *parent=nullptr):QWidget(parent),isScene(scene) {
  auto *outer=new QVBoxLayout(this); outer->setContentsMargins(depth*12+2,1,2,1); outer->setSpacing(1);
  auto *top=new QWidget(this); first=new QHBoxLayout(top); first->setContentsMargins(0,0,0,0); first->setSpacing(3); outer->addWidget(top);
  thumb=new QLabel(top); thumb->setFixedSize(scene?48:36,scene?27:22);
  thumb->setAlignment(Qt::AlignCenter); first->addWidget(thumb);
  name=new QLabel(title,top); name->setToolTip(title); name->setMinimumWidth(0); name->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred); first->addWidget(name,1);
  name->installEventFilter(this); thumb->installEventFilter(this);
  secondary=new QWidget(this); extra=new QHBoxLayout(secondary); extra->setContentsMargins(0,0,0,0); extra->setSpacing(3); extra->addStretch(); outer->addWidget(secondary);
  setExpanded(open);
 }
 void setExpanded(bool value) { opened=value; secondary->setVisible(value&&!isScene); }
 bool expanded() const {return opened;}
 void addHover(const QString &tip,QStyle::StandardPixmap icon,std::function<void()> action,bool always=false) {
  auto *b=makeButton(tip,icon,std::move(action)); first->addWidget(b); hoverButtons.append(b);
  b->setProperty("always",always); auto p=b->sizePolicy(); p.setRetainSizeWhenHidden(true); b->setSizePolicy(p); b->setVisible(always||underMouse());
 }
 void addSecondary(const QString &tip,QStyle::StandardPixmap icon,std::function<void()> action) {
  auto *b=makeButton(tip,icon,std::move(action)); extra->addWidget(b); secondaryButtons.append(b);
 }
protected:
 void enterEvent(QEnterEvent *e) override {for(auto*b:hoverButtons)b->show(); QWidget::enterEvent(e);}
 void leaveEvent(QEvent *e) override {for(auto*b:hoverButtons)b->setVisible(b->property("always").toBool()); QWidget::leaveEvent(e);}
 bool eventFilter(QObject *o,QEvent *e) override {
  if((o==name||o==thumb)&&e->type()==QEvent::MouseButtonRelease&&static_cast<QMouseEvent*>(e)->button()==Qt::LeftButton) {if(onBody)onBody(); return true;}
  return QWidget::eventFilter(o,e);
 }
private:
 bool isScene, opened=false;
 QHBoxLayout *first,*extra;
 QToolButton* makeButton(const QString &tip,QStyle::StandardPixmap icon,std::function<void()> action) {
  auto*b=new QToolButton(this); b->setAutoRaise(true); b->setIcon(style()->standardIcon(icon)); b->setIconSize(QSize(15,15)); b->setFixedSize(23,23); b->setToolTip(tip); b->setAccessibleName(tip);
  connect(b,&QToolButton::clicked,this,[a=std::move(action)]{a();}); return b;
 }
};
