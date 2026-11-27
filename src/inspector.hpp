#pragma once
#include <obs.h>
#include <QWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QSignalBlocker>
#include <optional>
#include <functional>
#include <cmath>
#include <QTabWidget>
#include <QVariantAnimation>
#include <QGraphicsOpacityEffect>
#include <QPointer>
#include "filter-panel.hpp"
#include "row.hpp"
#include <QStyleOptionSpinBox>

struct TransformState {
 obs_transform_info info{};
 obs_sceneitem_crop crop{};
 TransformState(){info.scale.x=info.scale.y=1;info.alignment=OBS_ALIGN_LEFT|OBS_ALIGN_TOP;}
 QByteArray encode()const{
  return QJsonDocument(QJsonArray{info.pos.x,info.pos.y,info.scale.x,info.scale.y,info.rot,int(info.alignment),int(info.bounds_type),int(info.bounds_alignment),info.bounds.x,info.bounds.y,info.crop_to_bounds,crop.left,crop.top,crop.right,crop.bottom}).toJson(QJsonDocument::Compact);
 }
 static std::optional<TransformState> decode(const QByteArray&bytes){
  auto doc=QJsonDocument::fromJson(bytes);if(!doc.isArray())return {};
  auto a=doc.array();if(a.size()!=15||!a[10].isBool())return {};
  for(int i=0;i<a.size();++i)if(i!=10&&(!a[i].isDouble()||!std::isfinite(a[i].toDouble())||qAbs(a[i].toDouble())>1e8))return {};
  for(int i:{5,6,7,11,12,13,14})if(a[i].toDouble()!=a[i].toInt())return {};
  auto validAlign=[](int x){return x==0||x==1||x==2||x==4||x==8||x==5||x==6||x==9||x==10;};
  if(!validAlign(a[5].toInt())||!validAlign(a[7].toInt())||a[6].toInt()<0||a[6].toInt()>OBS_BOUNDS_MAX_ONLY||a[8].toDouble()<0||a[9].toDouble()<0)return {};
  for(int i=11;i<15;++i)if(a[i].toInt()<0)return {};
  TransformState t;auto&v=t.info;
  v.pos.x=a[0].toDouble();v.pos.y=a[1].toDouble();v.scale.x=a[2].toDouble();v.scale.y=a[3].toDouble();v.rot=a[4].toDouble();
  v.alignment=a[5].toInt();v.bounds_type=static_cast<obs_bounds_type>(a[6].toInt());v.bounds_alignment=a[7].toInt();v.bounds.x=a[8].toDouble();v.bounds.y=a[9].toDouble();v.crop_to_bounds=a[10].toBool();
  t.crop.left=a[11].toInt();t.crop.top=a[12].toInt();t.crop.right=a[13].toInt();t.crop.bottom=a[14].toInt();return t;
 }
 static TransformState read(obs_sceneitem_t*i){TransformState t;obs_sceneitem_get_info2(i,&t.info);obs_sceneitem_get_crop(i,&t.crop);return t;}
 void apply(obs_sceneitem_t*i)const{obs_sceneitem_defer_update_begin(i);obs_sceneitem_set_info2(i,&info);obs_sceneitem_set_crop(i,&crop);obs_sceneitem_defer_update_end(i);}
};
class PresetStore{
 QSettings settings;
 static QString key(const QString&name){return "presets/"+QString::fromLatin1(name.toUtf8().toHex());}
public:
 explicit PresetStore(const QString&p):settings(p,QSettings::IniFormat){}
 QStringList names(){settings.sync();settings.beginGroup("presets");QStringList list;for(auto k:settings.childKeys())list<<QString::fromUtf8(QByteArray::fromHex(k.toLatin1()));settings.endGroup();list.sort(Qt::CaseInsensitive);return list;}
 std::optional<TransformState> load(const QString&name){settings.sync();return TransformState::decode(settings.value(key(name)).toByteArray());}
 bool save(const QString&name,const TransformState&t){if(name.trimmed().isEmpty()||name.size()>128||!TransformState::decode(t.encode()))return false;settings.setValue(key(name),t.encode());settings.sync();return settings.status()==QSettings::NoError;}
 bool remove(const QString&name){settings.remove(key(name));settings.sync();return settings.status()==QSettings::NoError;}
};
// Keep the full numeric range and precision without letting the longest
// possible number dictate the minimum width of the entire dock.
template<class Spin> class InspectorNumber:public Spin{
public:
 explicit InspectorNumber(QWidget*p):Spin(p){this->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);}
 QSize minimumSizeHint()const override{
  QStyleOptionSpinBox opt;this->initStyleOption(&opt);
  const auto fm=this->fontMetrics();
  return this->style()->sizeFromContents(QStyle::CT_SpinBox,&opt,QSize(fm.horizontalAdvance("-000.0"),fm.height()),this);
 }
 QSize sizeHint()const override{return minimumSizeHint();}
};
class InspectorDouble:public InspectorNumber<QDoubleSpinBox>{
public:using InspectorNumber::InspectorNumber;
protected:
 QString textFromValue(double value)const override{
  // Only presentation changes: value(), draft precision and input range
  // stay identical to the previous six-decimal editor.
  auto text=QDoubleSpinBox::textFromValue(value);
  const auto decimal=locale().decimalPoint(),zero=locale().zeroDigit();
  if(text.contains(decimal)){while(text.endsWith(zero))text.chop(zero.size());if(text.endsWith(decimal))text.chop(decimal.size());}
  return text;
 }
};
class Inspector:public QWidget{
 PresetStore store;
 QVariantAnimation tabMotion;
 QPointer<QWidget> fadingPage;
 int motionDuration=0;
 void fadeTab(){
  finishMotion();
  auto*page=tabs->currentWidget();if(!motionDuration||!isVisible()||!page||page->graphicsEffect())return;
  fadingPage=page;auto*effect=new QGraphicsOpacityEffect(page);effect->setOpacity(.7);page->setGraphicsEffect(effect);
  tabMotion.setDuration(motionDuration);tabMotion.setStartValue(.7);tabMotion.setEndValue(1.0);tabMotion.start();
 }
 QTabWidget*tabs=nullptr;FilterPanel*filterPanel=nullptr;QListWidget*usage;
 ElidedLabel *title;QLabel*notice;
 QWidget*form;
 QDoubleSpinBox *posX,*posY,*scaleX,*scaleY,*rot,*boundsX,*boundsY;
 QSpinBox *left,*top,*right,*bottom;
 QComboBox *align,*boundsType,*boundsAlign,*presets;
 QCheckBox *cropBounds;GlyphButton*pin;
 QPushButton *applyButton,*discardButton;GlyphButton *savePreset,*loadPreset,*removePreset;
 TransformState base,displayBase;
 bool loading=false,locked=false,valid=false;
 void controls(const TransformState&t){
  loading=true;
  posX->setValue(t.info.pos.x);posY->setValue(t.info.pos.y);scaleX->setValue(t.info.scale.x);scaleY->setValue(t.info.scale.y);rot->setValue(t.info.rot);
  align->setCurrentIndex(align->findData(int(t.info.alignment)));boundsType->setCurrentIndex(boundsType->findData(int(t.info.bounds_type)));boundsAlign->setCurrentIndex(boundsAlign->findData(int(t.info.bounds_alignment)));
  boundsX->setValue(t.info.bounds.x);boundsY->setValue(t.info.bounds.y);cropBounds->setChecked(t.info.crop_to_bounds);left->setValue(t.crop.left);top->setValue(t.crop.top);right->setValue(t.crop.right);bottom->setValue(t.crop.bottom);
  loading=false;updateActions();
 }
 void updateActions(){
  if(loading)return;
  auto*f=static_cast<QFormLayout*>(form->layout());
  const bool bounded=boundsType->currentData().toInt()!=OBS_BOUNDS_NONE;
  for(auto*w:{boundsX->parentWidget(),static_cast<QWidget*>(boundsAlign),static_cast<QWidget*>(cropBounds)})
   if(f->isRowVisible(w)!=bounded)f->setRowVisible(w,bounded);
  const bool changed=dirty();form->setEnabled(valid&&!locked);
  applyButton->setEnabled(valid&&!locked&&changed);discardButton->setEnabled(valid&&changed);
  notice->setText(!valid?"Select a source":locked?"Item is locked":changed?"Unapplied changes":"This item only");
  savePreset->setEnabled(valid);loadPreset->setEnabled(valid&&!locked&&presets->count());removePreset->setEnabled(presets->count());
 }
 void reloadPresets(const QString&name={}){
  presets->clear();presets->addItems(store.names());if(!name.isEmpty())presets->setCurrentText(name);updateActions();
 }
 QDoubleSpinBox*number(const QString&name,double minimum=-1e8){
  auto*b=new InspectorDouble(form);b->setObjectName(name);b->setDecimals(6);b->setRange(minimum,1e8);b->setKeyboardTracking(true);connect(b,&QDoubleSpinBox::valueChanged,this,[this]{updateActions();});return b;
 }
 QSpinBox*crop(){auto*b=new InspectorNumber<QSpinBox>(form);b->setRange(0,100000000);b->setKeyboardTracking(true);connect(b,&QSpinBox::valueChanged,this,[this]{updateActions();});return b;}
 static void compactCombo(QComboBox*b){
  b->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);b->setMinimumContentsLength(6);b->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
  auto update=[b]{b->setToolTip(b->currentText());};QObject::connect(b,&QComboBox::currentTextChanged,b,update);update();
 }
 static GlyphButton*iconAction(QWidget*parent,const QString&id,const QString&name,const QString&label){
  auto*b=new GlyphButton(id,parent);b->setObjectName(name);b->setToolTip(label);b->setAccessibleName(label);return b;
 }
 static void describe(QWidget*w,const QString&name){w->setAccessibleName(name);w->setToolTip(name);}
 void compactSpacing(){
  const int h=qBound(2,themeSpacing(this,Qt::Horizontal)/2,4),v=qBound(1,themeSpacing(this,Qt::Vertical)/2,3);
  layout()->setContentsMargins(h,v,h,v);
  auto layouts=findChildren<QLayout*>();for(auto*l:layouts){
   auto*w=l->parentWidget();if(w&&(w==filterPanel||filterPanel->isAncestorOf(w)||qobject_cast<QAbstractSpinBox*>(w)||qobject_cast<QComboBox*>(w)||qobject_cast<QAbstractScrollArea*>(w)))continue;
   if(qobject_cast<QFormLayout*>(l)){auto*f=static_cast<QFormLayout*>(l);f->setHorizontalSpacing(h);f->setVerticalSpacing(v);}
   else l->setSpacing(v);
  }
 }
 QComboBox*alignment(){
  auto*b=new QComboBox(form);
  for(auto p:QList<QPair<QString,int>>{{"Top left",5},{"Top",4},{"Top right",6},{"Left",1},{"Center",0},{"Right",2},{"Bottom left",9},{"Bottom",8},{"Bottom right",10}})b->addItem(p.first,p.second);
  compactCombo(b);connect(b,&QComboBox::currentIndexChanged,this,[this]{updateActions();});return b;
 }
 static QWidget*pair(QWidget*a,QWidget*b){auto*w=new QWidget;auto*l=new QHBoxLayout(w);l->setContentsMargins(0,0,0,0);l->setSpacing(3);l->addWidget(a,1);l->addWidget(b,1);return w;}
