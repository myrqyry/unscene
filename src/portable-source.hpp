#pragma once
#include "portable-filter.hpp"
#include "item-copy.hpp"
#include "inspector.hpp"
#include <type_traits>

struct PortableSource {QJsonObject source;ItemCopy item;};
inline QByteArray encodePortableSource(const QJsonObject&source,const ItemCopy&i){
 auto &t=i.transform;auto &c=i.crop;
 auto vec=[](const vec2&v){return QJsonObject{{"x",v.x},{"y",v.y}};};
 QJsonObject transform{{"pos",vec(t.pos)},{"scale",vec(t.scale)},{"rot",t.rot},{"alignment",int(t.alignment)},{"bounds_type",int(t.bounds_type)},{"bounds",vec(t.bounds)},{"bounds_alignment",int(t.bounds_alignment)},{"crop_to_bounds",t.crop_to_bounds},{"left",c.left},{"top",c.top},{"right",c.right},{"bottom",c.bottom}};
 return QJsonDocument(QJsonObject{{"source",source},{"transform",transform},{"visible",i.visible},{"locked",i.locked},{"scale_filter",int(i.scale)},{"blending_method",int(i.blendMethod)},{"blending_mode",int(i.blend)}}).toJson(QJsonDocument::Indented);
}
inline std::optional<PortableSource> decodePortableSource(const QByteArray&bytes){
 if(bytes.isEmpty()||bytes.size()>PortableFilterMaxBytes)return {};
 auto doc=QJsonDocument::fromJson(bytes);if(!doc.isObject())return {};
 auto o=doc.object();if(!o["source"].isObject()||o.contains("sources")||o.contains("show_transition")||o.contains("hide_transition"))return {};
 auto s=o["source"].toObject();auto name=s["name"].toString(),id=s["id"].toString();
 if(name.trimmed().isEmpty()||name.size()>512||name.contains(QChar(0))||id.isEmpty()||id.size()>256||id.contains(QChar(0))||id=="scene"||id=="group"||!s["settings"].isObject())return {};
 if(s.contains("versioned_id")&&(!s["versioned_id"].isString()||s["versioned_id"].toString().contains(QChar(0))))return {};
 if(s.contains("filters")){
  if(!s["filters"].isArray())return {};
  auto f=s["filters"].toArray();if(!f.isEmpty()&&!decodePortableFilters(encodePortableFilters(f)))return {};
 }
 ItemCopy i;i.uid=s["uuid"].toString();
 TransformState t;
 if(o.contains("transform")){
  if(!o["transform"].isObject())return {};
  auto a=o["transform"].toObject();auto xy=[&](const char*k,const char*axis){return a.value(k).toObject().value(axis);};
  static_assert(std::is_same_v<decltype(xy("pos","x")),QJsonValue>, "Nested JSON lookup must return an owning value");
  QJsonArray fields{xy("pos","x"),xy("pos","y"),xy("scale","x"),xy("scale","y"),a["rot"],a["alignment"],a["bounds_type"],a["bounds_alignment"],xy("bounds","x"),xy("bounds","y"),(a.contains("crop_to_bounds")?a["crop_to_bounds"]:QJsonValue(false)),a["left"],a["top"],a["right"],a["bottom"]};
  auto decoded=TransformState::decode(QJsonDocument(fields).toJson(QJsonDocument::Compact));if(!decoded)return {};t=*decoded;
 }
 i.transform=t.info;i.crop=t.crop;
 for(auto key:{"visible","locked"})if(o.contains(key)&&!o[key].isBool())return {};
 i.visible=o["visible"].toBool(true);i.locked=o["locked"].toBool(false);
 auto integer=[&](const char*k,int max){auto v=o.contains(k)?o.value(k):QJsonValue(0);return v.isDouble()&&v.toDouble()==v.toInt(-1)&&v.toInt()>=0&&v.toInt()<=max;};
 if(!integer("scale_filter",OBS_SCALE_AREA)||!integer("blending_method",OBS_BLEND_METHOD_SRGB_OFF)||!integer("blending_mode",OBS_BLEND_DARKEN))return {};
 i.scale=static_cast<obs_scale_type>(o["scale_filter"].toInt());i.blendMethod=static_cast<obs_blending_method>(o["blending_method"].toInt());i.blend=static_cast<obs_blending_type>(o["blending_mode"].toInt());
 return PortableSource{s,i};
}
inline QJsonObject preparePortableSource(QJsonObject s,const QString&name){
 s["name"]=name;s["uuid"]=QUuid::createUuid().toString(QUuid::WithoutBraces);s.remove("hotkeys");
 QJsonArray filters;for(auto v:s["filters"].toArray()){auto f=v.toObject();f["uuid"]=QUuid::createUuid().toString(QUuid::WithoutBraces);f.remove("hotkeys");filters.append(f);}
 if(s.contains("filters"))s["filters"]=filters;
 return s;
}
