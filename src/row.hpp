#pragma once
#include "context-appearance.hpp"
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
#include "scene-palette.hpp"
#include "motion.hpp"

enum class TypographyRole {Scene,Source,Status,Tab};
inline QFont rowTypography(QFont font,TypographyRole role){
 font.setWeight(role==TypographyRole::Scene?QFont::DemiBold:role==TypographyRole::Tab?QFont::Medium:QFont::Normal);
 const qreal scale=role==TypographyRole::Scene?1.15:role==TypographyRole::Status?.90:1.;
 if(font.pointSizeF()>0)font.setPointSizeF(font.pointSizeF()*scale);
 else if(font.pixelSize()>0)font.setPixelSize(qMax(1,qRound(font.pixelSize()*scale)));
 return font;
}
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
class SourceArtworkLabel:public QLabel{
 bool dimmed=false;
public:
 explicit SourceArtworkLabel(QWidget*parent):QLabel(parent){}
 void setDimmed(bool value){if(dimmed!=value){dimmed=value;update();}}
protected:
 void paintEvent(QPaintEvent*e)override{
  const auto art=pixmap();
  if(!dimmed||art.isNull()){QLabel::paintEvent(e);return;}
  QPainter painter(this);painter.setOpacity(.72);
  const auto size=art.deviceIndependentSize();
  const auto area=contentsRect();
  painter.drawPixmap(QPointF(area.center().x()+.5-size.width()/2,area.center().y()+.5-size.height()/2),art);
 }
};
class Row : public QWidget {
public:
 ElidedLabel *name=nullptr;
 SourceArtworkLabel *thumb;
 QWidget *secondary;
 QVector<QToolButton*> hoverButtons, secondaryButtons;
 std::function<void()> onBody,onAppearance,onDisclosure;
 std::function<bool(QPoint)> onDragMove;
 std::function<void(QPoint)> onDrop;
 std::function<void()> onDragCancel;
 void cancelGesture(){pressed=dragging=false;}
 void setDropIndicator(int edge){if(dropEdge!=edge){dropEdge=edge;update();}}
 void setStripe(const unscene::RowStripes &value,bool enabled,bool odd){
  if(stripes==value&&striped==enabled&&alternateRow==odd)return;
  stripes=value;striped=enabled;alternateRow=odd;applyInk();update();
 }
 QColor idleSurface()const{
  const auto pal=nativeList?nativeList->palette():palette();
  const auto original=isScene?roles.header:branchEnabled?roles.container:pal.color(QPalette::Base);
  return striped?stripes.color(pal,original,alternateRow,accentColor.isValid()||branchColor.isValid()):original;
 }
 void setNote(const QString&text,std::function<void()> edit){
  noteAction=std::move(edit);
  if(!noteBadge&&!text.isEmpty()){
   noteBadge=new GlyphButton("note",top);noteBadge->setObjectName("unscene-note");noteBadge->setAccessibleName("Edit private note");
   first->insertWidget(first->indexOf(name)+1,noteBadge);
   connect(noteBadge,&QToolButton::clicked,this,[this]{if(noteAction)noteAction();});
   styleControl(noteBadge);
  }
  if(noteBadge){
   const auto tip=unscene::noteTooltip(text);if(noteBadge->toolTip()!=tip)noteBadge->setToolTip(tip);
   noteBadge->setVisible(!text.isEmpty());
  }
 }
 void setAccent(const QColor &color){if(accentColor!=color){accentColor=color;refreshRoles();applyInk();update();}}
 QColor accent()const{return accentColor;}
 void setCardHeader(bool joined){
  if(cardJoined==joined)return;
  cardJoined=joined;update();
 }
 void setBranchColor(const QColor&color,bool first,bool last){
  if(branchColor==color&&branchFirst==first&&branchLast==last&&branchEnabled)return;
  branchColor=color;branchFirst=first;branchLast=last;branchEnabled=true;refreshRoles();applyInk();update();
 }
 Row(const QString &title,bool scene,bool open,int depth,QWidget *parent=nullptr):QWidget(parent),isScene(scene),nestDepth(depth){
  setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);setAttribute(Qt::WA_Hover);
  rowHover.setEasingCurve(QEasingCurve::OutCubic);
  connect(&rowHover,&QVariantAnimation::valueChanged,this,[this](const QVariant&v){hoverAmount=v.toReal();update();});
  auto *outer=new QVBoxLayout(this);outer->setContentsMargins(0,0,0,0);
  top=new QWidget(this);first=new QHBoxLayout(top);first->setContentsMargins(0,0,0,0);outer->addWidget(top);
  disclosure=new GlyphButton("right",top);disclosure->setFocusPolicy(Qt::NoFocus);disclosure->setCircular(true);
  connect(disclosure,&QToolButton::clicked,this,[this]{if(onDisclosure)onDisclosure();else if(onBody)onBody();});first->addWidget(disclosure);disclosure->hide();
  thumb=new SourceArtworkLabel(top);thumb->setFixedSize(scene?48:36,scene?27:22);thumb->setAlignment(Qt::AlignCenter);first->addWidget(thumb);
  name=new ElidedLabel(title,top);first->addWidget(name,1);
  name->installEventFilter(this);thumb->installEventFilter(this);top->installEventFilter(this);
  status=new ElidedLabel("",top);status->setObjectName("unscene-row-status");status->hide();first->addWidget(status);status->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
  secondary=new QWidget(this);extra=new QHBoxLayout(secondary);extra->setContentsMargins(0,0,0,0);extra->addStretch();secondaryReveal=new Reveal(secondary,this);outer->addWidget(secondaryReveal);
  if(isScene){
   // Include the header padding, status, and empty shortcut-row space;
   // real buttons own their own mouse events.
   installEventFilter(this);status->installEventFilter(this);
   secondary->installEventFilter(this);secondaryReveal->installEventFilter(this);
   setCursor(Qt::PointingHandCursor);
  }
  applyGeometry();refreshRoles();applyInk();setExpanded(open);
 }
 void setDisclosure(bool enabled,bool open){disclosure->setVisible(enabled);disclosure->setAngle(open?90:0);disclosure->setToolTip(open?"Collapse":"Expand");}
 void setSelected(bool value){selected=value;applyInk();update();}
 void setSourceVisible(bool visible){
  const bool dim=!isScene&&!visible;
  if(dim==sourceDimmed)return;
  sourceDimmed=dim;thumb->setDimmed(dim);applyInk();update();
 }
 void setNativeList(QListView *v){if(nativeList!=v){nativeList=v;applyGeometry();refreshRoles();applyInk();update();}}
 void setStatus(const QString&s){status->setFullText(s);status->setFont(rowTypography(font(),TypographyRole::Status));status->setFixedWidth(status->fontMetrics().horizontalAdvance(s)+qBound(2,themeSpacing(this,Qt::Horizontal),6));status->setVisible(!s.isEmpty());status->setToolTip(s);}
 QColor inkColor()const{
  const auto fg=selected?contrastInk(palette().color(QPalette::Highlight),palette().color(QPalette::HighlightedText)):striped?contrastInk(idleSurface(),palette().color(QPalette::WindowText)):isScene?roles.onHeader:branchEnabled?roles.onContainer:palette().color(QPalette::WindowText);
  if(!sourceDimmed)return fg;
  const auto bg=selected?palette().color(QPalette::Highlight):idleSurface();
  // Keep hidden text readable; reduce the dimming if the palette is already
  // near the normal-text contrast threshold.
  for(double amount:{.20,.15,.10,.05}){
   const auto dim=mixColor(fg,bg,amount);
   if(surfaceContrast(dim,bg)>=4.5)return dim;
  }
  return fg;
 }
 void setMotionDuration(int ms){duration=ms;if(!ms){rowHover.stop();hoverAmount=underMouse()?1:0;update();}if(!ms){secondaryReveal->finish();}for(auto*b:hoverButtons)static_cast<GlyphButton*>(b)->setMotionDuration(ms);for(auto*b:secondaryButtons)static_cast<GlyphButton*>(b)->setMotionDuration(ms);disclosure->setMotionDuration(ms);}
 bool motionActive()const{return secondaryReveal->moving();}
 void setExpanded(bool value){opened=value;secondaryReveal->setOpen(value&&(!isScene||!secondaryButtons.isEmpty()),duration);}
 void settleMotion(){secondaryReveal->finish();}
 bool expanded()const{return opened;}
 void addHover(const QString &tip,const QString&icon,std::function<void()> action,bool always=false){
  auto *b=makeButton(tip,icon,std::move(action));if(icon=="arrow-right"||icon=="live"||icon=="plus")static_cast<GlyphButton*>(b)->setCircular(true);if(activationButton)first->insertWidget(first->indexOf(activationButton),b);else first->addWidget(b);hoverButtons.append(b);
  b->setProperty("always",always);auto p=b->sizePolicy();p.setRetainSizeWhenHidden(true);b->setSizePolicy(p);static_cast<GlyphButton*>(b)->revealAction(always||underMouse());
 }
 void addSceneActivation(bool live,bool studio,bool preview,std::function<void()> action){
  const bool statusOnly=live&&(!studio||preview);
  const auto tip=live?(statusOnly?"Live scene":"Live scene — select for preview"):(studio?"Select preview scene":"Activate scene");
  addHover(tip,live?"live":"arrow-right",[statusOnly,action=std::move(action)]{if(!statusOnly&&action)action();},live);
  activationButton=hoverButtons.last();
  if(statusOnly)static_cast<GlyphButton*>(activationButton)->setIndicatorOnly(true);
 }
 void addSecondary(const QString &tip,const QString&icon,std::function<void()> action){
  auto *b=makeButton(tip,icon,std::move(action));extra->addWidget(b);secondaryButtons.append(b);
 }
