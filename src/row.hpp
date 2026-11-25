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
#include <functional>
#include "theme.hpp"

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
 void setAccent(const QColor &color){accentColor=color;update();}
 QColor accent()const{return accentColor;}
 Row(const QString &title,bool scene,bool open,int depth,QWidget *parent=nullptr):QWidget(parent),isScene(scene){
  setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);setAttribute(Qt::WA_Hover);
  auto *outer=new QVBoxLayout(this);outer->setContentsMargins(5+(depth?32+(depth-1)*16:0),2,4,2);outer->setSpacing(2);
  auto *top=new QWidget(this);first=new QHBoxLayout(top);first->setContentsMargins(0,0,0,0);first->setSpacing(5);outer->addWidget(top);
  disclosure=new GlyphButton("right",top);disclosure->setFixedSize(13,22);disclosure->setIconSize(QSize(12,12));disclosure->setFocusPolicy(Qt::NoFocus);
  connect(disclosure,&QToolButton::clicked,this,[this]{if(onBody)onBody();});first->addWidget(disclosure);disclosure->hide();
  thumb=new QLabel(top);thumb->setFixedSize(scene?48:36,scene?27:22);thumb->setAlignment(Qt::AlignCenter);first->addWidget(thumb);
  name=new ElidedLabel(title,top);if(scene){auto f=name->font();f.setWeight(QFont::DemiBold);name->setFont(f);}first->addWidget(name,1);
  name->installEventFilter(this);thumb->installEventFilter(this);top->installEventFilter(this);
  status=new ElidedLabel("",top);status->hide();first->addWidget(status);
  secondary=new QWidget(this);extra=new QHBoxLayout(secondary);extra->setContentsMargins(0,0,0,0);extra->setSpacing(4);extra->addStretch();outer->addWidget(secondary);
  setExpanded(open);
 }
 void setDisclosure(bool enabled,bool open){disclosure->setVisible(enabled);disclosure->setGlyph(open?"down":"right");disclosure->setToolTip(open?"Collapse":"Expand");}
 void setSelected(bool value){selected=value;applyInk();update();}
 void setNativeList(QListView *v){nativeList=v;update();}
 void setStatus(const QString&s){status->setFullText(s);auto f=status->font();if(f.pointSizeF()>2)f.setPointSizeF(f.pointSizeF()*.82);status->setFont(f);status->setFixedWidth(status->fontMetrics().horizontalAdvance(s)+16);status->setVisible(!s.isEmpty());status->setToolTip(s);}
 QColor inkColor()const{return palette().color(selected?QPalette::HighlightedText:QPalette::WindowText);}
 void setExpanded(bool value){opened=value;secondary->setVisible(value&&!isScene);}
 bool expanded()const{return opened;}
 void addHover(const QString &tip,const QString&icon,std::function<void()> action,bool always=false){
  auto *b=makeButton(tip,icon,std::move(action));first->addWidget(b);hoverButtons.append(b);
  b->setProperty("always",always);auto p=b->sizePolicy();p.setRetainSizeWhenHidden(true);b->setSizePolicy(p);b->setVisible(always||underMouse());
 }
 void addSecondary(const QString &tip,const QString&icon,std::function<void()> action){
  auto *b=makeButton(tip,icon,std::move(action));extra->addWidget(b);secondaryButtons.append(b);
 }
protected:
 void paintEvent(QPaintEvent*event) override{
  QWidget::paintEvent(event);QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
  const auto surface=rect().adjusted(1,0,-1,0);
  if(selected||underMouse()){
   if(nativeList){QStyleOptionViewItem opt;opt.initFrom(nativeList);opt.rect=surface;opt.state=QStyle::State_Enabled|QStyle::State_Active|(selected?QStyle::State_Selected:QStyle::State_None);if(underMouse()&&!selected)opt.state|=QStyle::State_MouseOver;opt.showDecorationSelected=true;nativeList->style()->drawControl(QStyle::CE_ItemViewItem,&opt,&p,nativeList);}
   else{p.setPen(Qt::NoPen);p.setBrush(selected?palette().highlight():QBrush(mixColor(palette().color(QPalette::Base),palette().color(QPalette::WindowText),.09)));p.drawRoundedRect(surface,4,4);}
  }
  if(accentColor.isValid()){
   auto tint=accentColor;tint.setAlpha(selected?22:38);p.setPen(Qt::NoPen);p.setBrush(tint);p.drawRoundedRect(surface,4,4);
   p.setBrush(accentColor);p.drawRoundedRect(QRect(2,3,3,qMax(0,height()-6)),1,1);
  }
  if(opened&&!isScene){auto divider=inkColor();divider.setAlpha(32);p.setPen(divider);const int y=secondary->geometry().top()-1;p.drawLine(outerLeft(),y,width()-5,y);}
 }
 void contextMenuEvent(QContextMenuEvent*e)override{if(onAppearance){onAppearance();e->accept();}else QWidget::contextMenuEvent(e);}
 void enterEvent(QEnterEvent *e)override{for(auto*b:hoverButtons)b->show();update();QWidget::enterEvent(e);}
 void leaveEvent(QEvent *e)override{for(auto*b:hoverButtons)b->setVisible(b->property("always").toBool());update();QWidget::leaveEvent(e);}
 void changeEvent(QEvent*e)override{QWidget::changeEvent(e);if(name&&(e->type()==QEvent::PaletteChange||e->type()==QEvent::StyleChange))applyInk();}
 bool eventFilter(QObject *o,QEvent *e)override{
  if(e->type()==QEvent::ContextMenu&&onAppearance){onAppearance();return true;}
  if(e->type()==QEvent::MouseButtonRelease&&static_cast<QMouseEvent*>(e)->button()==Qt::LeftButton){if(onBody)onBody();return true;}
  return QWidget::eventFilter(o,e);
 }
private:
 bool isScene,opened=false,selected=false;QColor accentColor;
 QHBoxLayout *first,*extra;GlyphButton*disclosure;ElidedLabel*status;QPointer<QListView>nativeList;
 int outerLeft()const{return layout()->contentsMargins().left();}
 void applyInk(){
  const auto fg=inkColor();
  for(auto*l:{name,status}){auto p=l->palette();p.setColor(QPalette::WindowText,fg);p.setColor(QPalette::Text,fg);l->setPalette(p);l->setInk(fg);}
  disclosure->setInk(fg);for(auto*b:hoverButtons)static_cast<GlyphButton*>(b)->setInk(fg);for(auto*b:secondaryButtons)static_cast<GlyphButton*>(b)->setInk(fg);
 }
 QToolButton*makeButton(const QString&tip,const QString&icon,std::function<void()>action){
  auto*b=new GlyphButton(icon,this);b->setInk(inkColor());b->setToolTip(tip);b->setAccessibleName(tip);connect(b,&QToolButton::clicked,this,[a=std::move(action)]{a();});return b;
 }
};
