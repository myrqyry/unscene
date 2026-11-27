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
  if(parent.isEmpty())membership.remove(id);else membership[id]=parent;return true;
 }
 bool remove(const QString&id){
  int i=index(id);if(i<0)return false;auto parent=entries[i].parent;entries.removeAt(i);
  for(auto&f:entries)if(f.parent==id)f.parent=parent;
  for(auto it=membership.begin();it!=membership.end();){
   if(it.value()!=id){++it;continue;}
   if(parent.isEmpty())it=membership.erase(it);else{it.value()=parent;++it;}
  }return true;
 }
 QByteArray encode()const{
  QJsonArray folders;QJsonObject scenes;
  for(const auto&f:entries)folders.append(QJsonObject{{"id",f.id},{"name",f.name},{"parent",f.parent}});
  for(auto it=membership.cbegin();it!=membership.cend();++it)scenes.insert(it.key(),it.value());
  return QJsonDocument(QJsonObject{{"folders",folders},{"scenes",scenes}}).toJson(QJsonDocument::Compact);
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
  return result;
 }
};

class SceneFolderStore {
 QSettings settings;
 static QString key(const QString&collection){return "collections/"+QString::fromLatin1(collection.toUtf8().toHex())+"/folders";}
public:
 explicit SceneFolderStore(const QString&file):settings(file,QSettings::IniFormat){}
 SceneFolders load(const QString&collection)const{return SceneFolders::decode(settings.value(key(collection)).toByteArray());}
 bool save(const QString&collection,const SceneFolders&folders){
  if(collection.isEmpty())return false;
  settings.setValue(key(collection),folders.encode());settings.sync();return settings.status()==QSettings::NoError;
 }
};
