#pragma once
#include <QJsonDocument>
#include <QJsonObject>
#include <optional>
#include <cmath>

struct FilterUndo {
 QString collection,parent,filter,operation;
 QJsonValue value;
 QByteArray encode()const{
  return QJsonDocument(QJsonObject{{"collection",collection},{"parent",parent},{"filter",filter},{"operation",operation},{"value",value}}).toJson(QJsonDocument::Compact);
 }
 static bool indexValid(const QJsonValue&v){return v.isDouble()&&v.toDouble()>=0&&v.toDouble()==v.toInt(-1);}
 static std::optional<FilterUndo> decode(const QByteArray&bytes){
  auto doc=QJsonDocument::fromJson(bytes);if(!doc.isObject())return {};
  auto o=doc.object();FilterUndo u{o["collection"].toString(),o["parent"].toString(),o["filter"].toString(),o["operation"].toString(),o["value"]};
  if(u.collection.isEmpty()||u.parent.isEmpty()||u.filter.isEmpty())return {};
  if(u.operation=="enabled"&&u.value.isBool())return u;
  if(u.operation=="name"&&u.value.isString()&&!u.value.toString().trimmed().isEmpty())return u;
  if(u.operation=="index"&&indexValid(u.value))return u;
  if(u.operation=="absent"&&u.value.isNull())return u;
  if(u.operation=="present"&&u.value.isObject()){
   auto v=u.value.toObject();auto s=v["source"].toObject();
   if(indexValid(v["index"])&&s["uuid"].toString()==u.filter&&!s["name"].toString().isEmpty()&&!s["id"].toString().isEmpty()&&s["settings"].isObject())return u;
  }
  return {};
 }
};