public:
 std::function<void()> onClose,onApply,onDiscard,onProperties,onFilters,onUsageRefresh;
 std::function<void(QString,QString)> onFilterAction;
 explicit Inspector(const QString&path,QWidget*parent=nullptr):QWidget(parent),store(path){
  auto*l=new QVBoxLayout(this);
  auto*header=new QHBoxLayout;title=new ElidedLabel("Inspector",this);auto heading=title->font();heading.setWeight(QFont::DemiBold);title->setFont(heading);header->addWidget(title,1);
  auto*properties=new GlyphButton("properties",this);properties->setObjectName("inspectorProperties");properties->setToolTip("Source properties");properties->setAccessibleName("Source properties");header->addWidget(properties);
  auto*filters=new GlyphButton("filters",this);filters->setObjectName("inspectorFilters");filters->setToolTip("Native source filters");filters->setAccessibleName("Native source filters");header->addWidget(filters);
  pin=iconAction(this,"pin","inspectorPin","Pin inspected source");pin->setCheckable(true);header->addWidget(pin);
  connect(pin,&QToolButton::toggled,this,[this](bool on){pin->setGlyph(on?"pin-filled":"pin");pin->setToolTip(on?"Unpin inspected source":"Pin inspected source");pin->setAccessibleName(pin->toolTip());});
  auto*close=iconAction(this,"close","inspectorClose","Close inspector");header->addWidget(close);l->addLayout(header);
  connect(close,&QToolButton::clicked,this,[this]{if(onClose)onClose();});
  connect(properties,&QToolButton::clicked,this,[this]{if(onProperties)onProperties();});connect(filters,&QToolButton::clicked,this,[this]{if(onFilters)onFilters();});
  tabs=new QTabWidget(this);l->addWidget(tabs,1);auto*transformPage=new QWidget(tabs);auto*tl=new QVBoxLayout(transformPage);tl->setContentsMargins(0,0,0,0);tabs->addTab(transformPage,"Transform");
  auto*area=new QScrollArea(this);area->setObjectName("transformScroll");area->setWidgetResizable(true);area->setFrameShape(QFrame::NoFrame);form=new QWidget;auto*f=new QFormLayout(form);f->setContentsMargins(2,2,2,2);f->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);f->setRowWrapPolicy(QFormLayout::WrapLongRows);
  posX=number("positionX");posY=number("positionY");scaleX=number("scaleX");scaleY=number("scaleY");rot=number("rotation");
  boundsX=number("boundsX",0);boundsY=number("boundsY",0);left=crop();top=crop();right=crop();bottom=crop();align=alignment();boundsAlign=alignment();
  boundsType=new QComboBox(form);boundsType->setObjectName("boundsType");for(auto s:{"None","Stretch","Scale inner","Scale outer","Scale to width","Scale to height","Maximum size"})boundsType->addItem(s,boundsType->count());
  connect(boundsType,&QComboBox::currentIndexChanged,this,[this]{updateActions();});cropBounds=new QCheckBox("Crop to bounds",form);connect(cropBounds,&QCheckBox::toggled,this,[this]{updateActions();});
  compactCombo(boundsType);
  describe(posX,"Position X");describe(posY,"Position Y");describe(scaleX,"Scale X");describe(scaleY,"Scale Y");
  describe(rot,"Rotation in degrees");describe(align,"Item alignment");describe(boundsType,"Bounding box type");
  describe(boundsX,"Bounds width");describe(boundsY,"Bounds height");describe(boundsAlign,"Bounds alignment");
  describe(left,"Crop left");describe(top,"Crop top");describe(right,"Crop right");describe(bottom,"Crop bottom");
  f->addRow("Position X/Y",pair(posX,posY));f->addRow("Scale X/Y",pair(scaleX,scaleY));f->addRow("Rotation",rot);f->addRow("Alignment",align);f->addRow("Bounds",boundsType);f->addRow("Size W/H",pair(boundsX,boundsY));f->addRow("Bounds align",boundsAlign);f->addRow("",cropBounds);f->addRow("Crop L/T",pair(left,top));f->addRow("Crop R/B",pair(right,bottom));
  area->setWidget(form);area->setMinimumHeight(84);tl->addWidget(area,1);
  auto*pr=new QHBoxLayout;presets=new QComboBox(this);presets->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);presets->setMinimumContentsLength(6);pr->addWidget(presets,1);
  presets->setToolTip("Transform presets");presets->setAccessibleName("Transform presets");
  loadPreset=iconAction(this,"load","inspectorLoadPreset","Load transform preset");savePreset=iconAction(this,"save","inspectorSavePreset","Save transform preset");removePreset=iconAction(this,"trash","inspectorRemovePreset","Delete transform preset");pr->addWidget(loadPreset);pr->addWidget(savePreset);pr->addWidget(removePreset);tl->addLayout(pr);
  notice=new QLabel(this);notice->setWordWrap(true);notice->setMinimumWidth(0);notice->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
  auto*actions=new QHBoxLayout;discardButton=new QPushButton("Discard",this);applyButton=new QPushButton("Apply",this);actions->addWidget(notice,1);actions->addWidget(discardButton);actions->addWidget(applyButton);tl->addLayout(actions);
  connect(applyButton,&QPushButton::clicked,this,[this]{if(onApply)onApply();});
  connect(discardButton,&QPushButton::clicked,this,[this]{if(onDiscard)onDiscard();});
  connect(loadPreset,&QToolButton::clicked,this,[this]{if(auto t=store.load(presets->currentText()))controls(*t);});
  connect(savePreset,&QToolButton::clicked,this,[this]{
   bool ok=false;auto name=QInputDialog::getText(this,"Save transform preset","Name",QLineEdit::Normal,presets->currentText(),&ok).trimmed();if(!ok||name.isEmpty())return;
   if(store.load(name)&&QMessageBox::question(this,"Replace preset","Replace existing preset?",QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)!=QMessageBox::Yes)return;
   if(!store.save(name,draft()))QMessageBox::warning(this,"Unscene","Could not save preset.");reloadPresets(name);
  });
  connect(removePreset,&QToolButton::clicked,this,[this]{auto name=presets->currentText();if(!name.isEmpty()&&QMessageBox::question(this,"Delete preset","Delete this preset?",QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)==QMessageBox::Yes){store.remove(name);reloadPresets();}});
  filterPanel=new FilterPanel(tabs);tabs->addTab(filterPanel,"Filters");
  filterPanel->onAction=[this](QString a,QString id){if(onFilterAction)onFilterAction(a,id);};
  auto*usagePage=new QWidget(tabs);auto*ul=new QVBoxLayout(usagePage);ul->setContentsMargins(0,0,0,0);
  auto*usageHeader=new QHBoxLayout;auto*hint=new QLabel("Direct scene/group instances",usagePage);hint->setWordWrap(true);hint->setToolTip("Includes collapsed branches; excludes indirect nested scenes.");usageHeader->addWidget(hint,1);
  auto*refresh=iconAction(usagePage,"refresh","inspectorRefreshUsage","Refresh usage");usageHeader->addWidget(refresh);ul->addLayout(usageHeader);
  usage=new QListWidget(usagePage);ul->addWidget(usage,1);
  tabs->addTab(usagePage,"Usage");
  connect(refresh,&QToolButton::clicked,this,[this]{if(onUsageRefresh)onUsageRefresh();});
  tabMotion.setEasingCurve(QEasingCurve::OutCubic);
  connect(&tabMotion,&QVariantAnimation::valueChanged,this,[this](const QVariant&v){if(fadingPage)if(auto*effect=qobject_cast<QGraphicsOpacityEffect*>(fadingPage->graphicsEffect()))effect->setOpacity(v.toReal());});
  connect(&tabMotion,&QVariantAnimation::finished,this,[this]{finishMotion();});
  connect(tabs,&QTabWidget::currentChanged,this,[this](int){if(onUsageRefresh)onUsageRefresh();fadeTab();});
  reloadPresets();compactSpacing();
 }
