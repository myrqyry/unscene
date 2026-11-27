#pragma once
#include <QVector>
#include <QSet>
#include <QSettings>
#include <QSize>
#include <QStringList>
#include <optional>
#include <algorithm>
#include "snapshot-policy.hpp"

enum class Density {Compact,Normal,Large};
inline QSize thumbnailSize(Density d,bool scene){
 if(d==Density::Compact)return {20,20};
 if(d==Density::Large)return scene?QSize(80,45):QSize(56,32);
 return scene?QSize(48,27):QSize(36,22);
}
template<class Node> QVector<Node> filterHierarchy(const QVector<Node>&nodes,const QString&raw){
 const auto query=raw.trimmed();
 if(query.isEmpty())return nodes;
 QVector<bool> keep(nodes.size(),false);
 QVector<qsizetype> ancestors;
 int matchingDepth=-1;
 for(qsizetype i=0;i<nodes.size();++i){
  const auto&n=nodes[i];
  while(!ancestors.isEmpty()&&nodes[ancestors.last()].depth>=n.depth)ancestors.removeLast();
  if(matchingDepth>=n.depth)matchingDepth=-1;
  if(n.title.contains(query,Qt::CaseInsensitive)){
   if(matchingDepth<0)matchingDepth=n.depth;keep[i]=true;
   for(auto a:ancestors)keep[a]=true;
  }else if(matchingDepth>=0)keep[i]=true;
  ancestors.append(i);
 }
 QVector<Node> result;
 for(qsizetype i=0;i<nodes.size();++i)if(keep[i])result.append(nodes[i]);
 return result;
}
inline QStringList favoriteFirst(QStringList ids,const QSet<QString>&favorites){
 std::stable_partition(ids.begin(),ids.end(),[&](const QString&id){return favorites.contains(id);});return ids;
}
class NavigationStore {
 QSettings settings;
 static QString key(const QString&collection){return "collections/"+QString::fromLatin1(collection.toUtf8().toHex())+"/expanded";}
 Density density(const char*key)const{
  auto d=settings.value(key,1).toInt();
  return d==0?Density::Compact:d==2?Density::Large:Density::Normal;
 }
public:
 explicit NavigationStore(const QString&path):settings(path,QSettings::IniFormat){}
 std::optional<QSet<QString>> opened(const QString&collection)const{
  if(collection.isEmpty()||!settings.contains(key(collection)))return std::nullopt;
  auto list=settings.value(key(collection)).toStringList();
  return QSet<QString>(list.begin(),list.end());
 }
 bool saveOpened(const QString&collection,const QSet<QString>&opened){
  if(collection.isEmpty())return false;
  auto list=opened.values();std::sort(list.begin(),list.end());
  settings.setValue(key(collection),list);settings.sync();return settings.status()==QSettings::NoError;
 }
 QSet<QString> favorites(const QString&collection)const{
  auto ids=settings.value(key(collection)+"-favorites").toStringList();return QSet<QString>(ids.begin(),ids.end());
 }
 bool saveFavorites(const QString&collection,const QSet<QString>&favorites){
  if(collection.isEmpty())return false;auto ids=favorites.values();std::sort(ids.begin(),ids.end());
  settings.setValue(key(collection)+"-favorites",ids);settings.sync();return settings.status()==QSettings::NoError;
 }
 bool searchAllTabs()const{return settings.value("search/allTabs",false).toBool();}
 bool saveSearchAllTabs(bool value){settings.setValue("search/allTabs",value);settings.sync();return settings.status()==QSettings::NoError;}
 SnapshotMode snapshotMode()const{return settings.value("snapshots/idleCapture",false).toBool()?SnapshotMode::IdleOnly:SnapshotMode::CachedOnly;}
 bool saveSnapshotMode(SnapshotMode mode){settings.setValue("snapshots/idleCapture",mode==SnapshotMode::IdleOnly);settings.sync();return settings.status()==QSettings::NoError;}
 int motionDuration()const{int v=settings.value("motion/duration",150).toInt();return v==0?0:v==90?90:150;}
 bool saveMotion(int ms){settings.setValue("motion/duration",ms==0?0:ms==90?90:150);settings.sync();return settings.status()==QSettings::NoError;}
 Density sceneDensity()const{return density("density/scenes");}
 Density sourceDensity()const{return density("density/sources");}
 bool saveDensity(Density scenes,Density sources){
  settings.setValue("density/scenes",int(scenes));settings.setValue("density/sources",int(sources));settings.sync();return settings.status()==QSettings::NoError;
 }
};
