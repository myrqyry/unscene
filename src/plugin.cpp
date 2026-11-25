#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QPointer>
#include <QSet>
#include <QHash>
#include <QPushButton>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QMenu>
#include <memory>
#include "row.hpp"
#include "snapshot.hpp"
#include <QElapsedTimer>

OBS_DECLARE_MODULE()
MODULE_EXPORT const char *obs_module_name(void){return "SceneTree";}
MODULE_EXPORT const char *obs_module_description(void){return "Combined scene and source explorer";}
namespace {
using Source=std::unique_ptr<obs_source_t,decltype(&obs_source_release)>;
Source source(const QString &uuid){return Source(obs_get_source_by_uuid(uuid.toUtf8().constData()),obs_source_release);}
QString uuid(obs_source_t*s){return s?QString::fromUtf8(obs_source_get_uuid(s)):QString();}
struct Node {
 QString key,uid,parent,title; int64_t id=0; int depth=0;
 bool scene=false,group=false,visible=true,locked=false,program=false,preview=false,video=false,audio=false;
 bool operator==(const Node &b)const {return key==b.key&&uid==b.uid&&parent==b.parent&&title==b.title&&id==b.id&&depth==b.depth&&scene==b.scene&&group==b.group&&visible==b.visible&&locked==b.locked&&program==b.program&&preview==b.preview&&video==b.video&&audio==b.audio;}
};
Node makeNode(obs_source_t *s){Node n; n.uid=uuid(s); n.title=QString::fromUtf8(obs_source_get_name(s)); auto flags=obs_source_get_output_flags(s); n.video=flags&OBS_SOURCE_VIDEO; n.audio=flags&OBS_SOURCE_AUDIO; return n;}
class Dock : public QWidget {
 QVBoxLayout *rows; QScrollArea *scroll; QWidget *body; QTimer timer;
 QSet<QString> opened; QString selected,collection; QVector<Node> previous; bool initialized=false, pending=false, stopping=false;
 QHash<QString,QPixmap> thumbnails;
 QHash<QString,qint64> attempted;
 QHash<QString,QPointer<QLabel>> labels;
 QMultiHash<QString,QString> keys;
 QElapsedTimer clock;
 static constexpr int maxDepth=24;
 void updateSnapshots(){
  if(!isVisible())return;
  for(const auto&n:previous){
   auto label=labels.value(n.key);
   if(!n.video||!label||thumbnails.contains(n.uid)||!scroll->viewport()->rect().intersects(QRect(label->mapTo(scroll->viewport(),QPoint()),label->size())))continue;
   auto s=source(n.uid);if(!s||!obs_source_showing(s.get()))continue;
   if(attempted.contains(n.uid)&&clock.elapsed()-attempted.value(n.uid)<3000)continue;
   attempted.insert(n.uid,clock.elapsed());
   const auto image=captureSnapshot(s.get());if(image.isNull())return;
   if(thumbnails.size()>=512)thumbnails.clear();
   auto pix=QPixmap::fromImage(image);thumbnails.insert(n.uid,pix);
   for(const auto&key:keys.values(n.uid)){auto l=labels.value(key);if(l)l->setPixmap(pix.scaled(l->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));}
   return;
  }
 }
 void collect(obs_scene_t *scene,const QString &path,int depth,QVector<Node>&out) {
  if(!scene||depth>maxDepth)return;
  struct Context {QVector<Node> nodes; QString parent,path; int depth;};
  Context c{{},uuid(obs_scene_get_source(scene)),path,depth};
  obs_scene_enum_items(scene,[](obs_scene_t*,obs_sceneitem_t *item,void *p){
   auto &c=*static_cast<Context*>(p); auto *s=obs_sceneitem_get_source(item); if(!s)return true;
   auto n=makeNode(s); n.parent=c.parent; n.id=obs_sceneitem_get_id(item); n.key=c.path+"/"+QString::number(n.id)+":"+n.uid; n.depth=c.depth;
   n.visible=obs_sceneitem_visible(item); n.locked=obs_sceneitem_locked(item); n.group=obs_sceneitem_is_group(item); c.nodes.prepend(n); return true;
  },&c);
  for(const auto&n:c.nodes){
   out.append(n);
   if(n.group&&opened.contains(n.key)){auto s=source(n.uid); if(s)collect(obs_group_from_source(s.get()),n.key,depth+1,out);}
  }
 }
 template<class F> void itemAction(const Node&n,F f){
  auto p=source(n.parent); if(!p)return; auto *scene=obs_scene_from_source(p.get()); if(!scene)scene=obs_group_from_source(p.get()); if(!scene)return;
  auto *item=obs_scene_find_sceneitem_by_id(scene,n.id); if(!item||uuid(obs_sceneitem_get_source(item))!=n.uid)return;
  obs_sceneitem_addref(item); f(item); obs_sceneitem_release(item); request();
 }
 void request(){if(pending||stopping)return;pending=true;QTimer::singleShot(0,this,[this]{pending=false;refresh();});}
 void toggle(const Node &n){
  if(n.scene||n.group){if(opened.contains(n.key))opened.remove(n.key);else opened.insert(n.key);}
  if(!n.scene)selected=selected==n.key?QString():n.key;
  previous.clear();request();
 }
 void render(const QVector<Node>&nodes){
  const int pos=scroll->verticalScrollBar()->value();
  labels.clear();keys.clear();
  while(auto *it=rows->takeAt(0)){delete it->widget();delete it;}
  for(const auto&n:nodes){
   const bool isOpen=opened.contains(n.key), expanded=selected==n.key;
   QString title=n.title;
   if(n.scene||n.group)title=(isOpen?QStringLiteral("▾ "):QStringLiteral("▸ "))+title;
   if(n.program)title+=QStringLiteral("  • LIVE"); else if(n.preview)title+=QStringLiteral("  • PREVIEW");
   auto *r=new Row(title,n.scene,expanded,n.depth,body);
   auto ico=style()->standardIcon(n.scene?QStyle::SP_DirIcon:n.group?QStyle::SP_DirIcon:n.video?QStyle::SP_DesktopIcon:QStyle::SP_MediaVolume);
   r->thumb->setPixmap(ico.pixmap(18,18));
   labels.insert(n.key,r->thumb);keys.insert(n.uid,n.key);
   if(thumbnails.contains(n.uid))r->thumb->setPixmap(thumbnails.value(n.uid).scaled(r->thumb->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
   if(n.program||n.preview||expanded){r->setAutoFillBackground(true);r->setBackgroundRole(QPalette::AlternateBase);}
   r->onBody=[this,n]{toggle(n);};
   if(n.scene){
    r->addHover(obs_frontend_preview_program_mode_active()?"Select preview scene":"Activate scene",QStyle::SP_ArrowRight,[this,n]{auto s=source(n.uid);if(!s||!obs_scene_from_source(s.get()))return;if(obs_frontend_preview_program_mode_active())obs_frontend_set_current_preview_scene(s.get());else obs_frontend_set_current_scene(s.get());request();});
   }else{
    r->addHover(n.visible?"Hide source":"Show source",n.visible?QStyle::SP_DialogYesButton:QStyle::SP_DialogNoButton,[this,n]{itemAction(n,[](auto*i){obs_sceneitem_set_visible(i,!obs_sceneitem_visible(i));});},!n.visible);
    r->addHover(n.locked?"Unlock source":"Lock source",n.locked?QStyle::SP_DialogApplyButton:QStyle::SP_DialogResetButton,[this,n]{itemAction(n,[](auto*i){obs_sceneitem_set_locked(i,!obs_sceneitem_locked(i));});},n.locked);
    r->addHover("Properties",QStyle::SP_FileDialogDetailedView,[n]{auto s=source(n.uid);if(s)obs_frontend_open_source_properties(s.get());});
    r->addSecondary("Filters",QStyle::SP_FileDialogContentsView,[n]{auto s=source(n.uid);if(s)obs_frontend_open_source_filters(s.get());});
    r->addSecondary("Move up",QStyle::SP_ArrowUp,[this,n]{itemAction(n,[](auto*i){obs_sceneitem_set_order(i,OBS_ORDER_MOVE_UP);});});
    r->addSecondary("Move down",QStyle::SP_ArrowDown,[this,n]{itemAction(n,[](auto*i){obs_sceneitem_set_order(i,OBS_ORDER_MOVE_DOWN);});});
    if(n.audio)r->addSecondary("Toggle mute",QStyle::SP_MediaVolume,[n]{auto s=source(n.uid);if(s)obs_source_set_muted(s.get(),!obs_source_muted(s.get()));});
   }
   rows->addWidget(r);
  }
  rows->addStretch(); scroll->verticalScrollBar()->setValue(pos);
 }
public:
 Dock(){
  setObjectName("SceneTreeContent");
  auto*l=new QVBoxLayout(this);l->setContentsMargins(2,2,2,2);l->setSpacing(2);
  auto*bar=new QHBoxLayout; auto*collapse=new QToolButton(this);collapse->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));collapse->setToolTip("Collapse all scenes");
  auto*expand=new QToolButton(this);expand->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));expand->setToolTip("Expand all scenes");
  auto*refreshButton=new QToolButton(this);refreshButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));refreshButton->setToolTip("Refresh snapshots");
  connect(refreshButton,&QToolButton::clicked,this,[this]{thumbnails.clear();attempted.clear();previous.clear();request();});
  bar->addStretch();bar->addWidget(refreshButton);bar->addWidget(collapse);bar->addWidget(expand);l->addLayout(bar);
  scroll=new QScrollArea(this);scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);body=new QWidget;rows=new QVBoxLayout(body);rows->setContentsMargins(0,0,0,0);rows->setSpacing(1);scroll->setWidget(body);l->addWidget(scroll);
  connect(collapse,&QToolButton::clicked,this,[this]{opened.clear();selected.clear();previous.clear();request();});
  connect(expand,&QToolButton::clicked,this,[this]{for(auto &n:previous)if(n.scene)opened.insert(n.key);previous.clear();request();});
  connect(&timer,&QTimer::timeout,this,[this]{refresh();});clock.start();timer.start(400);
 }
 void stop(){stopping=true;timer.stop();}
 void refresh(){
  if(stopping||!isVisible())return;
  char *raw=obs_frontend_get_current_scene_collection();QString now=QString::fromUtf8(raw?raw:"");bfree(raw);
  if(collection!=now){collection=now;thumbnails.clear();attempted.clear();opened.clear();selected.clear();initialized=false;previous.clear();}
  Source live(obs_frontend_get_current_scene(),obs_source_release);
  Source preview(obs_frontend_preview_program_mode_active()?obs_frontend_get_current_preview_scene():nullptr,obs_source_release);
  if(!initialized){if(live)opened.insert(uuid(live.get()));initialized=true;}
  obs_frontend_source_list list={};obs_frontend_get_scenes(&list);
  QVector<Node> nodes;
  for(size_t i=0;i<list.sources.num;i++){
   auto*s=list.sources.array[i];auto n=makeNode(s);n.scene=true;n.key=n.uid;n.program=s==live.get();n.preview=s==preview.get();nodes.append(n);
   if(opened.contains(n.key))collect(obs_scene_from_source(s),n.key,1,nodes);
  }
  obs_frontend_source_list_free(&list);
  if(nodes!=previous){previous=nodes;render(nodes);}
  updateSnapshots();
 }
};
QPointer<Dock> dock;
void event(obs_frontend_event e,void*){if(e==OBS_FRONTEND_EVENT_EXIT&&dock)dock->stop();}
}
bool obs_module_load(void){dock=new Dock;if(!obs_frontend_add_dock_by_id("scene-tree.dock","Scenes & Sources",dock)){delete dock;return false;}obs_frontend_add_event_callback(event,nullptr);return true;}
void obs_module_unload(void){obs_frontend_remove_event_callback(event,nullptr);if(dock){dock->stop();obs_frontend_remove_dock("scene-tree.dock");}}
