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
#include "appearance-dialog.hpp"
#include <QDir>
#include <QMetaProperty>
#include <QIcon>
#include <QElapsedTimer>

OBS_DECLARE_MODULE()
MODULE_EXPORT const char *obs_module_name(void){return "Unscene";}
MODULE_EXPORT const char *obs_module_description(void){return "Combined scene and source explorer";}
namespace {
using Source=std::unique_ptr<obs_source_t,decltype(&obs_source_release)>;
Source source(const QString &uuid){return Source(obs_get_source_by_uuid(uuid.toUtf8().constData()),obs_source_release);}
QString uuid(obs_source_t*s){return s?QString::fromUtf8(obs_source_get_uuid(s)):QString();}
struct Node {
 QString key,uid,parent,title; int iconType=0; bool muted=false; int64_t id=0; int depth=0;
 bool scene=false,group=false,visible=true,locked=false,program=false,preview=false,video=false,audio=false;
 bool operator==(const Node &b)const {return key==b.key&&uid==b.uid&&parent==b.parent&&title==b.title&&iconType==b.iconType&&muted==b.muted&&id==b.id&&depth==b.depth&&scene==b.scene&&group==b.group&&visible==b.visible&&locked==b.locked&&program==b.program&&preview==b.preview&&video==b.video&&audio==b.audio;}
};
Node makeNode(obs_source_t *s){Node n; n.uid=uuid(s); n.title=QString::fromUtf8(obs_source_get_name(s)); auto flags=obs_source_get_output_flags(s); n.iconType=obs_source_get_icon_type(obs_source_get_id(s));n.muted=obs_source_muted(s);n.video=flags&OBS_SOURCE_VIDEO; n.audio=flags&OBS_SOURCE_AUDIO; return n;}
QString sourceGlyph(const Node&n){
 if(n.scene)return "layers";
 if(n.group)return "folder";
 switch(n.iconType){
 case OBS_ICON_TYPE_IMAGE:case OBS_ICON_TYPE_SLIDESHOW:return "image";
 case OBS_ICON_TYPE_COLOR:return "color";
 case OBS_ICON_TYPE_AUDIO_INPUT:return "mic";
 case OBS_ICON_TYPE_AUDIO_OUTPUT:case OBS_ICON_TYPE_PROCESS_AUDIO_OUTPUT:return "volume";
 case OBS_ICON_TYPE_DESKTOP_CAPTURE:case OBS_ICON_TYPE_WINDOW_CAPTURE:return "screen";
 case OBS_ICON_TYPE_GAME_CAPTURE:return "game";
 case OBS_ICON_TYPE_CAMERA:return "camera";
 case OBS_ICON_TYPE_TEXT:return "text";
 case OBS_ICON_TYPE_MEDIA:return "media";
 case OBS_ICON_TYPE_BROWSER:return "browser";
 default:return n.video?"screen":n.audio?"mic":"layers";
 }
}
const char* nativeIconProperty(const Node&n){
 if(n.scene)return "sceneIcon";
 if(n.group)return "groupIcon";
 switch(n.iconType){
 case OBS_ICON_TYPE_IMAGE:return "imageIcon";
 case OBS_ICON_TYPE_COLOR:return "colorIcon";
 case OBS_ICON_TYPE_SLIDESHOW:return "slideshowIcon";
 case OBS_ICON_TYPE_AUDIO_INPUT:return "audioInputIcon";
 case OBS_ICON_TYPE_AUDIO_OUTPUT:return "audioOutputIcon";
 case OBS_ICON_TYPE_PROCESS_AUDIO_OUTPUT:return "audioProcessOutputIcon";
 case OBS_ICON_TYPE_DESKTOP_CAPTURE:return "desktopCapIcon";
 case OBS_ICON_TYPE_WINDOW_CAPTURE:return "windowCapIcon";
 case OBS_ICON_TYPE_GAME_CAPTURE:return "gameCapIcon";
 case OBS_ICON_TYPE_CAMERA:return "cameraIcon";
 case OBS_ICON_TYPE_TEXT:return "textIcon";
 case OBS_ICON_TYPE_MEDIA:return "mediaIcon";
 case OBS_ICON_TYPE_BROWSER:return "browserIcon";
 default:return "defaultIcon";
 }
}
QPixmap sourceArtwork(const Node&n,Row*r){
 auto*window=static_cast<QWidget*>(obs_frontend_get_main_window());
 if(window){
  const auto index=window->metaObject()->indexOfProperty(nativeIconProperty(n));
  if(index>=0){
   const auto value=window->metaObject()->property(index).read(window);
   const auto icon=qvariant_cast<QIcon>(value);
   auto pix=icon.pixmap(QSize(18,18),r->devicePixelRatioF());
   if(!pix.isNull()){
    if(n.program||n.preview||r->expanded()){
     QPainter painter(&pix);painter.setCompositionMode(QPainter::CompositionMode_SourceIn);painter.fillRect(pix.rect(),r->inkColor());
    }
    return pix;
   }
  }
 }
 return glyphPixmap(sourceGlyph(n),r->inkColor(),18,r->devicePixelRatioF());
}
QPalette artworkPalette(Row*r){auto p=r->palette();p.setColor(QPalette::WindowText,r->inkColor());return p;}
QString appearancePath(){
 char* raw=obs_module_config_path("appearance.ini");
 const auto path=QString::fromUtf8(raw?raw:"");bfree(raw);
 return path;
}
class Dock : public QWidget {
 QListView *nativeList; QVBoxLayout *rows; QScrollArea *scroll; QWidget *body; QTimer timer;
 QSet<QString> opened; QString selected,collection; QVector<Node> previous; bool initialized=false, pending=false, stopping=false;
 QHash<QString,QPixmap> thumbnails;
 QHash<QString,qint64> attempted;
 QHash<QString,QPointer<QLabel>> labels;
 QMultiHash<QString,QString> keys;
 QElapsedTimer clock;
 AppearanceStore appearances{appearancePath()};
 QSet<QString> artworkOverrides;
 bool editingAppearance=false;
 void editAppearance(const Node& n){
  if(editingAppearance||stopping)return;
  editingAppearance=true;
  AppearanceDialog dialog(n.title,appearances.load(n.uid),this);
  if(dialog.exec()==QDialog::Accepted){
   if(!appearances.save(n.uid,dialog.value()))QMessageBox::warning(this,"Unscene","Could not save appearance settings.");
   previous.clear();
  }
  editingAppearance=false;request();
 }
 void queueAppearance(const Node&n){QTimer::singleShot(0,this,[this,n]{editAppearance(n);});}
 static constexpr int maxDepth=24;
 void updateSnapshots(){
  if(!isVisible())return;
  for(const auto&n:previous){
   auto label=labels.value(n.key);
   if(artworkOverrides.contains(n.uid)||!n.video||!label||thumbnails.contains(n.uid)||!scroll->viewport()->rect().intersects(QRect(label->mapTo(scroll->viewport(),QPoint()),label->size())))continue;
   auto s=source(n.uid);if(!s||!obs_source_showing(s.get()))continue;
   if(attempted.contains(n.uid)&&clock.elapsed()-attempted.value(n.uid)<3000)continue;
   attempted.insert(n.uid,clock.elapsed());
   const auto image=captureSnapshot(s.get());if(!hasVisiblePixels(image))return;
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
  labels.clear();keys.clear();artworkOverrides.clear();
  while(auto *it=rows->takeAt(0)){delete it->widget();delete it;}
  for(const auto&n:nodes){
   const bool isOpen=opened.contains(n.key), expanded=selected==n.key;
   auto *r=new Row(n.title,n.scene,expanded,n.depth,body);
   r->setNativeList(nativeList);
   r->setDisclosure(n.scene||n.group,isOpen);
   r->setSelected(n.program||n.preview||expanded);
   r->setStatus(n.program?"LIVE":n.preview?"PREVIEW":"");
   r->thumb->setPixmap(sourceArtwork(n,r));
   labels.insert(n.key,r->thumb);keys.insert(n.uid,n.key);
   if(thumbnails.contains(n.uid))r->thumb->setPixmap(thumbnails.value(n.uid).scaled(r->thumb->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
   const auto appearance=appearances.load(n.uid);
   const auto art=appearancePixmap(appearance,artworkPalette(r),r->thumb->size());
   if(!art.isNull()){artworkOverrides.insert(n.uid);r->thumb->setPixmap(art);}
   r->setAccent(appearance.color);
   r->onAppearance=[this,n]{queueAppearance(n);};
   r->onBody=[this,n]{toggle(n);};
   if(n.scene){
    r->addHover(obs_frontend_preview_program_mode_active()?"Select preview scene":"Activate scene","arrow-right",[this,n]{auto s=source(n.uid);if(!s||!obs_scene_from_source(s.get()))return;if(obs_frontend_preview_program_mode_active())obs_frontend_set_current_preview_scene(s.get());else obs_frontend_set_current_scene(s.get());request();});
    r->addHover("Appearance","appearance",[this,n]{queueAppearance(n);});
   }else{
    r->addHover(n.visible?"Hide source":"Show source",n.visible?"eye":"eye-off",[this,n]{itemAction(n,[](auto*i){obs_sceneitem_set_visible(i,!obs_sceneitem_visible(i));});},!n.visible);
    r->addHover(n.locked?"Unlock source":"Lock source",n.locked?"lock":"unlock",[this,n]{itemAction(n,[](auto*i){obs_sceneitem_set_locked(i,!obs_sceneitem_locked(i));});},n.locked);
    r->addHover("Properties","properties",[n]{auto s=source(n.uid);if(s)obs_frontend_open_source_properties(s.get());});
    r->addSecondary("Filters","filters",[n]{auto s=source(n.uid);if(s)obs_frontend_open_source_filters(s.get());});
    r->addSecondary("Move up","up",[this,n]{itemAction(n,[](auto*i){obs_sceneitem_set_order(i,OBS_ORDER_MOVE_UP);});});
    r->addSecondary("Move down","down",[this,n]{itemAction(n,[](auto*i){obs_sceneitem_set_order(i,OBS_ORDER_MOVE_DOWN);});});
    r->addSecondary("Appearance","appearance",[this,n]{queueAppearance(n);});
    if(n.audio)r->addSecondary("Toggle mute",n.muted?"mute":"volume",[n]{auto s=source(n.uid);if(s)obs_source_set_muted(s.get(),!obs_source_muted(s.get()));});
   }
   rows->addWidget(r);
  }
  rows->addStretch(); scroll->verticalScrollBar()->setValue(pos);
 }
public:
 Dock(){
  setObjectName("UnsceneContent");
  nativeList=new QListView(this);nativeList->hide();nativeList->ensurePolished();
  auto*l=new QVBoxLayout(this);l->setContentsMargins(2,2,2,2);l->setSpacing(2);
  auto*bar=new QHBoxLayout; auto*collapse=new GlyphButton("collapse-all",this);collapse->setToolTip("Collapse all scenes");
  auto*expand=new GlyphButton("expand-all",this);expand->setToolTip("Expand all scenes");
  auto*refreshButton=new GlyphButton("refresh",this);refreshButton->setToolTip("Refresh snapshots");
  connect(refreshButton,&QToolButton::clicked,this,[this]{thumbnails.clear();attempted.clear();previous.clear();request();});
  bar->addStretch();bar->addWidget(refreshButton);bar->addWidget(collapse);bar->addWidget(expand);l->addLayout(bar);
  scroll=new QScrollArea(this);scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);body=new QWidget;rows=new QVBoxLayout(body);rows->setContentsMargins(0,0,0,0);rows->setSpacing(1);scroll->setWidget(body);l->addWidget(scroll);
  connect(collapse,&QToolButton::clicked,this,[this]{opened.clear();selected.clear();previous.clear();request();});
  connect(expand,&QToolButton::clicked,this,[this]{for(auto &n:previous)if(n.scene)opened.insert(n.key);previous.clear();request();});
  connect(&timer,&QTimer::timeout,this,[this]{refresh();});clock.start();timer.start(400);
 }
 void themeChanged(){previous.clear();request();}
 void stop(){stopping=true;timer.stop();}
 void refresh(){
  if(stopping||editingAppearance||!isVisible())return;
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
void event(obs_frontend_event e,void*){if(!dock)return;if(e==OBS_FRONTEND_EVENT_EXIT)dock->stop();else if(e==OBS_FRONTEND_EVENT_THEME_CHANGED)dock->themeChanged();}
}
bool obs_module_load(void){dock=new Dock;if(!obs_frontend_add_dock_by_id("unscene.dock","Unscene",dock)){delete dock;return false;}obs_frontend_add_event_callback(event,nullptr);return true;}
void obs_module_unload(void){obs_frontend_remove_event_callback(event,nullptr);if(dock){dock->stop();obs_frontend_remove_dock("unscene.dock");}}
