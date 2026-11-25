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
#include "navigation.hpp"
#include "source-picker.hpp"
#include "item-copy.hpp"
#include <QInputDialog>
#include <QScopedValueRollback>
#include <cstring>
#include <QActionGroup>
#include <QCache>
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
QString configPath(const char* file){
 char* raw=obs_module_config_path(file);
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
 AppearanceStore appearances{configPath("appearance.ini")};
 NavigationStore navigation{configPath("navigation.ini")};
 Density sceneDensity=navigation.sceneDensity(),sourceDensity=navigation.sourceDensity();
 QLineEdit*search;
 GlyphButton *collapse,*expand;
 QString query;
 bool dirty=true;
 QSet<QString> sceneKeys,snapshotKeys;
 bool searching()const{return !query.trimmed().isEmpty();}
 Density density(const Node&n)const{return n.scene?sceneDensity:sourceDensity;}
 void saveOpened(){if(!collection.isEmpty()&&!navigation.saveOpened(collection,opened))blog(LOG_WARNING,"[Unscene] Could not save expansion settings");}
 void changeDensity(bool scenes,Density d){
  (scenes?sceneDensity:sourceDensity)=d;
  if(!navigation.saveDensity(sceneDensity,sourceDensity))blog(LOG_WARNING,"[Unscene] Could not save density settings");
  dirty=true;request();
 }
 void densityMenu(QWidget*anchor){
  QMenu menu(this);
  for(bool scenes:{true,false}){
   auto*sub=menu.addMenu(scenes?"Scene thumbnails":"Source thumbnails");
   auto*group=new QActionGroup(sub);group->setExclusive(true);
   for(auto d:{Density::Compact,Density::Normal,Density::Large}){
    auto*a=sub->addAction(d==Density::Compact?"Compact (icons)":d==Density::Normal?"Standard":"Large");
    a->setCheckable(true);a->setChecked((scenes?sceneDensity:sourceDensity)==d);group->addAction(a);
    connect(a,&QAction::triggered,this,[this,scenes,d]{changeDensity(scenes,d);});
   }
  }
  menu.exec(anchor->mapToGlobal(QPoint(0,anchor->height())));
 }
 QSet<QString> artworkOverrides;
 bool editingAppearance=false;
 std::optional<ItemCopy> copied;
 bool sameCollection(const QString&expected)const{
  char*raw=obs_frontend_get_current_scene_collection();
  auto current=QString::fromUtf8(raw?raw:"");bfree(raw);
  return !stopping&&!expected.isEmpty()&&current==expected;
 }
 void changedScene(const QString&sceneKey){
  const auto root=sceneKey.section('/',0,0);
  thumbnails.remove(root);attempted.remove(root);
  obs_frontend_save();dirty=true;request();
 }
 void revealScene(const Node&n){
  if(!opened.contains(n.key)){opened.insert(n.key);saveOpened();}
  changedScene(n.key);
 }
 void addSource(const Node&n){
  if(editingAppearance||!n.scene||stopping)return;
  const auto expected=collection;
  QScopedValueRollback<bool> guard(editingAppearance,true);
  QVector<SourceChoice> existing,types;
  obs_enum_sources([](void*opaque,obs_source_t*s){
   auto&list=*static_cast<QVector<SourceChoice>*>(opaque);
   if(obs_source_get_type(s)!=OBS_SOURCE_TYPE_INPUT||obs_source_removed(s)||obs_source_is_group(s)||obs_source_is_scene(s))return true;
   auto name=QString::fromUtf8(obs_source_get_name(s));auto id=uuid(s);
   if(!name.isEmpty()&&!id.isEmpty())list.append({id,name});
   return true;
  },&existing);
  QSet<QString> seen;
  const char*id=nullptr,*base=nullptr;
  for(size_t i=0;obs_enum_input_types2(i,&id,&base);++i){
   if(!id||!base||!strcmp(base,"scene")||!strcmp(base,"group"))continue;
   auto flags=obs_get_source_output_flags(id);
   if(flags&(OBS_SOURCE_DEPRECATED|OBS_SOURCE_CAP_DISABLED))continue;
   if(seen.contains(QString::fromUtf8(base)))continue;
   auto label=QString::fromUtf8(obs_source_get_display_name(id));
   if(label.isEmpty())continue;
   seen.insert(QString::fromUtf8(base));types.append({QString::fromUtf8(id),label,true});
  }
  auto decorate=[this](SourceChoice&choice,int kind){
   Node sample;sample.iconType=kind;choice.icon=QIcon(glyphPixmap(sourceGlyph(sample),palette().color(QPalette::WindowText),18,devicePixelRatioF()));
  };
  for(auto&e:existing){auto s=source(e.id);if(s)decorate(e,obs_source_get_icon_type(obs_source_get_id(s.get())));}
  for(auto&t:types)decorate(t,obs_source_get_icon_type(t.id.toUtf8().constData()));
  SourcePicker picker(n.title,existing,types,this);
  if(picker.exec()!=QDialog::Accepted||!sameCollection(expected))return;
  auto choice=picker.choice();if(choice.id.isEmpty())return;
  QString name;
  if(choice.create){
   name=choice.label;
   for(int i=2;;++i){Source used(obs_get_source_by_name(name.toUtf8().constData()),obs_source_release);if(!used)break;name=choice.label+" "+QString::number(i);}
   bool ok=false;name=QInputDialog::getText(this,"New source","Source name",QLineEdit::Normal,name,&ok).trimmed();
   if(!ok||name.isEmpty()||!sameCollection(expected))return;
   Source used(obs_get_source_by_name(name.toUtf8().constData()),obs_source_release);
   if(used){QMessageBox::warning(this,"Unscene","That source name is already in use.");return;}
  }
  auto target=source(n.uid);auto*scene=target?obs_scene_from_source(target.get()):nullptr;
  if(!scene||!sameCollection(expected))return;
  Source input(choice.create?obs_source_create(choice.id.toUtf8().constData(),name.toUtf8().constData(),nullptr,nullptr):obs_get_source_by_uuid(choice.id.toUtf8().constData()),obs_source_release);
  if(!input||obs_source_removed(input.get())||obs_source_get_type(input.get())!=OBS_SOURCE_TYPE_INPUT||obs_source_is_group(input.get())||obs_source_is_scene(input.get())||!obs_scene_add(scene,input.get())){
   QMessageBox::warning(this,"Unscene","Could not add the source to this scene.");return;
  }
  revealScene(n);
  if(choice.create&&obs_source_configurable(input.get()))obs_frontend_open_source_properties(input.get());
 }
 bool canPaste(bool duplicate){
  if(!copied)return false;
  auto s=source(copied->uid);
  return s&&!obs_source_removed(s.get())&&(!duplicate||!(obs_source_get_output_flags(s.get())&OBS_SOURCE_DO_NOT_DUPLICATE));
 }
 void pasteSource(const Node&n,bool duplicate){
  if(stopping||!n.scene||!sameCollection(collection)||!canPaste(duplicate))return;
  auto target=source(n.uid);auto*scene=target?obs_scene_from_source(target.get()):nullptr;
  auto input=source(copied->uid);if(!scene||!input)return;
  if(duplicate){
   const auto base=QString::fromUtf8(obs_source_get_name(input.get()))+" copy";
   QString name=base;
   for(int i=2;;++i){Source used(obs_get_source_by_name(name.toUtf8().constData()),obs_source_release);if(!used)break;name=base+" "+QString::number(i);}
   Source dup(obs_source_duplicate(input.get(),name.toUtf8().constData(),false),obs_source_release);
   if(!dup||dup.get()==input.get()){QMessageBox::warning(this,"Unscene","This source cannot be duplicated independently.");return;}
   input=std::move(dup);
  }
  if(!copied->paste(scene,input.get())){QMessageBox::warning(this,"Unscene","Could not paste the source into this scene.");return;}
  revealScene(n);
 }
 void removeItem(const Node&n){
  if(n.scene||n.group||editingAppearance||stopping)return;
  const auto expected=collection;
  QScopedValueRollback<bool> guard(editingAppearance,true);
  QMessageBox question(QMessageBox::Question,"Remove source","Remove this source instance from its scene? Other instances of the same source will remain. This action does not currently support OBS Undo.",QMessageBox::Yes|QMessageBox::Cancel,this);
  question.setInformativeText(n.title);question.setTextFormat(Qt::PlainText);question.setDefaultButton(QMessageBox::Cancel);
  if(question.exec()!=QMessageBox::Yes||!sameCollection(expected))return;
  itemAction(n,[this,n](auto*i){obs_sceneitem_remove(i);changedScene(n.key);});
 }
 void renameSource(const Node&n){
  if(editingAppearance||stopping)return;
  const auto expected=collection;
  QScopedValueRollback<bool> guard(editingAppearance,true);
  bool ok=false;auto name=QInputDialog::getText(this,"Rename source","Name (applies to every instance)",QLineEdit::Normal,n.title,&ok).trimmed();
  if(!ok||name.isEmpty()||!sameCollection(expected))return;
  auto s=source(n.uid);if(!s||obs_source_removed(s.get()))return;
  Source other(obs_get_source_by_name(name.toUtf8().constData()),obs_source_release);
  if(other&&other.get()!=s.get()){QMessageBox::warning(this,"Unscene","That source name is already in use.");return;}
  obs_source_set_name(s.get(),name.toUtf8().constData());obs_frontend_save();dirty=true;request();
 }
 void contextMenu(const Node&n,const QPoint&pos){
  if(editingAppearance||stopping)return;
  auto expected=collection;
  QMenu menu(this);
  if(n.scene){
   auto*add=menu.addAction("Add source…");connect(add,&QAction::triggered,this,[this,n]{QTimer::singleShot(0,this,[this,n]{addSource(n);});});
   auto*ref=menu.addAction("Paste (reference)");ref->setEnabled(canPaste(false));
   connect(ref,&QAction::triggered,this,[this,n,expected]{if(sameCollection(expected))QTimer::singleShot(0,this,[this,n,expected]{if(sameCollection(expected))pasteSource(n,false);});});
   auto*dup=menu.addAction("Paste (duplicate)");dup->setEnabled(canPaste(true));
   connect(dup,&QAction::triggered,this,[this,n,expected]{if(sameCollection(expected))QTimer::singleShot(0,this,[this,n,expected]{if(sameCollection(expected))pasteSource(n,true);});});
  }else{
   auto*props=menu.addAction("Properties");connect(props,&QAction::triggered,this,[n]{auto s=source(n.uid);if(s)obs_frontend_open_source_properties(s.get());});
   auto*filters=menu.addAction("Filters");connect(filters,&QAction::triggered,this,[n]{auto s=source(n.uid);if(s)obs_frontend_open_source_filters(s.get());});
   auto*transform=menu.addAction("Edit transform");connect(transform,&QAction::triggered,this,[this,n,expected]{if(sameCollection(expected))itemAction(n,[](auto*i){obs_frontend_open_sceneitem_edit_transform(i);});});
   if(!n.group){
    menu.addSeparator();
    auto*copy=menu.addAction("Copy source");auto input=source(n.uid);copy->setEnabled(input&&!obs_source_is_scene(input.get()));connect(copy,&QAction::triggered,this,[this,n,expected]{if(sameCollection(expected))itemAction(n,[this](auto*i){copied=ItemCopy::capture(i);if(copied)obs_frontend_copy_sceneitem(i);});});
    auto*remove=menu.addAction("Remove from scene…");connect(remove,&QAction::triggered,this,[this,n,expected]{if(sameCollection(expected))QTimer::singleShot(0,this,[this,n]{removeItem(n);});});
   }
  }
  menu.addSeparator();
  auto*rename=menu.addAction("Rename…");connect(rename,&QAction::triggered,this,[this,n]{QTimer::singleShot(0,this,[this,n]{renameSource(n);});});
  auto*appearance=menu.addAction("Appearance…");connect(appearance,&QAction::triggered,this,[this,n]{queueAppearance(n);});
  menu.exec(pos);
 }
 void queueContext(const Node&n){const auto pos=QCursor::pos();QTimer::singleShot(0,this,[this,n,pos]{contextMenu(n,pos);});}

 void editAppearance(const Node& n){
  if(editingAppearance||stopping)return;
  editingAppearance=true;
  AppearanceDialog dialog(n.title,appearances.load(n.uid),this);
  if(dialog.exec()==QDialog::Accepted){
   if(!appearances.save(n.uid,dialog.value()))QMessageBox::warning(this,"Unscene","Could not save appearance settings.");
   dirty=true;
  }
  editingAppearance=false;request();
 }
 void queueAppearance(const Node&n){QTimer::singleShot(0,this,[this,n]{editAppearance(n);});}
 static constexpr int maxDepth=24;
 void updateSnapshots(){
  if(!isVisible())return;
  for(const auto&n:previous){
   auto label=labels.value(n.key);
   if(!snapshotKeys.contains(n.key)||artworkOverrides.contains(n.uid)||!n.video||!label||thumbnails.contains(n.uid)||!scroll->viewport()->rect().intersects(QRect(label->mapTo(scroll->viewport(),QPoint()),label->size())))continue;
   auto s=source(n.uid);if(!s||!obs_source_showing(s.get()))continue;
   if(attempted.contains(n.uid)&&clock.elapsed()-attempted.value(n.uid)<3000)continue;
   attempted.insert(n.uid,clock.elapsed());
   const auto image=captureSnapshot(s.get());if(!hasVisiblePixels(image))return;
   if(thumbnails.size()>=512)thumbnails.clear();
   auto pix=QPixmap::fromImage(image);thumbnails.insert(n.uid,pix);
   for(const auto&key:keys.values(n.uid)){auto l=labels.value(key);if(l&&snapshotKeys.contains(key))l->setPixmap(pix.scaled(l->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));}
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
   if(n.group&&(searching()||opened.contains(n.key))){auto s=source(n.uid); if(s)collect(obs_group_from_source(s.get()),n.key,depth+1,out);}
  }
 }
 template<class F> void itemAction(const Node&n,F f){
  auto p=source(n.parent); if(!p)return; auto *scene=obs_scene_from_source(p.get()); if(!scene)scene=obs_group_from_source(p.get()); if(!scene)return;
  auto *item=obs_scene_find_sceneitem_by_id(scene,n.id); if(!item||uuid(obs_sceneitem_get_source(item))!=n.uid)return;
  obs_sceneitem_addref(item); f(item); obs_sceneitem_release(item); request();
 }
 void request(){if(pending||stopping)return;pending=true;QTimer::singleShot(0,this,[this]{pending=false;refresh();});}
 void toggle(const Node &n){
  if((n.scene||n.group)&&!searching()){if(opened.contains(n.key))opened.remove(n.key);else opened.insert(n.key);saveOpened();}
  if(!n.scene)selected=selected==n.key?QString():n.key;
  dirty=true;request();
 }
 void render(const QVector<Node>&nodes){
  const int pos=scroll->verticalScrollBar()->value();
  labels.clear();keys.clear();artworkOverrides.clear();snapshotKeys.clear();
  while(auto *it=rows->takeAt(0)){delete it->widget();delete it;}
  for(const auto&n:nodes){
   const bool isOpen=searching()||opened.contains(n.key), expanded=selected==n.key;
   auto *r=new Row(n.title,n.scene,expanded,n.depth,body);
   r->setNativeList(nativeList);
   r->setDisclosure(n.scene||n.group,isOpen);
   r->thumb->setFixedSize(thumbnailSize(density(n),n.scene));
   r->setSelected(n.program||n.preview||expanded);
   r->setStatus(n.program?"LIVE":n.preview?"PREVIEW":"");
   r->thumb->setPixmap(sourceArtwork(n,r));
   labels.insert(n.key,r->thumb);keys.insert(n.uid,n.key);
   if(density(n)!=Density::Compact){snapshotKeys.insert(n.key);}
   if(snapshotKeys.contains(n.key)&&thumbnails.contains(n.uid))r->thumb->setPixmap(thumbnails.value(n.uid).scaled(r->thumb->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
   const auto appearance=appearances.load(n.uid);
   const auto art=appearancePixmap(appearance,artworkPalette(r),r->thumb->size());
   if(!art.isNull()){artworkOverrides.insert(n.uid);r->thumb->setPixmap(art);}
   r->setAccent(appearance.color);
   r->onAppearance=[this,n]{queueContext(n);};
   r->onBody=[this,n]{toggle(n);};
   if(n.scene){
    r->addHover(obs_frontend_preview_program_mode_active()?"Select preview scene":"Activate scene","arrow-right",[this,n]{auto s=source(n.uid);if(!s||!obs_scene_from_source(s.get()))return;if(obs_frontend_preview_program_mode_active())obs_frontend_set_current_preview_scene(s.get());else obs_frontend_set_current_scene(s.get());request();});
    r->addHover("Add source","plus",[this,n]{QTimer::singleShot(0,this,[this,n]{addSource(n);});});
    r->addHover("More actions","more",[this,n]{queueContext(n);});
   }else{
    r->addHover(n.visible?"Hide source":"Show source",n.visible?"eye":"eye-off",[this,n]{itemAction(n,[](auto*i){obs_sceneitem_set_visible(i,!obs_sceneitem_visible(i));});},!n.visible);
    r->addHover(n.locked?"Unlock source":"Lock source",n.locked?"lock":"unlock",[this,n]{itemAction(n,[](auto*i){obs_sceneitem_set_locked(i,!obs_sceneitem_locked(i));});},n.locked);
    r->addHover("Properties","properties",[n]{auto s=source(n.uid);if(s)obs_frontend_open_source_properties(s.get());});
    r->addSecondary("Filters","filters",[n]{auto s=source(n.uid);if(s)obs_frontend_open_source_filters(s.get());});
    r->addSecondary("Move up","up",[this,n]{itemAction(n,[](auto*i){obs_sceneitem_set_order(i,OBS_ORDER_MOVE_UP);});});
    r->addSecondary("Move down","down",[this,n]{itemAction(n,[](auto*i){obs_sceneitem_set_order(i,OBS_ORDER_MOVE_DOWN);});});
    r->addSecondary("More actions","more",[this,n]{queueContext(n);});
    if(n.audio)r->addSecondary("Toggle mute",n.muted?"mute":"volume",[n]{auto s=source(n.uid);if(s)obs_source_set_muted(s.get(),!obs_source_muted(s.get()));});
   }
   rows->addWidget(r);
  }
  if(nodes.isEmpty()){
   auto*empty=new QLabel(searching()?"No matching scenes or sources":"No scenes",body);empty->setAlignment(Qt::AlignCenter);empty->setContentsMargins(4,12,4,12);rows->addWidget(empty);
  }
  rows->addStretch(); scroll->verticalScrollBar()->setValue(pos);
 }
public:
 Dock(){
  setObjectName("UnsceneContent");
  nativeList=new QListView(this);nativeList->hide();nativeList->ensurePolished();
  auto*l=new QVBoxLayout(this);l->setContentsMargins(2,2,2,2);l->setSpacing(2);
  auto*bar=new QHBoxLayout; collapse=new GlyphButton("collapse-all",this);collapse->setToolTip("Collapse all scenes");
  expand=new GlyphButton("expand-all",this);expand->setToolTip("Expand all scenes");
  auto*refreshButton=new GlyphButton("refresh",this);refreshButton->setToolTip("Refresh snapshots");
  connect(refreshButton,&QToolButton::clicked,this,[this]{thumbnails.clear();attempted.clear();dirty=true;request();});
  auto*settingsButton=new GlyphButton("properties",this);settingsButton->setToolTip("Thumbnail density");settingsButton->setAccessibleName("Thumbnail density");
  connect(settingsButton,&QToolButton::clicked,this,[this,settingsButton]{densityMenu(settingsButton);});
  search=new QLineEdit(this);search->setPlaceholderText("Find scenes or sources…");search->setClearButtonEnabled(true);search->setAccessibleName("Find scenes or sources");search->setMinimumWidth(50);
  bar->addWidget(search,1);bar->addWidget(refreshButton);bar->addWidget(collapse);bar->addWidget(expand);bar->addWidget(settingsButton);l->addLayout(bar);
  connect(search,&QLineEdit::textChanged,this,[this](const QString&t){query=t;collapse->setEnabled(!searching());expand->setEnabled(!searching());dirty=true;request();});
  scroll=new QScrollArea(this);scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);body=new QWidget;rows=new QVBoxLayout(body);rows->setContentsMargins(0,0,0,0);rows->setSpacing(1);scroll->setWidget(body);l->addWidget(scroll);
  connect(collapse,&QToolButton::clicked,this,[this]{opened.clear();selected.clear();saveOpened();dirty=true;request();});
  connect(expand,&QToolButton::clicked,this,[this]{opened.unite(sceneKeys);saveOpened();dirty=true;request();});
  connect(&timer,&QTimer::timeout,this,[this]{refresh();});clock.start();timer.start(400);
 }
 void themeChanged(){dirty=true;request();}
 void stop(){stopping=true;timer.stop();}
 void refresh(){
  if(stopping||editingAppearance||!isVisible())return;
  char *raw=obs_frontend_get_current_scene_collection();QString now=QString::fromUtf8(raw?raw:"");bfree(raw);
  if(collection!=now){collection=now;copied.reset();thumbnails.clear();attempted.clear();opened.clear();selected.clear();initialized=false;dirty=true;}
  Source live(obs_frontend_get_current_scene(),obs_source_release);
  Source preview(obs_frontend_preview_program_mode_active()?obs_frontend_get_current_preview_scene():nullptr,obs_source_release);
  if(!initialized&&!now.isEmpty()){auto saved=navigation.opened(now);if(saved)opened=*saved;else if(live)opened.insert(uuid(live.get()));initialized=true;}
  obs_frontend_source_list list={};obs_frontend_get_scenes(&list);
  QVector<Node> nodes;sceneKeys.clear();
  for(size_t i=0;i<list.sources.num;i++){
   auto*s=list.sources.array[i];auto n=makeNode(s);n.scene=true;n.key=n.uid;n.program=s==live.get();n.preview=s==preview.get();nodes.append(n);sceneKeys.insert(n.key);
   if(searching()||opened.contains(n.key))collect(obs_scene_from_source(s),n.key,1,nodes);
  }
  obs_frontend_source_list_free(&list);
  if(searching())nodes=filterHierarchy(nodes,query);
  if(dirty||nodes!=previous){previous=nodes;dirty=false;render(nodes);}
  updateSnapshots();
 }
};
QPointer<Dock> dock;
void event(obs_frontend_event e,void*){if(!dock)return;if(e==OBS_FRONTEND_EVENT_EXIT)dock->stop();else if(e==OBS_FRONTEND_EVENT_THEME_CHANGED)dock->themeChanged();}
}
bool obs_module_load(void){dock=new Dock;if(!obs_frontend_add_dock_by_id("unscene.dock","Unscene",dock)){delete dock;return false;}obs_frontend_add_event_callback(event,nullptr);return true;}
void obs_module_unload(void){obs_frontend_remove_event_callback(event,nullptr);if(dock){dock->stop();obs_frontend_remove_dock("unscene.dock");}}
