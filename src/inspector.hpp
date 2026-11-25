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
class Inspector:public QWidget{
 PresetStore store;
 QLabel *title,*notice;
 QWidget*form;
 QDoubleSpinBox *posX,*posY,*scaleX,*scaleY,*rot,*boundsX,*boundsY;
 QSpinBox *left,*top,*right,*bottom;
 QComboBox *align,*boundsType,*boundsAlign,*presets;
 QCheckBox *cropBounds,*pin;
 QPushButton *applyButton,*discardButton,*savePreset,*loadPreset,*removePreset;
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
  const bool changed=dirty();form->setEnabled(valid&&!locked);
  applyButton->setEnabled(valid&&!locked&&changed);discardButton->setEnabled(valid&&changed);
  notice->setText(!valid?"Select a source":locked?"Item is locked":changed?"Unapplied changes — Apply or Discard":"Edits affect only this scene item");
  savePreset->setEnabled(valid);loadPreset->setEnabled(valid&&!locked&&presets->count());removePreset->setEnabled(presets->count());
 }
 void reloadPresets(const QString&name={}){
  presets->clear();presets->addItems(store.names());if(!name.isEmpty())presets->setCurrentText(name);updateActions();
 }
 QDoubleSpinBox*number(const QString&name,double minimum=-1e8){
  auto*b=new QDoubleSpinBox(form);b->setObjectName(name);b->setDecimals(6);b->setRange(minimum,1e8);b->setKeyboardTracking(true);connect(b,&QDoubleSpinBox::valueChanged,this,[this]{updateActions();});return b;
 }
 QSpinBox*crop(){auto*b=new QSpinBox(form);b->setRange(0,100000000);b->setKeyboardTracking(true);connect(b,&QSpinBox::valueChanged,this,[this]{updateActions();});return b;}
 QComboBox*alignment(){
  auto*b=new QComboBox(form);
  for(auto p:QList<QPair<QString,int>>{{"Top left",5},{"Top",4},{"Top right",6},{"Left",1},{"Center",0},{"Right",2},{"Bottom left",9},{"Bottom",8},{"Bottom right",10}})b->addItem(p.first,p.second);
  connect(b,&QComboBox::currentIndexChanged,this,[this]{updateActions();});return b;
 }
 static QWidget*pair(QWidget*a,QWidget*b){auto*w=new QWidget;auto*l=new QHBoxLayout(w);l->setContentsMargins(0,0,0,0);l->addWidget(a);l->addWidget(b);return w;}
public:
 std::function<void()> onClose,onApply,onDiscard,onProperties,onFilters;
 explicit Inspector(const QString&path,QWidget*parent=nullptr):QWidget(parent),store(path){
  auto*l=new QVBoxLayout(this);
  auto*header=new QHBoxLayout;title=new QLabel("Inspector",this);title->setTextFormat(Qt::PlainText);title->setWordWrap(true);header->addWidget(title,1);
  pin=new QCheckBox("Pin",this);header->addWidget(pin);
  auto*close=new QPushButton("Close",this);header->addWidget(close);l->addLayout(header);
  connect(close,&QPushButton::clicked,this,[this]{if(onClose)onClose();});
  auto*links=new QHBoxLayout;auto*properties=new QPushButton("Properties…",this);auto*filters=new QPushButton("Filters…",this);links->addWidget(properties);links->addWidget(filters);l->addLayout(links);
  connect(properties,&QPushButton::clicked,this,[this]{if(onProperties)onProperties();});connect(filters,&QPushButton::clicked,this,[this]{if(onFilters)onFilters();});
  auto*area=new QScrollArea(this);area->setWidgetResizable(true);area->setFrameShape(QFrame::NoFrame);form=new QWidget;auto*f=new QFormLayout(form);f->setContentsMargins(0,0,0,0);
  posX=number("positionX");posY=number("positionY");scaleX=number("scaleX");scaleY=number("scaleY");rot=number("rotation");
  boundsX=number("boundsX",0);boundsY=number("boundsY",0);left=crop();top=crop();right=crop();bottom=crop();align=alignment();boundsAlign=alignment();
  boundsType=new QComboBox(form);for(auto s:{"None","Stretch","Scale inner","Scale outer","Scale to width","Scale to height","Maximum size"})boundsType->addItem(s,boundsType->count());
  connect(boundsType,&QComboBox::currentIndexChanged,this,[this]{updateActions();});cropBounds=new QCheckBox("Crop to bounds",form);connect(cropBounds,&QCheckBox::toggled,this,[this]{updateActions();});
  f->addRow("Position X / Y",pair(posX,posY));f->addRow("Scale X / Y",pair(scaleX,scaleY));f->addRow("Rotation",rot);f->addRow("Alignment",align);f->addRow("Bounds",boundsType);f->addRow("Bounds W / H",pair(boundsX,boundsY));f->addRow("Bounds alignment",boundsAlign);f->addRow("",cropBounds);f->addRow("Crop L / T",pair(left,top));f->addRow("Crop R / B",pair(right,bottom));
  area->setWidget(form);area->setMinimumHeight(120);l->addWidget(area,1);
  auto*pr=new QHBoxLayout;presets=new QComboBox(this);presets->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);presets->setMinimumContentsLength(6);pr->addWidget(presets,1);
  loadPreset=new QPushButton("Load",this);savePreset=new QPushButton("Save…",this);removePreset=new QPushButton("Delete",this);pr->addWidget(loadPreset);pr->addWidget(savePreset);pr->addWidget(removePreset);l->addLayout(pr);
  notice=new QLabel(this);notice->setWordWrap(true);l->addWidget(notice);
  auto*actions=new QHBoxLayout;discardButton=new QPushButton("Discard",this);applyButton=new QPushButton("Apply",this);actions->addStretch();actions->addWidget(discardButton);actions->addWidget(applyButton);l->addLayout(actions);
  connect(applyButton,&QPushButton::clicked,this,[this]{if(onApply)onApply();});
  connect(discardButton,&QPushButton::clicked,this,[this]{if(onDiscard)onDiscard();});
  connect(loadPreset,&QPushButton::clicked,this,[this]{if(auto t=store.load(presets->currentText()))controls(*t);});
  connect(savePreset,&QPushButton::clicked,this,[this]{
   bool ok=false;auto name=QInputDialog::getText(this,"Save transform preset","Name",QLineEdit::Normal,presets->currentText(),&ok).trimmed();if(!ok||name.isEmpty())return;
   if(store.load(name)&&QMessageBox::question(this,"Replace preset","Replace existing preset?",QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)!=QMessageBox::Yes)return;
   if(!store.save(name,draft()))QMessageBox::warning(this,"Unscene","Could not save preset.");reloadPresets(name);
  });
  connect(removePreset,&QPushButton::clicked,this,[this]{auto name=presets->currentText();if(!name.isEmpty()&&QMessageBox::question(this,"Delete preset","Delete this preset?",QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)==QMessageBox::Yes){store.remove(name);reloadPresets();}});
  reloadPresets();
 }
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
  valid=true;locked=isLocked;title->setText(name+" — "+scene);controls(t);base=t;displayBase=rawDraft();updateActions();
 }
 void setLocked(bool value){locked=value;updateActions();}
 void invalidate(){valid=false;unpin();updateActions();}
};
