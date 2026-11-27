#pragma once
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QSettings>
#include <QUuid>
#include <QVector>

struct SceneFolder { QString id,name,parent; };

// Organization only: folder IDs never represent OBS sources or scene items.
class SceneFolders {
 QVector<SceneFolder> entries;
 QMap<QString,QString> membership;
 QMap<QString,QStringList> sceneOrder;
 int index(const QString&id)const{
  for(int i=0;i<entries.size();++i)if(entries[i].id==id)return i;
  return -1;
 }
 bool validParent(const QString&id)const{return id.isEmpty()||index(id)>=0;}
 bool validTree()const{
  for(const auto&f:entries){
   QSet<QString> seen;auto id=f.id;
   while(!id.isEmpty()){
    if(seen.contains(id)||seen.size()>=8||index(id)<0)return false;
    seen.insert(id);id=parentOfFolder(id);
   }
  }
  return true;
 }
public:
 const QVector<SceneFolder>& folders()const{return entries;}
 QString parentOfFolder(const QString&id)const{int i=index(id);return i<0?QString():entries[i].parent;}
 QString parentOfScene(const QString&id)const{return membership.value(id);}
 bool contains(const QString&id)const{return index(id)>=0;}
 bool validTab(const QString&id)const{return id.isEmpty()||(contains(id)&&parentOfFolder(id).isEmpty());}
 QStringList rootTabs()const{
  QStringList result;for(const auto&f:entries)if(f.parent.isEmpty())result.append(f.id);return result;
 }
 bool reorderTabs(const QStringList&order){
  auto current=rootTabs();
  if(order.size()!=current.size()||QSet<QString>(order.begin(),order.end())!=QSet<QString>(current.begin(),current.end()))return false;
  auto old=entries;int i=0;
  for(auto&f:entries)if(f.parent.isEmpty()){
   auto id=order[i++];for(const auto&candidate:old)if(candidate.id==id){f=candidate;break;}
  }
  return true;
 }
 QStringList orderedScenes(const QString&parent,const QStringList&native)const{
  QStringList result;QSet<QString> seen;
  for(const auto&id:sceneOrder.value(parent))if(native.contains(id)&&!seen.contains(id)){result.append(id);seen.insert(id);}
  for(const auto&id:native)if(!seen.contains(id)){result.append(id);seen.insert(id);}
  return result;
 }
 bool reorderScenes(const QString&parent,const QStringList&order){
  if(!validParent(parent)||order.contains(QString())||QSet<QString>(order.begin(),order.end()).size()!=order.size())return false;
  if(order.isEmpty())sceneOrder.remove(parent);else sceneOrder[parent]=order;return true;
 }
 QString rootForFolder(const QString&id)const{
  auto current=id;
  while(!current.isEmpty()&&!parentOfFolder(current).isEmpty())current=parentOfFolder(current);
  return current;
 }
 QString rootForScene(const QString&id)const{return rootForFolder(parentOfScene(id));}
 QString duplicateStructure(const QString&id,const QString&name){
  if(id.isEmpty()||!validTab(id))return {};
  const auto original=entries;SceneFolders next=*this;
  auto root=next.add(name,{});if(root.isEmpty())return {};
  QMap<QString,QString> mapping{{id,root}};
  for(int level=0;level<8;++level){
   for(const auto&f:original)if(!mapping.contains(f.id)&&mapping.contains(f.parent)){
    auto child=next.add(f.name,mapping.value(f.parent));if(child.isEmpty())return {};
    mapping.insert(f.id,child);
   }
  }
  *this=next;return root;
 }
 QString path(const QString&id)const{
  QStringList parts;auto current=id;
  for(int i=0;i<8&&!current.isEmpty();++i){int at=index(current);if(at<0)break;parts.prepend(entries[at].name);current=entries[at].parent;}
  return parts.join(" / ");
 }
 QString add(const QString&name,const QString&parent){
  if(name.trimmed().isEmpty()||!validParent(parent)||entries.size()>=256)return {};
  auto id="folder:"+QUuid::createUuid().toString(QUuid::WithoutBraces);
  entries.append({id,name.trimmed(),parent});
  if(!validTree()){entries.removeLast();return {};}return id;
 }
 bool rename(const QString&id,const QString&name){
  int i=index(id);if(i<0||name.trimmed().isEmpty())return false;
  entries[i].name=name.trimmed();return true;
 }
 bool moveFolder(const QString&id,const QString&parent){
  int i=index(id);if(i<0||!validParent(parent)||id==parent)return false;
  auto previous=entries[i].parent;entries[i].parent=parent;
  if(!validTree()){entries[i].parent=previous;return false;}return true;
 }
 bool moveScene(const QString&id,const QString&parent){
  if(id.isEmpty()||!validParent(parent))return false;
  auto old=parentOfScene(id);if(old!=parent&&sceneOrder.contains(old)){sceneOrder[old].removeAll(id);if(sceneOrder[old].isEmpty())sceneOrder.remove(old);}
  if(parent.isEmpty())membership.remove(id);else membership[id]=parent;return true;
 }
 bool remove(const QString&id){
  int i=index(id);if(i<0)return false;auto parent=entries[i].parent;entries.removeAt(i);sceneOrder.remove(id);
  for(auto&f:entries)if(f.parent==id)f.parent=parent;
  for(auto it=membership.begin();it!=membership.end();){
   if(it.value()!=id){++it;continue;}
   if(parent.isEmpty())it=membership.erase(it);else{it.value()=parent;++it;}
  }return true;
 }
 QByteArray encode()const{
  QJsonArray folders;QJsonObject scenes,order;
  for(const auto&f:entries)folders.append(QJsonObject{{"id",f.id},{"name",f.name},{"parent",f.parent}});
  for(auto it=membership.cbegin();it!=membership.cend();++it)scenes.insert(it.key(),it.value());
  for(auto it=sceneOrder.cbegin();it!=sceneOrder.cend();++it)order.insert(it.key(),QJsonArray::fromStringList(it.value()));
  QJsonObject result{{"folders",folders},{"scenes",scenes}};if(!order.isEmpty())result.insert("sceneOrder",order);
  return QJsonDocument(result).toJson(QJsonDocument::Compact);
 }
 static SceneFolders decode(const QByteArray&bytes){
  SceneFolders result;auto obj=QJsonDocument::fromJson(bytes).object();auto folders=obj["folders"].toArray();
  if(folders.size()>256)return {};
  for(const auto&v:folders){auto f=v.toObject();auto id=f["id"].toString(),name=f["name"].toString(),parent=f["parent"].toString();
   if(!id.startsWith("folder:")||id.contains('/')||name.trimmed().isEmpty()||result.contains(id))return {};
   result.entries.append({id,name,parent});
  }
  if(!result.validTree())return {};
  auto scenes=obj["scenes"].toObject();
  for(auto it=scenes.begin();it!=scenes.end();++it)if(result.contains(it.value().toString()))result.moveScene(it.key(),it.value().toString());
  auto orders=obj["sceneOrder"].toObject();
  for(auto it=orders.begin();it!=orders.end();++it)if(it.value().isArray()){
   QStringList ids;for(auto v:it.value().toArray())ids.append(v.toString());
   result.reorderScenes(it.key(),ids);
  }
  return result;
 }
};

