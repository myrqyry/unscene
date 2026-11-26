#pragma once
#include <obs.h>
#include <QWidget>
#include <QListWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QStyle>
#include <QMenu>
#include "theme.hpp"
#include <functional>
struct FilterEntry{
 QString id,name,type;bool enabled=false,async=false;
 bool operator==(const FilterEntry&b)const{return id==b.id&&name==b.name&&type==b.type&&enabled==b.enabled&&async==b.async;}
};
inline bool filterCompatible(uint32_t source,uint32_t filter){
 if(filter&(OBS_SOURCE_DEPRECATED|OBS_SOURCE_CAP_DISABLED))return false;
 if((filter&OBS_SOURCE_VIDEO)&&!(source&OBS_SOURCE_VIDEO))return false;
 if((filter&OBS_SOURCE_AUDIO)&&!(source&OBS_SOURCE_AUDIO))return false;
 if((filter&OBS_SOURCE_ASYNC)&&!(source&OBS_SOURCE_ASYNC)&&!(filter&OBS_SOURCE_AUDIO))return false;
 return true;
}
class FilterPanel:public QWidget{
 QListWidget*list;QVector<FilterEntry> entries;
 QVector<QToolButton*> selectionButtons;
 QToolButton*up,*down;GlyphButton*toggle;
 void updateButtons(){
  int row=list->currentRow();for(auto*b:selectionButtons)b->setEnabled(row>=0&&row<entries.size());
  bool before=false,after=false;
  if(row>=0&&row<entries.size())for(int i=0;i<entries.size();++i)if(entries[i].async==entries[row].async){before|=i<row;after|=i>row;}
  up->setEnabled(before);down->setEnabled(after);
  bool has=row>=0&&row<entries.size();
  bool enabled=has&&entries[row].enabled;
  toggle->setEnabled(has);toggle->setGlyph(enabled?"eye":"eye-off");
  toggle->setToolTip(enabled?"Disable filter":"Enable filter");toggle->setAccessibleName(toggle->toolTip());
 }
public:
 std::function<void(QString,QString)> onAction;
 explicit FilterPanel(QWidget*p=nullptr):QWidget(p){
  auto*l=new QVBoxLayout(this);l->setContentsMargins(0,0,0,0);
  auto*hint=new QLabel("Filters are shared by every instance of this source.",this);hint->setWordWrap(true);l->addWidget(hint);
  list=new QListWidget(this);list->setObjectName("filterStack");list->setSelectionMode(QAbstractItemView::SingleSelection);l->addWidget(list,1);
  auto*bar=new QHBoxLayout;bar->setContentsMargins(0,0,0,0);
  auto add=[&](const QString&label,const QString&action,const QString& icon,bool needsSelection){
   auto*b=new GlyphButton(icon,this);b->setToolTip(label);b->setAccessibleName(label);bar->addWidget(b);
   if(needsSelection)selectionButtons.append(b);
   connect(b,&QToolButton::clicked,this,[this,action]{if(onAction)onAction(action,selectedId());});return b;
  };
  add("Add filter","add","plus",false);
  add("Remove filter","remove","minus",true);
  up=add("Move up","up","up",true);down=add("Move down","down","down",true);
  toggle=new GlyphButton("eye",this);bar->addWidget(toggle);
  connect(toggle,&QToolButton::clicked,this,[this]{int row=list->currentRow();if(row>=0&&row<entries.size()&&onAction)onAction(entries[row].enabled?"disable":"enable",entries[row].id);});
  bar->addStretch();
  auto*more=new QToolButton(this);more->setText("Rename");bar->addWidget(more);selectionButtons.append(more);
  connect(more,&QToolButton::clicked,this,[this]{if(onAction)onAction("rename",selectedId());});
  auto*settings=new QToolButton(this);settings->setText("Edit…");bar->addWidget(settings);
  connect(settings,&QToolButton::clicked,this,[this]{if(onAction)onAction("settings",selectedId());});
  auto*transfer=new QToolButton(this);transfer->setText("Copy / Paste");transfer->setPopupMode(QToolButton::InstantPopup);
  auto*menu=new QMenu(transfer);transfer->setMenu(menu);
  auto action=[&](const QString&label,const QString&op,bool selected,bool any){
   auto*a=menu->addAction(label);
   connect(menu,&QMenu::aboutToShow,this,[this,a,selected,any]{a->setEnabled((!selected||!selectedId().isEmpty())&&(!any||!entries.isEmpty()));});
   connect(a,&QAction::triggered,this,[this,op]{if(onAction)onAction(op,selectedId());});
  };
  action("Copy filter","copy",true,true);action("Copy filter stack","copy-stack",false,true);
  action("Paste filters…","paste",false,false);menu->addSeparator();
  action("Export filter…","export",true,true);action("Export filter stack…","export-stack",false,true);
  action("Import filters…","import",false,false);
  l->addLayout(bar);l->addWidget(transfer,0,Qt::AlignRight);
  connect(list,&QListWidget::currentRowChanged,this,[this]{updateButtons();});
  connect(list,&QListWidget::itemChanged,this,[this](auto*i){if(onAction)onAction(i->checkState()==Qt::Checked?"enable":"disable",i->data(Qt::UserRole).toString());});
  updateButtons();
 }
 QString selectedId()const{return list->currentItem()?list->currentItem()->data(Qt::UserRole).toString():QString();}
 void setEntries(QVector<FilterEntry> next){
  if(entries==next)return;
  const auto selected=selectedId();QSignalBlocker guard(list);entries=std::move(next);list->clear();
  for(auto&e:entries){auto*i=new QListWidgetItem(e.name+(e.enabled?"":" (disabled)"),list);i->setData(Qt::UserRole,e.id);i->setToolTip(e.type+(e.async?" — Audio/video":" — Effect"));i->setFlags(i->flags()|Qt::ItemIsUserCheckable);i->setCheckState(e.enabled?Qt::Checked:Qt::Unchecked);if(e.id==selected)list->setCurrentItem(i);}
  if(!list->currentItem()&&list->count())list->setCurrentRow(0);updateButtons();
 }
};
