#pragma once
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QUuid>
#include <optional>

inline constexpr int PortableFilterMaxBytes=8*1024*1024;
inline constexpr int PortableFilterMaxCount=128;
inline QByteArray encodePortableFilters(const QJsonArray&filters){
 // Single filter uses native OBS saved-source JSON, interoperable with Source Copy.
 if(filters.size()==1)return QJsonDocument(filters[0].toObject()).toJson(QJsonDocument::Indented);
 return QJsonDocument(QJsonObject{{"format","unscene.filters"},{"version",1},{"filters",filters}}).toJson(QJsonDocument::Indented);
}
inline std::optional<QJsonArray> decodePortableFilters(const QByteArray&bytes){
 if(bytes.isEmpty()||bytes.size()>PortableFilterMaxBytes)return {};
 auto doc=QJsonDocument::fromJson(bytes);if(!doc.isObject())return {};
 auto obj=doc.object();QJsonArray filters;
 if(obj.contains("format")){
  if(obj["format"]!="unscene.filters"||obj["version"]!=1||!obj["filters"].isArray())return {};
  filters=obj["filters"].toArray();
 }else filters.append(obj);
 if(filters.isEmpty()||filters.size()>PortableFilterMaxCount)return {};
 for(auto v:filters){
  if(!v.isObject())return {};
  auto f=v.toObject();auto name=f["name"].toString(),id=f["id"].toString();
  if(name.trimmed().isEmpty()||name.size()>512||id.isEmpty()||id.size()>256||name.contains(QChar(0))||id.contains(QChar(0))||!f["settings"].isObject())return {};
  if(f.contains("enabled")&&!f["enabled"].isBool())return {};
  if(f.contains("versioned_id")&&(!f["versioned_id"].isString()||f["versioned_id"].toString().contains(QChar(0))))return {};
  if(f.contains("filters")&&(!f["filters"].isArray()||!f["filters"].toArray().isEmpty()))return {};
 }
 return filters;
}
inline QJsonObject preparePortableFilter(QJsonObject f,QSet<QString>&names){
 auto base=f["name"].toString().trimmed(),name=base;
 for(int i=2;names.contains(name);++i)name=base+" "+QString::number(i);
 names.insert(name);f["name"]=name;f["uuid"]=QUuid::createUuid().toString(QUuid::WithoutBraces);
 return f;
}
