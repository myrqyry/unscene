#pragma once
#include <obs.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <optional>

inline int itemStateMax(const QString&field){
 if(field=="visibility"||field=="locked")return 1;
 if(field=="scale")return OBS_SCALE_AREA;
 if(field=="blend")return OBS_BLEND_DARKEN;
 if(field=="blend-method")return OBS_BLEND_METHOD_SRGB_OFF;
 return -1;
}
inline int readItemState(obs_sceneitem_t*i,const QString&field){
 if(field=="visibility")return obs_sceneitem_visible(i);
 if(field=="locked")return obs_sceneitem_locked(i);
 if(field=="scale")return obs_sceneitem_get_scale_filter(i);
 if(field=="blend")return obs_sceneitem_get_blending_mode(i);
 if(field=="blend-method")return obs_sceneitem_get_blending_method(i);
 return -1;
}
inline bool writeItemState(obs_sceneitem_t*i,const QString&field,int value){
 if(!i||value<0||value>itemStateMax(field))return false;
 if(field=="visibility")obs_sceneitem_set_visible(i,value!=0);
 else if(field=="locked")obs_sceneitem_set_locked(i,value!=0);
 else if(field=="scale")obs_sceneitem_set_scale_filter(i,static_cast<obs_scale_type>(value));
 else if(field=="blend")obs_sceneitem_set_blending_mode(i,static_cast<obs_blending_type>(value));
 else if(field=="blend-method")obs_sceneitem_set_blending_method(i,static_cast<obs_blending_method>(value));
 else return false;
 return readItemState(i,field)==value;
}
struct ItemStateUndo {
 QString collection,parent,source,field;
 int64_t id=0;
 int expected=0,next=0;
 QByteArray encode()const{
  return QJsonDocument(QJsonObject{{"collection",collection},{"parent",parent},{"source",source},{"field",field},{"id",QString::number(id)},{"expected",expected},{"next",next}}).toJson(QJsonDocument::Compact);
 }
 static std::optional<ItemStateUndo> decode(const QByteArray&bytes){
  auto o=QJsonDocument::fromJson(bytes).object();bool ok=false;
  ItemStateUndo u{o["collection"].toString(),o["parent"].toString(),o["source"].toString(),o["field"].toString(),o["id"].toString().toLongLong(&ok),o["expected"].toInt(-1),o["next"].toInt(-1)};
  int max=itemStateMax(u.field);
  if(!ok||u.id<=0||u.collection.isEmpty()||u.parent.isEmpty()||u.source.isEmpty()||max<0)return {};
  if(u.expected<0||u.expected>max||u.next<0||u.next>max||u.expected==u.next)return {};
  return u;
 }
};