protected:
 void paintEvent(QPaintEvent*event) override{
  QWidget::paintEvent(event);QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
  const auto h=themeSpacing(this,Qt::Horizontal),v=themeSpacing(this,Qt::Vertical);
  const qreal radius=qBound(8,h+v/2+3,14);
  const int edge=qBound(1,h/3,3);
  const bool moving=parentWidget()&&parentWidget()->property("unsceneReordering").toBool();
  const bool joined=isScene&&cardJoined&&!moving;
  const bool inCard=!isScene&&branchEnabled&&!moving;
  const QRectF surface=QRectF(rect()).adjusted(edge,isScene?2:0,-edge,
      isScene?(joined?0:-2):(inCard&&branchLast?-2:0));
  const auto pal=nativeList?nativeList->palette():palette();
  // Build the contiguous card using rounded outer ends and square joins.
  // The interior source bands span the same card edges as the header.
  auto pathFor=[radius](const QRectF&r,bool roundTop,bool roundBottom){
   QPainterPath path;path.addRoundedRect(r,radius,radius);
   if(!roundTop)path.addRect(QRectF(r.left(),r.top(),r.width(),qMin(radius,r.height())));
   if(!roundBottom)path.addRect(QRectF(r.left(),qMax(r.top(),r.bottom()-radius),r.width(),qMin(radius,r.height())));
   path.setFillRule(Qt::WindingFill);return path;
  };
  const auto branch=pathFor(surface,!inCard||branchFirst,!inCard||branchLast);
  const auto base=pal.color(QPalette::Base);
  if(moving)p.fillRect(rect(),base);
  if(!isScene&&branchEnabled)p.fillPath(branch,roles.container);
  const QRectF rowSurface=isScene?surface:surface.adjusted(1,1,-1,-1);
  const auto rounded=isScene?pathFor(rowSurface,true,!joined):pathFor(rowSurface,true,true);
  if(!selected&&(isScene||striped))
   p.fillPath(isScene?rounded:inCard?branch:rounded,idleSurface());
  // Draw the theme selection as an antialiased path, rather than clipping a
  // square native item painter (raster clip edges can look visibly jagged).
  if(selected)p.fillPath(rounded,palette().color(QPalette::Highlight));
  else if(hoverAmount>0&&!isScene&&!branchEnabled&&!accentColor.isValid()){
   auto layer=pal.color(QPalette::Highlight);layer.setAlphaF(.14*hoverAmount);p.fillPath(rounded,layer);
  }
  if(!selected&&hoverAmount>0&&(isScene||branchEnabled||accentColor.isValid())){
   auto layer=inkColor();layer.setAlphaF(.08*hoverAmount);p.fillPath(rounded,layer);
  }
  if(accentColor.isValid()){
   // Source accents remain distinct without recoloring the whole scene surface.
   auto indicatorColor=accentRoles.accent;
   if(selected)indicatorColor=contrastInk(palette().color(QPalette::Highlight),indicatorColor,3);
   else if(isScene||branchEnabled||striped)indicatorColor=contrastInk(idleSurface(),indicatorColor,3);
   p.setPen(Qt::NoPen);p.setBrush(indicatorColor);
   const qreal barWidth=qMax(2,h/2);
   const QRectF indicator(rowSurface.left()+2,rowSurface.top()+4,barWidth,qMax(0.0,rowSurface.height()-8));
   p.drawRoundedRect(indicator,barWidth/2,barWidth/2);
  }
  if(dropEdge){QPen pen(palette().color(QPalette::Highlight));pen.setWidth(2);p.setPen(pen);if(dropEdge==2){p.setBrush(Qt::NoBrush);p.drawRoundedRect(rowSurface.adjusted(1,1,-1,-1),radius,radius);}else{int y=dropEdge<0?1:height()-2;p.drawLine(outerLeft(),y,width()-2,y);}}
  if(opened&&!secondaryButtons.isEmpty()){auto divider=inkColor();divider.setAlpha(32);p.setPen(divider);const int y=secondaryReveal->geometry().top()-1;const int left=qMax(outerLeft(),qCeil(rowSurface.left()+radius/2));const int right=qFloor(rowSurface.right()-radius/2);if(left<right)p.drawLine(left,y,right,y);}
 }
 void contextMenuEvent(QContextMenuEvent*e)override{if(onAppearance){onAppearance();e->accept();}else QWidget::contextMenuEvent(e);}
 void enterEvent(QEnterEvent *e)override{animateHover(1);for(auto*b:hoverButtons)static_cast<GlyphButton*>(b)->revealAction(true);update();QWidget::enterEvent(e);}
 void leaveEvent(QEvent *e)override{animateHover(0);for(auto*b:hoverButtons)static_cast<GlyphButton*>(b)->revealAction(b->property("always").toBool());update();QWidget::leaveEvent(e);}
 void changeEvent(QEvent*e)override{QWidget::changeEvent(e);if(ready&&(e->type()==QEvent::PaletteChange||e->type()==QEvent::StyleChange||e->type()==QEvent::FontChange)){applyGeometry();refreshRoles();applyInk();}}
 bool eventFilter(QObject *o,QEvent *e)override{
  if(e->type()==QEvent::ContextMenu&&onAppearance){onAppearance();return true;}
  if(e->type()==QEvent::MouseButtonPress){
   auto*m=static_cast<QMouseEvent*>(e);if(m->button()==Qt::LeftButton){
    // An indicator-only button can ignore a mouse event, which Qt then
    // propagates to its parent. A click on that button is not a header click.
    for(auto*w=childAt(mapFromGlobal(m->globalPosition().toPoint()));w&&w!=this;w=w->parentWidget())
     if(qobject_cast<QAbstractButton*>(w))return true;
    pressed=true;dragging=false;pressPoint=m->globalPosition().toPoint();return true;
   }
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
   else if(wasPressed&&rect().contains(mapFromGlobal(m->globalPosition().toPoint()))&&onBody)onBody();return true;
  }
  if(e->type()==QEvent::UngrabMouse&&pressed){pressed=dragging=false;if(onDragCancel)onDragCancel();}
  return QWidget::eventFilter(o,e);
 }
