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
#include "context-appearance.hpp"
#include <functional>
struct FilterEntry{
 QString id,name,type;bool enabled=false,async=false;QString note;
 bool operator==(const FilterEntry&b)const{return id==b.id&&name==b.name&&type==b.type&&enabled==b.enabled&&async==b.async&&note==b.note;}
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
  auto*l=new QVBoxLayout(this);l->setContentsMargins(2,2,2,2);l->setSpacing(3);
  auto*hint=new QLabel("Shared across source instances.",this);hint->setWordWrap(true);l->addWidget(hint);
  list=new QListWidget(this);list->setObjectName("filterStack");list->setSelectionMode(QAbstractItemView::SingleSelection);l->addWidget(list,1);
  auto*bar=new QHBoxLayout;bar->setContentsMargins(0,0,0,0);bar->setSpacing(2);
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
  auto*settings=add("Edit filter settings","settings","edit",true);settings->setObjectName("filterEdit");
  auto*transfer=new GlyphButton("more",this);transfer->setObjectName("filterMore");transfer->setToolTip("Filter actions — rename, copy, paste, import, export");transfer->setAccessibleName("More filter actions");transfer->setPopupMode(QToolButton::InstantPopup);bar->addWidget(transfer);
  auto*menu=new QMenu(transfer);transfer->setMenu(menu);
  auto action=[&](const QString&label,const QString&op,bool selected,bool any){
   auto*a=menu->addAction(label);
   connect(menu,&QMenu::aboutToShow,this,[this,a,selected,any]{a->setEnabled((!selected||!selectedId().isEmpty())&&(!any||!entries.isEmpty()));});
   connect(a,&QAction::triggered,this,[this,op]{if(onAction)onAction(op,selectedId());});
  };
  action("Rename filter…","rename",true,false);action("Add / edit note…","note",true,false);menu->addSeparator();
  action("Copy filter","copy",true,true);action("Copy filter stack","copy-stack",false,true);
  action("Paste filters…","paste",false,false);menu->addSeparator();
  action("Export filter…","export",true,true);action("Export filter stack…","export-stack",false,true);
  action("Import filters…","import",false,false);
  l->addLayout(bar);
  connect(list,&QListWidget::currentRowChanged,this,[this]{updateButtons();});
  connect(list->model(),&QAbstractItemModel::dataChanged,this,[this](const QModelIndex&first,const QModelIndex&last,const QList<int>&roles){
   if(list->signalsBlocked()||!onAction||!roles.contains(Qt::CheckStateRole))return;
   // Dispatch only check-state edits, not tooltip/name/icon updates.
   for(int row=first.row();row<=last.row()&&row<entries.size();++row){
    auto*i=list->item(row);if(!i)continue;
    auto id=i->data(Qt::UserRole).toString();bool enabled=i->checkState()==Qt::Checked;
    if(id==entries[row].id)onAction(enabled?"enable":"disable",id);
   }
  });
  updateButtons();
 }
 QString selectedId()const{return list->currentItem()?list->currentItem()->data(Qt::UserRole).toString():QString();}
 void setEntries(QVector<FilterEntry> next){
  if(entries==next)return;
  const auto selected=selectedId();QSignalBlocker guard(list);entries=std::move(next);list->clear();
  for(auto&e:entries){auto*i=new QListWidgetItem(e.name+(e.enabled?"":" (disabled)"),list);i->setData(Qt::UserRole,e.id);i->setToolTip(e.note.isEmpty()?e.type+(e.async?" — Audio/video":" — Effect"):unscene::noteTooltip(e.note));if(!e.note.isEmpty())i->setIcon(QIcon(glyphPixmap("note",palette().color(QPalette::Text),16,devicePixelRatioF())));i->setFlags(i->flags()|Qt::ItemIsUserCheckable);i->setCheckState(e.enabled?Qt::Checked:Qt::Unchecked);if(e.id==selected)list->setCurrentItem(i);}
  if(!list->currentItem()&&list->count())list->setCurrentRow(0);updateButtons();
 }
};
