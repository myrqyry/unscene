#pragma once
#include <QStringList>
#include <QSet>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <optional>
inline bool sameOrderMembers(const QStringList&a,const QStringList&b){
 if(a.size()!=b.size()||a.isEmpty()||a.contains(QString())||b.contains(QString()))return false;
 QSet<QString> aa(a.begin(),a.end()),bb(b.begin(),b.end());
 return aa.size()==a.size()&&bb.size()==b.size()&&aa==bb;
}
inline std::optional<QStringList> moveRelative(QStringList rows,const QString&from,const QString&target,bool before){
 if(from==target||!rows.contains(from)||!rows.contains(target)||!sameOrderMembers(rows,rows))return {};
 rows.removeOne(from);rows.insert(rows.indexOf(target)+(before?0:1),from);return rows;
}
struct OrderUndo {
 QString collection,parent;QStringList expected,next;
 QByteArray encode()const{return QJsonDocument(QJsonObject{{"collection",collection},{"parent",parent},{"expected",QJsonArray::fromStringList(expected)},{"next",QJsonArray::fromStringList(next)}}).toJson(QJsonDocument::Compact);}
 static std::optional<OrderUndo> decode(const QByteArray&bytes){
  auto obj=QJsonDocument::fromJson(bytes).object();OrderUndo u;u.collection=obj["collection"].toString();u.parent=obj["parent"].toString();
  if(u.collection.isEmpty()||u.parent.isEmpty()||!obj["expected"].isArray()||!obj["next"].isArray())return {};
  for(auto v:obj["expected"].toArray()){if(!v.isString())return {};u.expected<<v.toString();}
  for(auto v:obj["next"].toArray()){if(!v.isString())return {};u.next<<v.toString();}
  if(!sameOrderMembers(u.expected,u.next))return {};
  return u;
 }
};