protected:
 void changeEvent(QEvent*e)override{QWidget::changeEvent(e);if(tabs&&filterPanel&&(e->type()==QEvent::StyleChange||e->type()==QEvent::FontChange))compactSpacing();}
public:
 void finishMotion(){tabMotion.stop();if(fadingPage){fadingPage->setGraphicsEffect(nullptr);fadingPage=nullptr;}}
 void setMotionDuration(int ms){motionDuration=ms;for(auto*b:findChildren<QToolButton*>())if(auto*g=dynamic_cast<GlyphButton*>(b))g->setMotionDuration(ms);if(!ms)finishMotion();}
 bool motionActive()const{return tabMotion.state()==QAbstractAnimation::Running;}
 void showTransform(){if(tabs->isTabEnabled(0))tabs->setCurrentIndex(0);}
 void setVisual(bool visual){tabs->setTabEnabled(0,visual);if(!visual&&tabs->currentIndex()==0)tabs->setCurrentIndex(1);}
 bool filtersVisible()const{return tabs->currentIndex()==1;}
 bool usageVisible()const{return tabs->currentIndex()==2;}
 void setFilters(const QVector<FilterEntry>&entries){filterPanel->setEntries(entries);}
 void setUsage(const QStringList&lines){usage->clear();usage->addItems(lines);}
 bool pinned()const{return pin->isChecked();}
 void unpin(){pin->setChecked(false);}
 TransformState original()const{return base;}
 TransformState rawDraft()const{
  TransformState t=base;auto&v=t.info;v.pos.x=posX->value();v.pos.y=posY->value();v.scale.x=scaleX->value();v.scale.y=scaleY->value();v.rot=rot->value();
  v.alignment=align->currentData().toInt();v.bounds_type=static_cast<obs_bounds_type>(boundsType->currentData().toInt());v.bounds_alignment=boundsAlign->currentData().toInt();v.bounds.x=boundsX->value();v.bounds.y=boundsY->value();v.crop_to_bounds=cropBounds->isChecked();t.crop.left=left->value();t.crop.top=top->value();t.crop.right=right->value();t.crop.bottom=bottom->value();return t;
 }
 TransformState draft()const{
  auto raw=QJsonDocument::fromJson(rawDraft().encode()).array();
  auto display=QJsonDocument::fromJson(displayBase.encode()).array();
  auto exact=QJsonDocument::fromJson(base.encode()).array();
  for(int i=0;i<raw.size();++i)if(raw[i]==display[i])raw[i]=exact[i];
  return TransformState::decode(QJsonDocument(raw).toJson(QJsonDocument::Compact)).value_or(rawDraft());
 }
 bool dirty()const{return valid&&draft().encode()!=base.encode();}
 void load(const QString&name,const QString&scene,const TransformState&t,bool isLocked){
  valid=true;locked=isLocked;title->setFullText(name);title->setToolTip(name+" — "+scene);title->setAccessibleName(name+" — "+scene);controls(t);base=t;displayBase=rawDraft();updateActions();
 }
 void setLocked(bool value){locked=value;updateActions();}
 void invalidate(){valid=false;unpin();updateActions();}
};
