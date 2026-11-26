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
#include <vector>
#include "row.hpp"
#include "snapshot.hpp"
#include <QToolBar>
#include "appearance-dialog.hpp"
#include "navigation.hpp"
#include "source-picker.hpp"
#include "item-copy.hpp"
#include "native-menu.hpp"
#include "item-menu.hpp"
#include "item-state.hpp"
#include "inspector.hpp"
#include "inspector-motion.hpp"
#include "filter-undo.hpp"
#include "portable-filter.hpp"
#include "portable-source.hpp"
#include <QClipboard>
#include <QFileDialog>
#include <QSaveFile>
#include "order.hpp"
#include "drag-scroll.hpp"
#include "reorder-motion.hpp"
#include <QHideEvent>
#include <QJsonObject>
#include <QSplitter>
#include <QGuiApplication>
#include <QScreen>
#include <QListWidget>
#include <QItemSelectionModel>
#include <QMetaMethod>
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
 const auto size=qMin(r->thumb->height(),r->style()->pixelMetric(QStyle::PM_SmallIconSize,nullptr,r));
 auto*window=static_cast<QWidget*>(obs_frontend_get_main_window());
 if(window){
  const auto index=window->metaObject()->indexOfProperty(nativeIconProperty(n));
  if(index>=0){
   const auto value=window->metaObject()->property(index).read(window);
   const auto icon=qvariant_cast<QIcon>(value);
   auto pix=icon.pixmap(QSize(size,size),r->devicePixelRatioF());
   if(!pix.isNull()){
    if(n.program||n.preview||r->expanded()){
     QPainter painter(&pix);painter.setCompositionMode(QPainter::CompositionMode_SourceIn);painter.fillRect(pix.rect(),r->inkColor());
    }
    return pix;
   }
  }
 }
 return glyphPixmap(sourceGlyph(n),r->inkColor(),size,r->devicePixelRatioF());
}
QPalette artworkPalette(Row*r){auto p=r->palette();p.setColor(QPalette::WindowText,r->inkColor());return p;}
QString configPath(const char* file){
 char* raw=obs_module_config_path(file);
 const auto path=QString::fromUtf8(raw?raw:"");bfree(raw);
 return path;
}
QString currentCollection(){
 char*raw=obs_frontend_get_current_scene_collection();auto result=QString::fromUtf8(raw?raw:"");bfree(raw);return result;
}
template<class F> bool withItem(const QString&parent,int64_t id,const QString&uid,F fn){
 auto p=source(parent);if(!p||obs_source_removed(p.get()))return false;
 auto*scene=obs_scene_from_source(p.get());if(!scene)scene=obs_group_from_source(p.get());if(!scene)return false;
 auto*i=obs_scene_find_sceneitem_by_id(scene,id);if(!i||uuid(obs_sceneitem_get_source(i))!=uid)return false;
 obs_sceneitem_addref(i);fn(i);obs_sceneitem_release(i);return true;
}
void restoreItemState(const char*data){
 auto u=ItemStateUndo::decode(data);if(!u||u->collection!=currentCollection())return;
 bool changed=false;
 withItem(u->parent,u->id,u->source,[&](auto*i){
  if(readItemState(i,u->field)==u->expected)changed=writeItemState(i,u->field,u->next);
 });
 if(changed)obs_frontend_save();
}
QByteArray transformUndoData(const Node&n,const QString&collection,const TransformState&t){
 return QJsonDocument(QJsonObject{{"collection",collection},{"parent",n.parent},{"id",QString::number(n.id)},{"source",n.uid},{"transform",QString::fromUtf8(t.encode())}}).toJson(QJsonDocument::Compact);
}
void restoreInspectorTransform(const char*data){
 auto obj=QJsonDocument::fromJson(data).object();
 if(obj.value("collection").toString()!=currentCollection())return;
 bool ok=false;auto id=obj.value("id").toString().toLongLong(&ok);
 auto t=TransformState::decode(obj.value("transform").toString().toUtf8());if(!ok||!t)return;
 if(withItem(obj.value("parent").toString(),id,obj.value("source").toString(),[&](auto*i){t->apply(i);}))obs_frontend_save();
}
Source findAttachedFilter(obs_source_t*parent,const QString&id){
 struct C{QString id;obs_source_t*found=nullptr;}c{id};
 obs_source_enum_filters(parent,[](obs_source_t*,obs_source_t*f,void*p){auto&c=*static_cast<C*>(p);if(uuid(f)==c.id&&!c.found)c.found=obs_source_get_ref(f);},&c);
 return Source(c.found,obs_source_release);
}
int filterCount(obs_source_t*parent){
 int count=0;obs_source_enum_filters(parent,[](obs_source_t*,obs_source_t*,void*p){++*static_cast<int*>(p);},&count);return count;
}
QJsonValue savedFilter(obs_source_t*parent,obs_source_t*filter){
 std::unique_ptr<obs_data_t,decltype(&obs_data_release)> data(obs_save_source(filter),obs_data_release);
 if(!data)return {};
 return QJsonObject{{"index",obs_source_filter_get_index(parent,filter)},{"source",QJsonDocument::fromJson(obs_data_get_json(data.get())).object()}};
}
void restoreFilterUndo(const char*data){
 auto u=FilterUndo::decode(data);if(!u||u->collection!=currentCollection())return;
 auto parent=source(u->parent);if(!parent||obs_source_removed(parent.get()))return;
 auto f=findAttachedFilter(parent.get(),u->filter);
 if(u->operation=="present"){
  if(f)return;
  auto v=u->value.toObject();auto snapshot=v["source"].toObject();
  Source existing(obs_get_source_by_uuid(u->filter.toUtf8().constData()),obs_source_release);
  Source collision(obs_source_get_filter_by_name(parent.get(),snapshot["name"].toString().toUtf8().constData()),obs_source_release);
  if(existing||collision)return;
  const int index=v["index"].toInt();
  if(index>filterCount(parent.get()))return;
  auto bytes=QJsonDocument(snapshot).toJson(QJsonDocument::Compact);
  std::unique_ptr<obs_data_t,decltype(&obs_data_release)> settings(obs_data_create_from_json(bytes.constData()),obs_data_release);
  if(!settings)return;
  Source restored(obs_load_source(settings.get()),obs_source_release);
  if(!restored||uuid(restored.get())!=u->filter||obs_source_get_type(restored.get())!=OBS_SOURCE_TYPE_FILTER)return;
  obs_source_filter_add(parent.get(),restored.get());
  if(obs_source_filter_get_index(parent.get(),restored.get())<0)return;
  obs_source_filter_set_index(parent.get(),restored.get(),size_t(index));
 }else{
  if(!f)return;
  if(u->operation=="absent")obs_source_filter_remove(parent.get(),f.get());
  else if(u->operation=="enabled")obs_source_set_enabled(f.get(),u->value.toBool());
  else if(u->operation=="index"){
   const int index=u->value.toInt();if(index>=filterCount(parent.get()))return;
   obs_source_filter_set_index(parent.get(),f.get(),size_t(index));
  }else if(u->operation=="name"){
   auto name=u->value.toString().toUtf8();
   Source collision(obs_source_get_filter_by_name(parent.get(),name.constData()),obs_source_release);
   if(collision&&collision.get()!=f.get())return;
   obs_source_set_name(f.get(),name.constData());
  }
 }
 obs_frontend_save();
}
void registerFilterUndo(const QString&label,const FilterUndo&before,const FilterUndo&after){
 auto undo=before.encode(),redo=after.encode();
 if(undo==redo||!FilterUndo::decode(undo)||!FilterUndo::decode(redo))return;
 auto title=("Unscene: "+label).toUtf8();
 obs_frontend_add_undo_redo_action(title.constData(),restoreFilterUndo,restoreFilterUndo,undo.constData(),redo.constData(),false);
}
QString itemOrderKey(obs_sceneitem_t*i){return QString::number(obs_sceneitem_get_id(i))+":"+uuid(obs_sceneitem_get_source(i));}
QString nodeOrderKey(const Node&n){return QString::number(n.id)+":"+n.uid;}
obs_scene_t* ownerScene(obs_source_t*s){if(!s)return nullptr;auto*p=obs_scene_from_source(s);return p?p:obs_group_from_source(s);}
QStringList currentOrder(obs_scene_t*scene){
 QStringList out;if(scene)obs_scene_enum_items(scene,[](obs_scene_t*,obs_sceneitem_t*i,void*p){static_cast<QStringList*>(p)->prepend(itemOrderKey(i));return true;},&out);return out;
}
bool applyOrder(obs_scene_t*scene,const QStringList&expected,const QStringList&next){
 if(!scene||!sameOrderMembers(expected,next)||expected==next)return false;
 // Retain refs across enumeration and atomic reorder. OBS validates the membership under its own lock.
 struct Items{QStringList order;QHash<QString,obs_sceneitem_t*> refs;}items;
 obs_scene_enum_items(scene,[](obs_scene_t*,obs_sceneitem_t*i,void*p){
  auto&c=*static_cast<Items*>(p);auto k=itemOrderKey(i);c.order.prepend(k);obs_sceneitem_addref(i);c.refs.insert(k,i);return true;
 },&items);
 bool changed=false;
 if(items.order==expected){
  QVector<obs_sceneitem_t*> order;for(auto it=next.crbegin();it!=next.crend();++it)order.append(items.refs.value(*it));
  changed=obs_scene_reorder_items(scene,order.data(),size_t(order.size()));
 }
 for(auto*i:items.refs)obs_sceneitem_release(i);
 return changed;
}
void restoreOrder(const char*data){
 auto u=OrderUndo::decode(data);if(!u||u->collection!=currentCollection())return;
 auto p=source(u->parent);if(p&&!obs_source_removed(p.get())&&applyOrder(ownerScene(p.get()),u->expected,u->next))obs_frontend_save();
}
QString portableType(const QJsonObject&o){auto v=o["versioned_id"].toString();return v.isEmpty()?o["id"].toString():v;}
QString portableSourceError(const QJsonObject&o){
 const char*id=nullptr;auto type=portableType(o);bool installed=false;
 for(size_t i=0;obs_enum_input_types(i,&id);++i)if(type==QString::fromUtf8(id)){installed=true;break;}
 if(!installed)return "Missing input type: "+type;
 auto flags=obs_get_source_output_flags(type.toUtf8().constData());
 if(flags&(OBS_SOURCE_DO_NOT_DUPLICATE|OBS_SOURCE_CAP_DISABLED))return "Input type cannot be independently duplicated: "+type;
 QSet<QString> filters;
 for(size_t i=0;obs_enum_filter_types(i,&id);++i)filters.insert(QString::fromUtf8(id));
 for(auto v:o["filters"].toArray()){
  auto f=v.toObject();auto ft=portableType(f);
  if(!filters.contains(ft)||!filterCompatible(flags,obs_get_source_output_flags(ft.toUtf8().constData())))return "Missing or incompatible filter: "+ft;
 }
 return {};
}
QJsonObject savePortableSource(obs_source_t*s){
 std::unique_ptr<obs_data_t,decltype(&obs_data_release)> data(obs_save_source(s),obs_data_release);
 return data?QJsonDocument::fromJson(obs_data_get_json(data.get())).object():QJsonObject();
}
Source loadPortableSource(const QJsonObject&o){
 if(!portableSourceError(o).isEmpty())return Source(nullptr,obs_source_release);
 auto json=QJsonDocument(o).toJson(QJsonDocument::Compact);
 std::unique_ptr<obs_data_t,decltype(&obs_data_release)> data(obs_data_create_from_json(json.constData()),obs_data_release);
 Source input(data?obs_load_source(data.get()):nullptr,obs_source_release);
 if(!input||uuid(input.get())!=o["uuid"].toString()||obs_source_get_type(input.get())!=OBS_SOURCE_TYPE_INPUT||filterCount(input.get())!=o["filters"].toArray().size())return Source(nullptr,obs_source_release);
 obs_source_load2(input.get());return input;
}
void restorePortableItem(const char*data){
 auto o=QJsonDocument::fromJson(data).object();
 if(o["collection"].toString()!=currentCollection())return;
 auto parent=source(o["parent"].toString());auto*scene=parent?ownerScene(parent.get()):nullptr;
 bool ok=false;auto id=o["item"].toString().toLongLong(&ok);
 auto payload=decodePortableSource(o["payload"].toString().toUtf8());
 if(!scene||!ok||id<=0||!payload)return;
 auto uid=payload->source["uuid"].toString();if(uid.isEmpty())return;
 auto*i=obs_scene_find_sceneitem_by_id(scene,id);
 if(o["remove"].toBool()){
  if(!i||uuid(obs_sceneitem_get_source(i))!=uid)return;
  obs_sceneitem_remove(i);obs_frontend_save();return;
 }
 if(i)return;
 auto input=source(uid);
 if(input){if(obs_source_removed(input.get())||QString::fromUtf8(obs_source_get_id(input.get()))!=portableType(payload->source))return;}
 else{
  Source collision(obs_get_source_by_name(payload->source["name"].toString().toUtf8().constData()),obs_source_release);
  if(collision)return;
  input=loadPortableSource(payload->source);
 }
 if(input)if(auto*newItem=payload->item.paste(scene,input.get())){obs_sceneitem_set_id(newItem,id);obs_frontend_save();}
}
class Dock : public QWidget {
 QListView *nativeList; ReorderLayout *rows; QVBoxLayout *dockLayout; QToolBar*bar; QScrollArea *scroll; QWidget *body; QTimer timer;
 Inspector*inspector; QSplitter*splitter; InspectorDrawer*drawer;
 std::optional<Node> inspected;
 QString inspectorCollection;
 Source inspectorSource(const Node&n,const QString&expected){
  if(!sameCollection(expected))return Source(nullptr,obs_source_release);
  Source result(nullptr,obs_source_release);
  withItem(n.parent,n.id,n.uid,[&](auto*i){result.reset(obs_source_get_ref(obs_sceneitem_get_source(i)));});
  return result;
 }
 QVector<FilterEntry> filterEntries(obs_source_t*parent){
  QVector<FilterEntry> out;
  obs_source_enum_filters(parent,[](obs_source_t*,obs_source_t*f,void*opaque){
   auto&v=*static_cast<QVector<FilterEntry>*>(opaque);
   auto flags=obs_source_get_output_flags(f);
   v.append({uuid(f),QString::fromUtf8(obs_source_get_name(f)),QString::fromUtf8(obs_source_get_display_name(obs_source_get_id(f))),obs_source_enabled(f),bool(flags&OBS_SOURCE_ASYNC)});
  },&out);return out;
 }
 void refreshInspectorExtras(bool usage=false){
  if(!inspected)return;
  auto parent=inspectorSource(*inspected,inspectorCollection);if(!parent)return;
  if(inspector->filtersVisible())inspector->setFilters(filterEntries(parent.get()));
  if(!usage||!inspector->usageVisible())return;
  QStringList lines;
  obs_frontend_source_list scenes={};obs_frontend_get_scenes(&scenes);
  for(size_t i=0;i<scenes.sources.num;++i){
   auto*root=scenes.sources.array[i];collectUsage(obs_scene_from_source(root),QString::fromUtf8(obs_source_get_name(root)),inspected->uid,lines,0);
  }
  obs_frontend_source_list_free(&scenes);
  if(lines.isEmpty())lines<<"No direct instances found";
  inspector->setUsage(lines);
 }
 static void collectUsage(obs_scene_t*scene,const QString&path,const QString&uid,QStringList&lines,int depth){
  if(!scene||depth>24)return;
  struct C{QString path,uid;QStringList*lines;int depth;int count=0;}c{path,uid,&lines,depth};
  obs_scene_enum_items(scene,[](obs_scene_t*,obs_sceneitem_t*i,void*p){
   auto&c=*static_cast<C*>(p);auto*s=obs_sceneitem_get_source(i);if(!s)return true;
   if(uuid(s)==c.uid)++c.count;
   if(obs_sceneitem_is_group(i))collectUsage(obs_sceneitem_group_get_scene(i),c.path+" / "+QString::fromUtf8(obs_source_get_name(s)),c.uid,*c.lines,c.depth+1);
   return true;
  },&c);
  if(c.count)lines<<QString("%1  (%2)").arg(path).arg(c.count);
 }
 QJsonArray portableFilters(obs_source_t*parent,const QString&id,bool stack){
  struct C{QString id;bool stack;QJsonArray out;bool failed=false;}c{id,stack};
  obs_source_enum_filters(parent,[](obs_source_t*,obs_source_t*f,void*p){
   auto&c=*static_cast<C*>(p);if(!c.stack&&uuid(f)!=c.id)return;
   std::unique_ptr<obs_data_t,decltype(&obs_data_release)> data(obs_save_source(f),obs_data_release);
   if(!data){c.failed=true;return;}
   c.out.append(QJsonDocument::fromJson(obs_data_get_json(data.get())).object());
  },&c);
  return c.failed?QJsonArray():c.out;
 }
 void transferFilters(const Node&n,const QString&expected,obs_source_t*parent,const QString&action,const QString&id){
  if(action=="copy"||action=="copy-stack"||action=="export"||action=="export-stack"){
   auto bytes=encodePortableFilters(portableFilters(parent,id,action.endsWith("-stack")));
   if(!decodePortableFilters(bytes)){QMessageBox::warning(this,"Unscene","Could not serialize filters or the transfer exceeds its size limit.");return;}
   if(QMessageBox::question(this,"Share filter settings","Saved filter settings may contain private paths, URLs or credentials. Only share with trusted recipients. Continue?",QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)!=QMessageBox::Yes)return;
   if(action.startsWith("copy")){QGuiApplication::clipboard()->setText(QString::fromUtf8(bytes));return;}
   auto file=QFileDialog::getSaveFileName(this,"Export filters","filters.json","JSON files (*.json)");
   if(file.isEmpty())return;
   QSaveFile out(file);
   if(!out.open(QIODevice::WriteOnly)||out.write(bytes)!=bytes.size()||!out.commit())QMessageBox::warning(this,"Unscene","Could not save filter file.");
   return;
  }
  QByteArray bytes;
  if(action=="paste")bytes=QGuiApplication::clipboard()->text().toUtf8();
  else {
   auto path=QFileDialog::getOpenFileName(this,"Import filters",QString(),"JSON files (*.json)");
   if(path.isEmpty())return;QFile f(path);
   if(!f.open(QIODevice::ReadOnly)||f.size()>PortableFilterMaxBytes){QMessageBox::warning(this,"Unscene","Unable to read filter file or it is too large.");return;}
   bytes=f.read(PortableFilterMaxBytes+1);
  }
  auto parsed=decodePortableFilters(bytes);
  if(!parsed){QMessageBox::warning(this,"Unscene","Invalid filter data. Use an OBS saved filter or Unscene filter-stack JSON.");return;}
  auto target=inspectorSource(n,expected);if(!target)return;
  QSet<QString> installed,names;const char*type=nullptr;
  for(size_t i=0;obs_enum_filter_types(i,&type);++i)if(type)installed.insert(QString::fromUtf8(type));
  obs_source_enum_filters(target.get(),[](obs_source_t*,obs_source_t*f,void*p){static_cast<QSet<QString>*>(p)->insert(QString::fromUtf8(obs_source_get_name(f)));},&names);
  QJsonArray prepared;QStringList labels;
  for(auto v:*parsed){
   auto f=v.toObject();auto type=f["versioned_id"].toString(f["id"].toString());
   if(!installed.contains(type)||!filterCompatible(obs_source_get_output_flags(target.get()),obs_get_source_output_flags(type.toUtf8().constData()))){
    QMessageBox::warning(this,"Unscene","Missing or incompatible filter type: "+type);return;
   }
   f=preparePortableFilter(f,names);prepared.append(f);labels<<f["name"].toString()+" ("+type+")";
  }
  QMessageBox confirm(QMessageBox::Question,"Import filters","Append these filters to the shared source? Existing filters will not be replaced. Only import settings you trust.",QMessageBox::Yes|QMessageBox::Cancel,this);
  confirm.setTextFormat(Qt::PlainText);confirm.setDetailedText(labels.join(QChar(10)));confirm.setInformativeText(QString("%1 filter(s) → %2. Each is individually undoable. Referenced external files are not included.").arg(prepared.size()).arg(n.title));confirm.setDefaultButton(QMessageBox::Cancel);
  if(confirm.exec()!=QMessageBox::Yes)return;
  target=inspectorSource(n,expected);if(!target)return;
  // Validate names again after the modal confirmation.
  for(auto v:prepared){auto f=v.toObject();Source existing(obs_source_get_filter_by_name(target.get(),f["name"].toString().toUtf8().constData()),obs_source_release);if(existing){QMessageBox::warning(this,"Unscene","Filter names changed; retry import.");return;}}
  std::vector<Source> added;bool failed=false;
  for(auto v:prepared){
   auto f=v.toObject();auto json=QJsonDocument(f).toJson(QJsonDocument::Compact);
   std::unique_ptr<obs_data_t,decltype(&obs_data_release)> settings(obs_data_create_from_json(json.constData()),obs_data_release);
   Source created(settings?obs_load_source(settings.get()):nullptr,obs_source_release);
   if(!created||uuid(created.get())!=f["uuid"].toString()||obs_source_get_type(created.get())!=OBS_SOURCE_TYPE_FILTER){failed=true;break;}
   obs_source_filter_add(target.get(),created.get());
   if(obs_source_filter_get_index(target.get(),created.get())<0){failed=true;break;}
   obs_source_load(created.get());added.push_back(std::move(created));
  }
  if(failed){
   for(auto&f:added)obs_source_filter_remove(target.get(),f.get());
   QMessageBox::warning(this,"Unscene","Could not load all filters; imported filters were rolled back.");return;
  }
  // Save undo snapshots in application order. Each filter was inserted at
  // internal index zero; redo must replay that same insertion order.
  for(auto&f:added){
   auto snap=savedFilter(target.get(),f.get()).toObject();snap["index"]=0;
   auto fid=uuid(f.get());
   registerFilterUndo("paste filter "+QString::fromUtf8(obs_source_get_name(f.get())),{expected,n.uid,fid,"absent",QJsonValue::Null},{expected,n.uid,fid,"present",snap});
  }
  thumbnails.remove(n.uid);attempted.remove(n.uid);changedScene(n.key);refreshInspectorExtras();
 }
 void filterAction(const Node&n,const QString&expected,const QString&action,const QString&id){
  if(!inspected||inspected->key!=n.key||!sameCollection(expected))return;
  auto parent=inspectorSource(n,expected);if(!parent)return;
  QScopedValueRollback<bool> guard(editingAppearance,true);
  if(action=="settings"){obs_frontend_open_source_filters(parent.get());return;}
  if(action=="copy"||action=="copy-stack"||action=="export"||action=="export-stack"||action=="paste"||action=="import"){
   transferFilters(n,expected,parent.get(),action,id);return;
  }
  auto record=[&](const QString&label,const QString&fid,const QString&op,QJsonValue before,QJsonValue after){
   registerFilterUndo(label,{expected,n.uid,fid,op,before},{expected,n.uid,fid,op,after});
  };
  if(action=="add"){
   QVector<QPair<QString,QString>> types;const char*type=nullptr;
   auto flags=obs_source_get_output_flags(parent.get());
   for(size_t i=0;obs_enum_filter_types(i,&type);++i)if(type&&filterCompatible(flags,obs_get_source_output_flags(type))){
    auto name=QString::fromUtf8(obs_source_get_display_name(type));if(!name.isEmpty())types.append({name,QString::fromUtf8(type)});
   }
   std::sort(types.begin(),types.end(),[](auto&a,auto&b){return a.first.localeAwareCompare(b.first)<0;});
   QStringList labels;for(auto&t:types)labels<<t.first+" — "+t.second;
   if(labels.isEmpty()){QMessageBox::information(this,"Unscene","No compatible filters.");return;}
   bool ok=false;auto choice=QInputDialog::getItem(this,"Add filter","Filter type",labels,0,false,&ok);if(!ok)return;
   auto index=labels.indexOf(choice);if(index<0)return;auto typeId=types[index].second;auto name=types[index].first;
   for(int i=2;;++i){Source other(obs_source_get_filter_by_name(parent.get(),name.toUtf8().constData()),obs_source_release);if(!other)break;name=types[index].first+" "+QString::number(i);}
   name=QInputDialog::getText(this,"Add filter","Name",QLineEdit::Normal,name,&ok).trimmed();
   if(!ok||name.isEmpty())return;
   parent=inspectorSource(n,expected);if(!parent)return;
   Source other(obs_source_get_filter_by_name(parent.get(),name.toUtf8().constData()),obs_source_release);
   if(other){QMessageBox::warning(this,"Unscene","That filter name is already in use.");return;}
   Source f(obs_source_create(typeId.toUtf8().constData(),name.toUtf8().constData(),nullptr,nullptr),obs_source_release);
   if(!f){QMessageBox::warning(this,"Unscene","Could not create the filter.");return;}
   obs_source_filter_add(parent.get(),f.get());
   registerFilterUndo("add filter "+name,{expected,n.uid,uuid(f.get()),"absent",QJsonValue::Null},{expected,n.uid,uuid(f.get()),"present",savedFilter(parent.get(),f.get())});
  }else{
   auto f=findAttachedFilter(parent.get(),id);if(!f)return;
   if(action=="enable"||action=="disable"){
    bool before=obs_source_enabled(f.get());obs_source_set_enabled(f.get(),action=="enable");
    record("toggle filter",id,"enabled",before,obs_source_enabled(f.get()));
   }else if(action=="up"||action=="down"){
    int before=obs_source_filter_get_index(parent.get(),f.get());
    obs_source_filter_set_order(parent.get(),f.get(),action=="up"?OBS_ORDER_MOVE_UP:OBS_ORDER_MOVE_DOWN);
    record("reorder filter",id,"index",before,obs_source_filter_get_index(parent.get(),f.get()));
   }
   else if(action=="rename"){
    bool ok=false;auto name=QInputDialog::getText(this,"Rename filter","Name",QLineEdit::Normal,QString::fromUtf8(obs_source_get_name(f.get())),&ok).trimmed();
    if(!ok||name.isEmpty())return;
    parent=inspectorSource(n,expected);if(!parent)return;f=findAttachedFilter(parent.get(),id);if(!f)return;
    Source other(obs_source_get_filter_by_name(parent.get(),name.toUtf8().constData()),obs_source_release);
    if(other&&other.get()!=f.get()){QMessageBox::warning(this,"Unscene","That filter name is already in use.");return;}
    auto before=QString::fromUtf8(obs_source_get_name(f.get()));
    obs_source_set_name(f.get(),name.toUtf8().constData());
    record("rename filter",id,"name",before,QString::fromUtf8(obs_source_get_name(f.get())));
   }else if(action=="remove"){
    if(QMessageBox::question(this,"Remove filter","Remove this shared filter from every instance of the source?",QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)!=QMessageBox::Yes)return;
    parent=inspectorSource(n,expected);if(!parent)return;f=findAttachedFilter(parent.get(),id);if(!f)return;
    auto snapshot=savedFilter(parent.get(),f.get());
    FilterUndo before{expected,n.uid,id,"present",snapshot};
    if(!FilterUndo::decode(before.encode())){QMessageBox::warning(this,"Unscene","Could not save filter for undo; filter was not removed.");return;}
    obs_source_filter_remove(parent.get(),f.get());
    if(!findAttachedFilter(parent.get(),id))registerFilterUndo("remove filter",before,{expected,n.uid,id,"absent",QJsonValue::Null});
   }else return;
  }
  thumbnails.remove(n.uid);attempted.remove(n.uid);changedScene(n.key);refreshInspectorExtras();
 }
 void closeInspector(bool animate=false){inspected.reset();inspector->finishMotion();inspector->invalidate();drawer->setOpen(false,animate?motionDuration:0);}
 bool canDiscardDraft(){
  return !inspector->dirty()||QMessageBox::question(this,"Unscene","Discard unapplied inspector changes?",QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Cancel)==QMessageBox::Discard;
 }
 void loadInspector(){
  if(!inspected||!sameCollection(inspectorCollection)){closeInspector();return;}
  auto n=*inspected;
  auto root=source(n.key.section('/',0,0));
  if(!withItem(n.parent,n.id,n.uid,[&](auto*i){
   auto*s=obs_sceneitem_get_source(i);
   inspector->load(QString::fromUtf8(obs_source_get_name(s)),root?QString::fromUtf8(obs_source_get_name(root.get())):QString(),TransformState::read(i),obs_sceneitem_locked(i));
  }))closeInspector();
  else {inspector->setVisual(n.video);refreshInspectorExtras(true);}
 }
 void inspect(const Node&n,bool explicitOpen){
  if(n.scene||stopping||!sameCollection(collection))return;
  if(!explicitOpen&&(!inspected||inspector->pinned()||inspector->dirty()))return;
  if(inspected&&inspected->key==n.key){if(explicitOpen)drawer->setOpen(true,motionDuration);return;}
  if(inspected&&inspector->dirty()&&!canDiscardDraft())return;
  inspected=n;inspectorCollection=collection;loadInspector();
  if(inspected)drawer->setOpen(true,motionDuration);
 }
 void applyInspector(){
  if(!inspected||!sameCollection(inspectorCollection))return;
  auto n=*inspected;auto next=inspector->draft();
  if(!TransformState::decode(next.encode()))return;
  bool conflict=false;
  bool found=withItem(n.parent,n.id,n.uid,[&](auto*i){
   auto old=TransformState::read(i);
   if(obs_sceneitem_locked(i)||old.encode()!=inspector->original().encode()){conflict=true;return;}
   if(old.encode()==next.encode())return;
   next.apply(i);auto actual=TransformState::read(i);
   auto undo=transformUndoData(n,inspectorCollection,old),redo=transformUndoData(n,inspectorCollection,actual);
   auto name=("Unscene: transform "+n.title).toUtf8();
   if(old.encode()!=actual.encode())obs_frontend_add_undo_redo_action(name.constData(),restoreInspectorTransform,restoreInspectorTransform,undo.constData(),redo.constData(),false);
   changedScene(n.key);
  });
  if(!found){closeInspector();return;}
  if(conflict){QMessageBox::warning(this,"Unscene","The item was locked or its transform changed elsewhere. Discard the draft to reload current values.");return;}
  loadInspector();
 }
 void refreshInspector(){
  if(!inspected)return;
  if(!sameCollection(inspectorCollection)){closeInspector();return;}
  auto n=*inspected;
  bool found=withItem(n.parent,n.id,n.uid,[&](auto*i){
   if(inspector->dirty()){inspector->setLocked(obs_sceneitem_locked(i));return;}
   auto now=TransformState::read(i);
   if(now.encode()!=inspector->original().encode())loadInspector();else inspector->setLocked(obs_sceneitem_locked(i));
  });
  if(!found)closeInspector();else refreshInspectorExtras();
 }
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
 struct RowEntry{Node node;Row*row;Reveal*shell;};
 QHash<QString,RowEntry> rowEntries;
 QVector<QString> rowOrder;
 QPointer<Row> dropMarker;
 QString dragCollection,dragKey,reorderLead;
 QTimer dragScrollTimer;
 QPoint dragPosition;
 void clearDrag(){
  dragScrollTimer.stop();
  if(dropMarker)dropMarker->setDropIndicator(0);dropMarker=nullptr;
  if(!dragKey.isEmpty()){auto it=rowEntries.find(dragKey);if(it!=rowEntries.end())it->row->cancelGesture();}
  dragKey.clear();dragCollection.clear();
 }
 int dragStep()const{
  auto*view=scroll->viewport();
  const int edge=qBound(24,fontMetrics().height()*3,64);
  return dragScrollStep(view->rect(),view->mapFromGlobal(dragPosition),edge);
 }
 bool updateDropMarker(const Node&from,QPoint pos){
  auto target=dropTarget(from,pos);
  Row*next=target&&sameCollection(dragCollection)?rowEntries.value(target->first.key).row:nullptr;
  if(dropMarker&&dropMarker!=next)dropMarker->setDropIndicator(0);
  dropMarker=next;if(next)next->setDropIndicator(target->second?-1:1);
  return next!=nullptr;
 }
 void updateDragScroll(){
  auto*bar=scroll->verticalScrollBar();const int step=dragStep();
  const bool canMove=(step<0&&bar->value()>bar->minimum())||(step>0&&bar->value()<bar->maximum());
  if(canMove){if(!dragScrollTimer.isActive())dragScrollTimer.start();}
  else dragScrollTimer.stop();
 }
 void scrollDraggedRow(){
  auto it=rowEntries.find(dragKey);
  if(stopping||!isVisible()||searching()||!sameCollection(dragCollection)||it==rowEntries.end()||!(QApplication::mouseButtons()&Qt::LeftButton)){clearDrag();return;}
  auto*bar=scroll->verticalScrollBar();
  bar->setValue(bar->value()+dragStep());
  updateDropMarker(it->node,dragPosition);updateDragScroll();
 }
 std::optional<QPair<Node,bool>> dropTarget(const Node&from,QPoint pos){
  if(from.scene||from.locked||searching()||!sameCollection(collection)||!scroll->viewport()->rect().contains(scroll->viewport()->mapFromGlobal(pos)))return {};
  for(auto&entry:rowEntries){
   const auto&n=entry.node;if(n.scene||n.key==from.key||n.parent!=from.parent)continue;
   auto*r=entry.row;if(!r->isVisible())continue;
   auto local=r->mapFromGlobal(pos);
   if(r->rect().contains(local))return QPair<Node,bool>{n,local.y()<r->height()/2};
  }return {};
 }
 bool dragMove(const Node&from,QPoint pos){
  if(stopping||searching()||from.scene||from.locked||!sameCollection(collection))return false;
  if(rows->moving())rows->finish();
  if(dragKey!=from.key){clearDrag();dragKey=from.key;dragCollection=collection;}
  dragPosition=pos;
  auto valid=updateDropMarker(from,pos);updateDragScroll();return valid;
 }
 void applySiblingOrder(const Node&n,const QString&expected,const QStringList&before,const QStringList&after){
  if(!sameCollection(expected)||before==after)return;
  auto p=source(n.parent);auto*scene=p?ownerScene(p.get()):nullptr;
  auto*i=scene?obs_scene_find_sceneitem_by_id(scene,n.id):nullptr;
  if(!i||uuid(obs_sceneitem_get_source(i))!=n.uid||obs_sceneitem_locked(i))return;
  if(!applyOrder(scene,before,after))return;
  reorderLead=n.key;
  auto undo=OrderUndo{expected,n.parent,after,before}.encode(),redo=OrderUndo{expected,n.parent,before,after}.encode();
  auto title=("Unscene: reorder "+n.title).toUtf8();
  obs_frontend_add_undo_redo_action(title.constData(),restoreOrder,restoreOrder,undo.constData(),redo.constData(),false);
  changedScene(n.key);
 }
 void finishDrag(const Node&from,QPoint pos){
  auto expected=dragCollection;auto target=dropTarget(from,pos);bool valid=dragKey==from.key&&sameCollection(expected);clearDrag();
  if(!valid||!target)return;
  auto p=source(from.parent);auto before=currentOrder(p?ownerScene(p.get()):nullptr);
  auto after=moveRelative(before,nodeOrderKey(from),nodeOrderKey(target->first),target->second);
  if(after)applySiblingOrder(from,expected,before,*after);
 }
 void changeItemState(const Node&n,const QString&expected,const QString&field,int value=-1){
  if(stopping||n.scene||!sameCollection(expected))return;
  withItem(n.parent,n.id,n.uid,[&](auto*i){
   int before=readItemState(i,field);if(before<0)return;
   int after=value<0?(before==0?1:0):value;
   if(before==after||!writeItemState(i,field,after))return;
   ItemStateUndo undo{expected,n.parent,n.uid,field,n.id,after,before},redo{expected,n.parent,n.uid,field,n.id,before,after};
   auto a=undo.encode(),b=redo.encode();
   QString label=field=="visibility"?(after?"show":"hide"):field=="locked"?(after?"lock":"unlock"):field=="scale"?"scale filtering":field=="blend"?"blending mode":"blending method";
   auto title=("Unscene: "+label+" "+n.title).toUtf8();
   obs_frontend_add_undo_redo_action(title.constData(),restoreItemState,restoreItemState,a.constData(),b.constData(),false);
   changedScene(n.key);
  });
 }
 void moveItem(const Node&n,int movement){
  if(n.scene||n.locked||!sameCollection(collection))return;
  auto p=source(n.parent);auto before=currentOrder(p?ownerScene(p.get()):nullptr);auto after=before;
  auto index=before.indexOf(nodeOrderKey(n));if(index<0)return;
  int dest=int(index);
  switch(movement){case OBS_ORDER_MOVE_UP:--dest;break;case OBS_ORDER_MOVE_DOWN:++dest;break;case OBS_ORDER_MOVE_TOP:dest=0;break;case OBS_ORDER_MOVE_BOTTOM:dest=after.size()-1;break;default:return;}
  dest=qBound(0,dest,int(after.size())-1);if(dest==index)return;
  after.move(index,dest);applySiblingOrder(n,collection,before,after);
 }
 QVector<QPointer<Reveal>> retiring;
 QVector<GlyphButton*> toolbar;
 int motionDuration=navigation.motionDuration();
 SnapshotMode snapshotMode=navigation.snapshotMode();
 bool captureAllowed()const{
  return canCapture(snapshotMode,{obs_frontend_streaming_active(),obs_frontend_recording_active(),obs_frontend_replay_buffer_active(),obs_frontend_virtualcam_active()});
 }
 bool resetRows=false,animateStructure=false;
 void finishRetiring(){
  for(auto shell:retiring)if(shell){shell->onSettled={};rows->removeWidget(shell);delete shell;}
  retiring.clear();
 }
 bool layoutMoving()const{
  if(rows->moving()||drawer->moving()||inspector->motionActive())return true;
  for(auto shell:retiring)if(shell&&shell->moving())return true;
  for(const auto&e:rowEntries)if(e.shell->moving()||e.row->motionActive())return true;
  return false;
 }
 void setMotion(int ms){
  motionDuration=ms;navigation.saveMotion(ms);inspector->setMotionDuration(ms);if(!ms&&drawer->moving())drawer->finish();
  if(!ms){rows->finish();finishRetiring();}
  for(auto&e:rowEntries){e.row->setMotionDuration(ms);if(!ms)e.shell->finish();}
  for(auto*b:toolbar)b->setMotionDuration(ms);
 }

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
  auto*captures=menu.addMenu("Snapshot capture");auto*captureGroup=new QActionGroup(captures);
  for(auto mode:{SnapshotMode::CachedOnly,SnapshotMode::IdleOnly}){
   auto*a=captures->addAction(mode==SnapshotMode::CachedOnly?"Cached artwork only (no GPU capture)":"Generate snapshots when outputs are idle");
   a->setCheckable(true);a->setChecked(mode==snapshotMode);captureGroup->addAction(a);
   connect(a,&QAction::triggered,this,[this,mode]{snapshotMode=mode;navigation.saveSnapshotMode(mode);request();});
  }
  auto*motion=menu.addMenu("Animations");auto*motionGroup=new QActionGroup(motion);
  for(int ms:{0,90,150}){auto*a=motion->addAction(ms==0?"Off":ms==90?"Fast":"Smooth");a->setCheckable(true);a->setChecked(ms==motionDuration);motionGroup->addAction(a);connect(a,&QAction::triggered,this,[this,ms]{setMotion(ms);});}
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
 void transferSource(const Node&n,const QString&expected,bool file,bool importing){
  if(stopping||editingAppearance||!sameCollection(expected))return;
  QScopedValueRollback<bool> guard(editingAppearance,true);
  if(!importing){
   QByteArray bytes;bool transitions=false;
   withItem(n.parent,n.id,n.uid,[&](auto*i){
    auto item=ItemCopy::capture(i);if(!item)return;
    transitions=obs_sceneitem_get_transition(i,true)||obs_sceneitem_get_transition(i,false);
    if(!transitions)bytes=encodePortableSource(savePortableSource(obs_sceneitem_get_source(i)),*item);
   });
   if(!decodePortableSource(bytes)){QMessageBox::warning(this,"Unscene",transitions?"Show/hide transition transfers are not supported yet.":"This source could not be serialized.");return;}
   if(QMessageBox::question(this,"Share source settings","Source settings may contain private paths, URLs or credentials. Only share with trusted recipients. Continue?",QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)!=QMessageBox::Yes)return;
   if(!file){QGuiApplication::clipboard()->setText(QString::fromUtf8(bytes));return;}
   auto path=QFileDialog::getSaveFileName(this,"Export source","source.json","JSON files (*.json)");
   if(path.isEmpty())return;QSaveFile out(path);
   if(!out.open(QIODevice::WriteOnly)||out.write(bytes)!=bytes.size()||!out.commit())QMessageBox::warning(this,"Unscene","Could not export source.");
   return;
  }
  if(!n.scene)return;
  QByteArray bytes;
  if(!file)bytes=QGuiApplication::clipboard()->text().toUtf8();
  else{
   auto path=QFileDialog::getOpenFileName(this,"Import source",QString(),"JSON files (*.json)");if(path.isEmpty())return;
   QFile f(path);if(!f.open(QIODevice::ReadOnly)||f.size()>PortableFilterMaxBytes){QMessageBox::warning(this,"Unscene","Could not read source file or it exceeds 8 MiB.");return;}
   bytes=f.read(PortableFilterMaxBytes+1);
  }
  auto payload=decodePortableSource(bytes);
  if(!payload){QMessageBox::warning(this,"Unscene","Invalid or unsupported source JSON. Nested scenes, groups and show/hide transitions are not supported yet.");return;}
  auto error=portableSourceError(payload->source);
  if(!error.isEmpty()){QMessageBox::warning(this,"Unscene",error);return;}
  auto base=payload->source["name"].toString().trimmed(),name=base;
  for(int i=2;;++i){Source used(obs_get_source_by_name(name.toUtf8().constData()),obs_source_release);if(!used)break;name=base+" "+QString::number(i);}
  QMessageBox confirm(QMessageBox::Question,"Import source","Create a new independent source? Only import settings you trust.",QMessageBox::Yes|QMessageBox::Cancel,this);
  confirm.setTextFormat(Qt::PlainText);confirm.setInformativeText(QString("%1 → %2 (%3 filter(s)). External files are not included; hotkey bindings are not copied.").arg(name,n.title).arg(payload->source["filters"].toArray().size()));confirm.setDefaultButton(QMessageBox::Cancel);
  if(confirm.exec()!=QMessageBox::Yes||!sameCollection(expected))return;
  auto parent=source(n.uid);auto*scene=parent?obs_scene_from_source(parent.get()):nullptr;
  Source collision(obs_get_source_by_name(name.toUtf8().constData()),obs_source_release);
  if(!scene||collision){QMessageBox::warning(this,"Unscene","Destination or source name changed; retry import.");return;}
  auto prepared=preparePortableSource(payload->source,name);
  auto input=loadPortableSource(prepared);
  if(!input){QMessageBox::warning(this,"Unscene","Could not load the source and its filters.");return;}
  auto*i=payload->item.paste(scene,input.get());
  if(!i){QMessageBox::warning(this,"Unscene","Could not insert source.");return;}
  auto actual=ItemCopy::capture(i);
  if(actual){
   auto snapshot=encodePortableSource(savePortableSource(input.get()),*actual);
   if(decodePortableSource(snapshot)){
    QJsonObject undo{{"collection",expected},{"parent",n.uid},{"item",QString::number(obs_sceneitem_get_id(i))},{"payload",QString::fromUtf8(snapshot)},{"remove",true}};
    auto redo=undo;redo["remove"]=false;
    auto a=QJsonDocument(undo).toJson(QJsonDocument::Compact),b=QJsonDocument(redo).toJson(QJsonDocument::Compact);
    auto title=("Unscene: import "+name).toUtf8();
    obs_frontend_add_undo_redo_action(title.constData(),restorePortableItem,restorePortableItem,a.constData(),b.constData(),false);
   }
  }
  revealScene(n);
 }
 void portableMenu(QMenu&menu,const Node&n,const QString&expected,bool importing){
  auto*sub=menu.addMenu("Portable source");
  for(bool file:{false,true}){
   auto*a=sub->addAction(importing?(file?"Import JSON…":"Paste from clipboard…"):(file?"Export JSON…":"Copy to clipboard…"));
   connect(a,&QAction::triggered,this,[this,n,expected,file,importing]{QTimer::singleShot(0,this,[this,n,expected,file,importing]{transferSource(n,expected,file,importing);});});
  }
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
 QString editingSceneUid()const{
  Source editing(obs_frontend_preview_program_mode_active()?obs_frontend_get_current_preview_scene():obs_frontend_get_current_scene(),obs_source_release);
  return uuid(editing.get());
 }
 static void nativeRows(obs_scene_t*scene,QVector<NativeRow>&out,int depth=0){
  if(!scene||depth>24)return;
  struct Ctx{QVector<NativeRow>*out;int depth;};
  Ctx ctx{&out,depth};
  obs_scene_enum_items(scene,[](obs_scene_t*parent,obs_sceneitem_t*i,void*opaque){
   auto&ctx=*static_cast<Ctx*>(opaque);
   auto*s=obs_sceneitem_get_source(i);if(!s||obs_source_removed(s))return true;
   if(obs_sceneitem_is_group(i)){
    auto*data=obs_sceneitem_get_private_settings(i);
    bool collapsed=data&&obs_data_get_bool(data,"collapsed");if(data)obs_data_release(data);
    if(!collapsed)nativeRows(obs_sceneitem_group_get_scene(i),*ctx.out,ctx.depth+1);
   }
   ctx.out->prepend({uuid(obs_scene_get_source(parent)),obs_sceneitem_get_id(i),uuid(s),QString::fromUtf8(obs_source_get_name(s))});
   return true;
  },&ctx);
 }
 bool nativeContext(const Node&n){
  if(editingAppearance||stopping||!sameCollection(collection))return false;
  const auto root=n.key.section('/',0,0);
  auto*window=static_cast<QWidget*>(obs_frontend_get_main_window());
  if(!window)return false;
  auto*scenes=window->findChild<QListWidget*>("scenes");
  if(!scenes||!scenes->currentItem())return false;
  Source nativeCurrent(obs_get_source_by_name(scenes->currentItem()->text().toUtf8().constData()),obs_source_release);
  if(!canUseNativeMenu(root,editingSceneUid(),uuid(nativeCurrent.get())))return false;
  QScopedValueRollback<bool> guard(editingAppearance,true);
  if(n.scene){
   if(window->metaObject()->indexOfMethod("on_scenes_customContextMenuRequested(QPoint)")<0)return false;
   auto*item=scenes->currentItem();
   if(scenes->selectedItems().size()!=1||!item->isSelected())return false;
   scenes->scrollToItem(item);
   const auto pos=scenes->visualItemRect(item).center();
   if(scenes->itemAt(pos)!=item)return false;
   return QMetaObject::invokeMethod(window,"on_scenes_customContextMenuRequested",Qt::DirectConnection,Q_ARG(QPoint,pos));
  }
  auto*view=window->findChild<QListView*>("sources");
  if(!view||!view->model()||!view->selectionModel()||window->metaObject()->indexOfMethod("on_sources_customContextMenuRequested(QPoint)")<0)return false;
  auto s=source(root);auto*scene=s?obs_scene_from_source(s.get()):nullptr;
  QVector<NativeRow> native;nativeRows(scene,native);
  const int row=nativeRowIndex(native,view->model(),n.parent,n.id,n.uid);
  if(row<0)return false;
  auto idx=view->model()->index(row,0);
  view->scrollTo(idx);
  const auto pos=view->visualRect(idx).center();
  if(view->indexAt(pos)!=idx)return false;
  auto parent=source(n.parent);auto*owner=parent?obs_scene_from_source(parent.get()):nullptr;
  if(!owner&&parent)owner=obs_group_from_source(parent.get());
  auto*i=owner?obs_scene_find_sceneitem_by_id(owner,n.id):nullptr;
  if(!i||uuid(obs_sceneitem_get_source(i))!=n.uid)return false;
  view->selectionModel()->setCurrentIndex(idx,QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);
  if(!obs_sceneitem_selected(i)||view->selectionModel()->selectedRows().size()!=1||view->currentIndex()!=idx)return false;
  // OBS's native menu actions use the current selection. Never open them
  // against an inactive scene or a model whose identity could not be proven.
  return QMetaObject::invokeMethod(window,"on_sources_customContextMenuRequested",Qt::DirectConnection,Q_ARG(QPoint,pos));
 }
 void queueNativeContext(const Node&n){
  const auto pos=QCursor::pos();const auto expected=collection;
  QTimer::singleShot(0,this,[this,n,pos,expected]{
   if(!sameCollection(expected))return;
   if(!nativeContext(n))contextMenu(n,pos,true);
   dirty=true;request();
  });
 }
 void addTargetActions(QMenu&menu,const Node&n,const QString&expected){
  auto src=source(n.uid);if(!src||obs_source_removed(src.get()))return;
  auto addSourceAction=[&](QMenu*where,const QString&label,std::function<void(obs_source_t*)> fn){
   auto*a=where->addAction(label);connect(a,&QAction::triggered,this,[this,n,expected,fn]{
    if(!sameCollection(expected))return;
    auto s=source(n.uid);if(s&&!obs_source_removed(s.get()))fn(s.get());
   });return a;
  };
  const auto flags=obs_source_get_output_flags(src.get());
  if(n.scene||(flags&OBS_SOURCE_VIDEO)){
   auto*project=menu.addMenu(n.scene?"Open scene projector":"Open source projector");
   auto projector=[n](obs_source_t*s,int monitor){obs_frontend_open_projector(n.scene?"Scene":"Source",monitor,nullptr,obs_source_get_name(s));};
   auto screens=QGuiApplication::screens();
   for(int i=0;i<screens.size();++i)addSourceAction(project,QString("Display %1 — %2").arg(i+1).arg(screens[i]->name()),[projector,i](auto*s){projector(s,i);});
   addSourceAction(project,"Windowed projector",[projector](auto*s){projector(s,-1);});
   addSourceAction(&menu,n.scene?"Save scene screenshot":"Save source screenshot",[](auto*s){obs_frontend_take_source_screenshot(s);});
  }
  if(n.scene){
   addSourceAction(&menu,"Filters",[](auto*s){obs_frontend_open_source_filters(s);});
   return;
  }
  if(flags&OBS_SOURCE_INTERACTION)addSourceAction(&menu,"Interact",[](auto*s){obs_frontend_open_source_interaction(s);});
  itemAction(n,[&](auto*i){
   auto itemToggle=[&](const QString&label,bool checked,const QString&field){
    auto*a=menu.addAction(label);a->setCheckable(true);a->setChecked(checked);
    connect(a,&QAction::triggered,this,[this,n,expected,field]{changeItemState(n,expected,field);});
   };
   menu.addSeparator();
   itemToggle("Visible",obs_sceneitem_visible(i),"visibility");
   itemToggle("Locked",obs_sceneitem_locked(i),"locked");
   auto*order=menu.addMenu("Order");
   for(auto c:QVector<MenuChoice>{{"Move up",OBS_ORDER_MOVE_UP},{"Move down",OBS_ORDER_MOVE_DOWN},{"Move to top",OBS_ORDER_MOVE_TOP},{"Move to bottom",OBS_ORDER_MOVE_BOTTOM}}){
    auto*a=order->addAction(c.label);a->setEnabled(!obs_sceneitem_locked(i));connect(a,&QAction::triggered,this,[this,n,expected,c]{if(sameCollection(expected))moveItem(n,c.value);});
   }
   if(!(flags&OBS_SOURCE_VIDEO))return;
   choiceMenu(&menu,"Scale filtering",scaleChoices(),obs_sceneitem_get_scale_filter(i),[this,n,expected](int v){changeItemState(n,expected,"scale",v);});
   choiceMenu(&menu,"Blending mode",blendChoices(),obs_sceneitem_get_blending_mode(i),[this,n,expected](int v){changeItemState(n,expected,"blend",v);});
   choiceMenu(&menu,"Blending method",blendMethodChoices(),obs_sceneitem_get_blending_method(i),[this,n,expected](int v){changeItemState(n,expected,"blend-method",v);});
   auto*transform=menu.addMenu("Transform");
   auto*reset=transform->addAction("Reset transform…");reset->setEnabled(!obs_sceneitem_locked(i));
   connect(reset,&QAction::triggered,this,[this,n,expected]{
    if(QMessageBox::question(this,"Reset transform","Reset position, scale, rotation, bounds and crop?",QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)!=QMessageBox::Yes)return;
    if(!sameCollection(expected))return;
    withItem(n.parent,n.id,n.uid,[&](auto*x){
     if(obs_sceneitem_locked(x))return;
     auto old=TransformState::read(x);TransformState next;next.info=resetTransform();next.apply(x);auto actual=TransformState::read(x);
     if(old.encode()==actual.encode())return;
     auto a=transformUndoData(n,expected,old),b=transformUndoData(n,expected,actual),title=("Unscene: reset transform "+n.title).toUtf8();
     obs_frontend_add_undo_redo_action(title.constData(),restoreInspectorTransform,restoreInspectorTransform,a.constData(),b.constData(),false);
     changedScene(n.key);
    });
   });
  });
 }
 void contextMenu(const Node&n,const QPoint&pos,bool nativeFallback=false){
  if(editingAppearance||stopping)return;
  auto expected=collection;
  QMenu menu(this);
  if(nativeFallback){
   const auto root=n.key.section('/',0,0);
   const bool inactive=root!=editingSceneUid();
   auto*notice=menu.addAction(inactive?"Native menu requires the editing scene":"Native menu unavailable for this row");
   notice->setEnabled(false);
   if(inactive){
    auto*activate=menu.addAction(obs_frontend_preview_program_mode_active()?"Select preview scene and open OBS menu…":"Activate scene and open OBS menu…");
    connect(activate,&QAction::triggered,this,[this,n,root,expected]{
     QTimer::singleShot(0,this,[this,n,root,expected]{
      if(!sameCollection(expected))return;
      auto s=source(root);if(!s||!obs_scene_from_source(s.get()))return;
      if(obs_frontend_preview_program_mode_active())obs_frontend_set_current_preview_scene(s.get());else obs_frontend_set_current_scene(s.get());
      QTimer::singleShot(100,this,[this,n,expected]{
       if(!sameCollection(expected))return;
       if(!nativeContext(n))QMessageBox::information(this,"Unscene","OBS's native menu is unavailable for this row. Use More actions instead.");
       dirty=true;request();
      });
     });
    });
   }
   menu.addSeparator();
  }
  if(n.scene){
   auto*add=menu.addAction("Add source…");connect(add,&QAction::triggered,this,[this,n]{QTimer::singleShot(0,this,[this,n]{addSource(n);});});
   auto*ref=menu.addAction("Paste (reference)");ref->setEnabled(canPaste(false));
   connect(ref,&QAction::triggered,this,[this,n,expected]{if(sameCollection(expected))QTimer::singleShot(0,this,[this,n,expected]{if(sameCollection(expected))pasteSource(n,false);});});
   portableMenu(menu,n,expected,true);
   auto*dup=menu.addAction("Paste (duplicate)");dup->setEnabled(canPaste(true));
   connect(dup,&QAction::triggered,this,[this,n,expected]{if(sameCollection(expected))QTimer::singleShot(0,this,[this,n,expected]{if(sameCollection(expected))pasteSource(n,true);});});
  }else{
   auto*props=menu.addAction("Properties");connect(props,&QAction::triggered,this,[n]{auto s=source(n.uid);if(s)obs_frontend_open_source_properties(s.get());});
   auto*filters=menu.addAction("Filters");connect(filters,&QAction::triggered,this,[n]{auto s=source(n.uid);if(s)obs_frontend_open_source_filters(s.get());});
   {auto*a=menu.addAction("Open inspector");connect(a,&QAction::triggered,this,[this,n,expected]{if(sameCollection(expected))QTimer::singleShot(0,this,[this,n,expected]{if(sameCollection(expected))inspect(n,true);});});}
   auto*transform=menu.addAction("Edit transform");transform->setEnabled(n.key.section('/',0,0)==editingSceneUid());connect(transform,&QAction::triggered,this,[this,n,expected]{if(sameCollection(expected))itemAction(n,[](auto*i){obs_frontend_open_sceneitem_edit_transform(i);});});
   if(!n.group){
    menu.addSeparator();
    auto*copy=menu.addAction("Copy source");auto input=source(n.uid);copy->setEnabled(input&&!obs_source_is_scene(input.get()));connect(copy,&QAction::triggered,this,[this,n,expected]{if(sameCollection(expected))itemAction(n,[this](auto*i){copied=ItemCopy::capture(i);if(copied)obs_frontend_copy_sceneitem(i);});});
    if(input&&!obs_source_is_scene(input.get()))portableMenu(menu,n,expected,false);
    auto*remove=menu.addAction("Remove from scene…");connect(remove,&QAction::triggered,this,[this,n,expected]{if(sameCollection(expected))QTimer::singleShot(0,this,[this,n]{removeItem(n);});});
   }
  }
  addTargetActions(menu,n,expected);
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
  if(!isVisible()||!dragKey.isEmpty()||layoutMoving()||!captureAllowed())return;
  for(const auto&n:previous){
   auto label=labels.value(n.key);
   if(!snapshotKeys.contains(n.key)||artworkOverrides.contains(n.uid)||!n.video||!label||thumbnails.contains(n.uid)||!scroll->viewport()->rect().intersects(QRect(label->mapTo(scroll->viewport(),QPoint()),label->size())))continue;
   auto s=source(n.uid);if(!s||!obs_source_showing(s.get()))continue;
   if(attempted.contains(n.uid)&&clock.elapsed()-attempted.value(n.uid)<3000)continue;
   attempted.insert(n.uid,clock.elapsed());
   if(!captureAllowed())return;
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
  animateStructure=true;
  if((n.scene||n.group)&&!searching()){if(opened.contains(n.key))opened.remove(n.key);else opened.insert(n.key);saveOpened();}
  if(!n.scene){selected=selected==n.key?QString():n.key;if(!selected.isEmpty())inspect(n,false);}
  dirty=true;request();
 }
 void render(const QVector<Node>&nodes){
  clearDrag();
  const auto origins=rows->positions();
  bool reorder=motionDuration&&!resetRows&&!searching()&&retiring.isEmpty()&&nodes.size()==rowOrder.size()&&nodes.size()<=96;
  bool changedOrder=false;
  if(reorder)for(int i=0;i<nodes.size();++i){
   const auto&n=nodes[i];auto it=rowEntries.find(n.key);
   if(it==rowEntries.end()||!(it->node==n)||it->shell->moving()||it->row->motionActive()){reorder=false;break;}
   changedOrder|=rowOrder[i]!=n.key;
  }
  reorder=reorder&&changedOrder;
  rows->finish();
  const int pos=scroll->verticalScrollBar()->value();
  labels.clear();keys.clear();artworkOverrides.clear();snapshotKeys.clear();
  finishRetiring();
  int changes=0;QSet<QString> nextKeys;
  for(const auto&n:nodes){nextKeys.insert(n.key);if(!rowEntries.contains(n.key))++changes;}
  for(auto it=rowEntries.cbegin();it!=rowEntries.cend();++it)if(!nextKeys.contains(it.key()))++changes;
  const bool animate=animateStructure&&!resetRows&&motionDuration&&changes<=24&&nodes.size()+rowEntries.size()<=96;
  animateStructure=false;
  QSet<QString> incoming;for(const auto&n:nodes)incoming.insert(n.key);
  QHash<QString,Reveal*> exits;
  for(auto it=rowEntries.begin();it!=rowEntries.end();){
   if(resetRows||!incoming.contains(it.key())){
    auto*shell=it->shell;
    if(animate){exits.insert(it.key(),shell);shell->setEnabled(false);}
    else{rows->removeWidget(shell);delete shell;}
    it=rowEntries.erase(it);
   }else ++it;
  }
  resetRows=false;
  while(auto*it=rows->takeAt(0)){auto*w=it->widget();if(w&&w->property("unsceneEmpty").toBool())delete w;delete it;}
  QVector<QString> nextOrder;
  int index=0;
  auto insertExitsBefore=[&](const QString&key){
   if(rowOrder.indexOf(key,index)<0)return;
   while(index<rowOrder.size()&&rowOrder[index]!=key){auto*shell=exits.take(rowOrder[index++]);if(shell)rows->addWidget(shell);}
   if(index<rowOrder.size())++index;
  };
  for(const auto&n:nodes){
   const bool isOpen=searching()||opened.contains(n.key), expanded=selected==n.key;
   auto found=rowEntries.find(n.key);
   bool fresh=found==rowEntries.end(),created=fresh;
   if(!fresh&&!(found->node==n)){rows->removeWidget(found->shell);delete found->shell;rowEntries.erase(found);fresh=true;}
   Row*r;Reveal*shell;
   if(fresh){
    r=new Row(n.title,n.scene,expanded,n.depth);
    shell=new Reveal(r,body,true);
    rowEntries.insert(n.key,{n,r,shell});
   }else{r=found->row;shell=found->shell;}
   r->setMotionDuration(fresh?0:motionDuration);
   r->setExpanded(expanded);
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
   if(fresh){
   r->onAppearance=[this,n]{queueNativeContext(n);};
   r->onBody=[this,n]{toggle(n);};
   if(!n.scene&&!n.locked){
    r->onDragMove=[this,n](QPoint pos){return dragMove(n,pos);};
    r->onDrop=[this,n](QPoint pos){finishDrag(n,pos);};
    r->onDragCancel=[this]{clearDrag();};
   }
   if(n.scene){
    r->addHover(obs_frontend_preview_program_mode_active()?"Select preview scene":"Activate scene","arrow-right",[this,n]{auto s=source(n.uid);if(!s||!obs_scene_from_source(s.get()))return;if(obs_frontend_preview_program_mode_active())obs_frontend_set_current_preview_scene(s.get());else obs_frontend_set_current_scene(s.get());request();});
    r->addHover("Add source","plus",[this,n]{QTimer::singleShot(0,this,[this,n]{addSource(n);});});
    r->addHover("More actions","more",[this,n]{queueContext(n);});
   }else{
    r->addHover("Properties","properties",[n]{auto s=source(n.uid);if(s)obs_frontend_open_source_properties(s.get());});
    r->addHover("Filters","filters",[n]{auto s=source(n.uid);if(s)obs_frontend_open_source_filters(s.get());});
    r->addHover(n.visible?"Hide source":"Show source",n.visible?"eye":"eye-off",[this,n]{changeItemState(n,collection,"visibility");},!n.visible);
    r->addSecondary("Inspector","inspector",[this,n]{inspect(n,true);});
    r->addSecondary(n.locked?"Unlock source":"Lock source",n.locked?"lock":"unlock",[this,n]{changeItemState(n,collection,"locked");});
    r->addSecondary("Move up","up",[this,n]{moveItem(n,OBS_ORDER_MOVE_UP);});
    r->addSecondary("Move down","down",[this,n]{moveItem(n,OBS_ORDER_MOVE_DOWN);});
    r->addSecondary("More actions","more",[this,n]{queueContext(n);});
    if(n.audio)r->addSecondary("Toggle mute",n.muted?"mute":"volume",[n]{auto s=source(n.uid);if(s)obs_source_set_muted(s.get(),!obs_source_muted(s.get()));});
   }
   }
   r->setMotionDuration(motionDuration);
   if(animate&&!created)insertExitsBefore(n.key);
   rows->addWidget(shell);
   if(animate&&created){shell->setOpen(false,0);shell->setOpen(true,motionDuration);}
   else if(created)shell->show();
   else if(!shell->isOpen())shell->setOpen(true,0);
   nextOrder.append(n.key);
  }
  for(const auto&key:rowOrder)if(auto*shell=exits.take(key))rows->addWidget(shell);
  for(auto*shell:exits)rows->addWidget(shell);
  // Exiting rows remain clipped at their old position until the finite animation ends.
  for(int i=0;i<rows->count();++i){
   auto*shell=dynamic_cast<Reveal*>(rows->itemAt(i)->widget());
   if(!shell||shell->isEnabled())continue;
   retiring.append(shell);
   shell->onSettled=[shell](bool open){if(!open)shell->deleteLater();};
   shell->setOpen(false,motionDuration);
  }
  rowOrder=nextOrder;
  if(nodes.isEmpty()){
   auto*empty=new QLabel(searching()?"No matching scenes or sources":"No scenes",body);empty->setProperty("unsceneEmpty",true);empty->setAlignment(Qt::AlignCenter);empty->setContentsMargins(4,12,4,12);rows->addWidget(empty);
  }
  rows->addStretch(); scroll->verticalScrollBar()->setValue(pos);
  if(reorder){
   rows->activate();auto preferred=rowEntries.find(reorderLead.isEmpty()?selected:reorderLead);
   rows->animateFrom(origins,motionDuration,preferred==rowEntries.end()?nullptr:preferred->shell);
  }
  reorderLead.clear();
 }
public:
 Dock(){
  setObjectName("UnsceneContent");
  nativeList=new QListView(this);nativeList->hide();nativeList->ensurePolished();
  auto*l=new QVBoxLayout(this);dockLayout=l;
  bar=new QToolBar(this);bar->setMovable(false);bar->setFloatable(false); collapse=new GlyphButton("collapse-all",bar);collapse->setToolTip("Collapse all scenes");
  expand=new GlyphButton("expand-all",bar);expand->setToolTip("Expand all scenes");
  auto*refreshButton=new GlyphButton("refresh",bar);refreshButton->setToolTip("Refresh snapshots (when idle capture is enabled and outputs are idle)");
  connect(refreshButton,&QToolButton::clicked,this,[this,refreshButton]{refreshButton->spin();if(!captureAllowed())return;thumbnails.clear();attempted.clear();dirty=true;request();});
  auto*settingsButton=new GlyphButton("properties",bar);settingsButton->setToolTip("Dock settings");settingsButton->setAccessibleName("Dock settings");
  toolbar={refreshButton,collapse,expand,settingsButton};for(auto*b:toolbar)b->setMotionDuration(motionDuration);
  connect(settingsButton,&QToolButton::clicked,this,[this,settingsButton]{densityMenu(settingsButton);});
  search=new QLineEdit(bar);search->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);search->setPlaceholderText("Find scenes or sources…");search->setClearButtonEnabled(true);search->setAccessibleName("Find scenes or sources");search->setMinimumWidth(50);
  bar->addWidget(search);bar->addWidget(refreshButton);bar->addWidget(collapse);bar->addWidget(expand);bar->addWidget(settingsButton);l->addWidget(bar);
  connect(search,&QLineEdit::textChanged,this,[this](const QString&t){query=t;animateStructure=true;collapse->setEnabled(!searching());expand->setEnabled(!searching());dirty=true;request();});
  scroll=new QScrollArea(this);scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);body=new QWidget;rows=new ReorderLayout(body);rows->setContentsMargins(0,0,0,0);rows->setSpacing(1);scroll->setWidget(body);
  splitter=new QSplitter(Qt::Vertical,this);splitter->setChildrenCollapsible(false);splitter->addWidget(scroll);
  inspector=new Inspector(configPath("transform-presets.ini"));drawer=new InspectorDrawer(inspector,splitter);splitter->addWidget(drawer);inspector->setMotionDuration(motionDuration);l->addWidget(splitter,1);
  inspector->onUsageRefresh=[this]{refreshInspectorExtras(true);};
  inspector->onFilterAction=[this](QString action,QString id){if(inspected){auto n=*inspected;auto expected=inspectorCollection;QTimer::singleShot(0,this,[this,n,expected,action,id]{filterAction(n,expected,action,id);});}};
  inspector->onClose=[this]{if(canDiscardDraft())closeInspector(true);};
  inspector->onApply=[this]{applyInspector();};
  inspector->onDiscard=[this]{loadInspector();};
  inspector->onProperties=[this]{if(inspected&&sameCollection(inspectorCollection)){auto s=source(inspected->uid);if(s)obs_frontend_open_source_properties(s.get());}};
  inspector->onFilters=[this]{if(inspected&&sameCollection(inspectorCollection)){auto s=source(inspected->uid);if(s)obs_frontend_open_source_filters(s.get());}};
  themeGeometry();
  connect(collapse,&QToolButton::clicked,this,[this]{animateStructure=true;opened.clear();selected.clear();saveOpened();dirty=true;request();});
  connect(expand,&QToolButton::clicked,this,[this]{animateStructure=true;opened.unite(sceneKeys);saveOpened();dirty=true;request();});
  dragScrollTimer.setInterval(25);connect(&dragScrollTimer,&QTimer::timeout,this,[this]{scrollDraggedRow();});
  connect(&timer,&QTimer::timeout,this,[this]{refresh();});clock.start();timer.start(400);
 }
 void themeGeometry(){
  int h=themeSpacing(this,Qt::Horizontal),v=themeSpacing(this,Qt::Vertical);
  dockLayout->setContentsMargins(qMax(1,h/2),qMax(1,v/2),qMax(1,h/2),qMax(1,v/2));dockLayout->setSpacing(qMax(1,v/2));
  rows->setSpacing(qMax(0,nativeList->spacing()));search->setMinimumWidth(search->fontMetrics().horizontalAdvance("MMMM"));
  bar->updateGeometry();
 }
 void themeChanged(){if(drawer->moving())drawer->finish();inspector->finishMotion();themeGeometry();resetRows=true;dirty=true;request();}
 void hideEvent(QHideEvent*e)override{clearDrag();rows->finish();if(drawer->moving())drawer->finish();inspector->finishMotion();QWidget::hideEvent(e);}
 void stop(){stopping=true;clearDrag();rows->finish();if(drawer->moving())drawer->finish();inspector->finishMotion();timer.stop();}
 void refresh(){
  if(stopping||editingAppearance||!isVisible())return;
  char *raw=obs_frontend_get_current_scene_collection();QString now=QString::fromUtf8(raw?raw:"");bfree(raw);
  if(collection!=now){closeInspector();resetRows=true;collection=now;copied.reset();thumbnails.clear();attempted.clear();opened.clear();selected.clear();initialized=false;dirty=true;}
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
  refreshInspector();updateSnapshots();
 }
};
QPointer<Dock> dock;
void event(obs_frontend_event e,void*){if(!dock)return;if(e==OBS_FRONTEND_EVENT_EXIT)dock->stop();else if(e==OBS_FRONTEND_EVENT_THEME_CHANGED)dock->themeChanged();}
}
bool obs_module_load(void){dock=new Dock;if(!obs_frontend_add_dock_by_id("unscene.dock","Unscene",dock)){delete dock;return false;}obs_frontend_add_event_callback(event,nullptr);return true;}
void obs_module_unload(void){obs_frontend_remove_event_callback(event,nullptr);if(dock){dock->stop();obs_frontend_remove_dock("unscene.dock");}}