private:
 int dropEdge=0;bool pressed=false,dragging=false;QPoint pressPoint;
 bool isScene,opened=false,selected=false,sourceDimmed=false;int nestDepth=0;bool ready=false;QWidget*top=nullptr;QColor accentColor,branchColor;bool branchEnabled=false,branchFirst=true,branchLast=true,cardJoined=false; int duration=0;Reveal*secondaryReveal;
 SceneSurfaceRoles roles,accentRoles;
 unscene::RowStripes stripes;bool striped=false,alternateRow=false;GlyphButton*noteBadge=nullptr;std::function<void()> noteAction;
 void refreshRoles(){const auto pal=nativeList?nativeList->palette():palette();roles=sceneSurfaceRoles(pal,isScene?accentColor:branchColor);accentRoles=sceneSurfaceRoles(pal,accentColor);}
 QVariantAnimation rowHover;qreal hoverAmount=0;
 void animateHover(qreal target){rowHover.stop();if(!duration){hoverAmount=target;update();return;}rowHover.setDuration(duration);rowHover.setStartValue(hoverAmount);rowHover.setEndValue(target);rowHover.start();}
 QHBoxLayout *first,*extra;GlyphButton*disclosure;QToolButton*activationButton=nullptr;ElidedLabel*status;QPointer<QListView>nativeList;
 int outerLeft()const{return layout()->contentsMargins().left();}
 void applyGeometry(){
  name->setFont(rowTypography(font(),isScene?TypographyRole::Scene:TypographyRole::Source));
  status->setFont(rowTypography(font(),TypographyRole::Status));
  const auto h=themeSpacing(this,Qt::Horizontal),v=themeSpacing(this,Qt::Vertical);
  auto*view=nativeList?static_cast<QWidget*>(nativeList.data()):this;
  const QSize content(name->fontMetrics().horizontalAdvance("M"),name->fontMetrics().height());
  const auto native=nativeItemSize(view,content);
  const auto insetX=qBound(3,(native.width()-content.width())/2,6);
  const auto insetY=qBound(1,(native.height()-content.height())/4,2);
  const int indent=qMax(10,disclosure->sizeHint().width()*2/3)+qBound(1,h/2,4);
  layout()->setContentsMargins(insetX+nestDepth*indent,insetY,insetX,insetY);
  layout()->setSpacing(qBound(0,v/3,2));first->setSpacing(qBound(2,h/2,4));extra->setSpacing(qBound(1,h/3,3));
  // Preserve explicit theme minimums while removing redundant content padding.
  const int themeFloor=nativeItemSize(view,QSize(0,0)).height();
  top->setMinimumHeight(qMax(name->fontMetrics().height(),themeFloor-2*insetY));
  ready=true;updateGeometry();
 }
 void styleControl(GlyphButton*b){
  const auto bg=selected?palette().color(QPalette::Highlight):idleSurface();
  const auto fg=selected?contrastInk(palette().color(QPalette::Highlight),palette().color(QPalette::HighlightedText)):striped?contrastInk(idleSurface(),palette().color(QPalette::WindowText)):isScene?roles.onHeader:branchEnabled?roles.onContainer:palette().color(QPalette::WindowText);
  b->setTonalColors(bg,fg);
 }
 void applyInk(){
  const auto fg=inkColor();
  for(auto*l:{name,status}){auto p=l->palette();p.setColor(QPalette::WindowText,fg);p.setColor(QPalette::Text,fg);l->setPalette(p);l->setInk(fg);}
  styleControl(disclosure);if(noteBadge)styleControl(noteBadge);for(auto*b:hoverButtons)styleControl(static_cast<GlyphButton*>(b));for(auto*b:secondaryButtons)styleControl(static_cast<GlyphButton*>(b));
 }
 QToolButton*makeButton(const QString&tip,const QString&icon,std::function<void()>action){
  auto*b=new GlyphButton(icon,this);styleControl(b);b->setMotionDuration(duration);b->setToolTip(tip);b->setAccessibleName(tip);connect(b,&QToolButton::clicked,this,[a=std::move(action)]{a();});return b;
 }
};