// A separate, bounded history of presentation edits; never enters OBS output undo.
class FolderHistory {
 struct Entry{QByteArray before,after;};
 QVector<Entry> undo,redo;
public:
 void clear(){undo.clear();redo.clear();}
 bool canUndo()const{return !undo.isEmpty();}
 bool canRedo()const{return !redo.isEmpty();}
 void record(const QByteArray&before,const QByteArray&after){
  if(before==after)return;
  redo.clear();undo.append({before,after});if(undo.size()>64)undo.removeFirst();
 }
 QByteArray undoTarget(const QByteArray&current)const{return canUndo()&&undo.last().after==current?undo.last().before:QByteArray();}
 QByteArray redoTarget(const QByteArray&current)const{return canRedo()&&redo.last().before==current?redo.last().after:QByteArray();}
 void didUndo(){if(canUndo())redo.append(undo.takeLast());}
 void didRedo(){if(canRedo())undo.append(redo.takeLast());}
};

class SceneFolderStore {
 QSettings settings;
 static QString key(const QString&collection){return "collections/"+QString::fromLatin1(collection.toUtf8().toHex())+"/folders";}
public:
 explicit SceneFolderStore(const QString&file):settings(file,QSettings::IniFormat){}
 QString tab(const QString&collection)const{return settings.value(key(collection)+"-active-tab").toString();}
 bool saveTab(const QString&collection,const QString&id){
  if(collection.isEmpty())return false;
  settings.setValue(key(collection)+"-active-tab",id);settings.sync();return settings.status()==QSettings::NoError;
 }
 SceneFolders load(const QString&collection)const{return SceneFolders::decode(settings.value(key(collection)).toByteArray());}
 bool save(const QString&collection,const SceneFolders&folders){
  if(collection.isEmpty())return false;
  settings.setValue(key(collection),folders.encode());settings.sync();return settings.status()==QSettings::NoError;
 }
};
