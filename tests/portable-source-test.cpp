#include "../src/portable-source.hpp"
#include <QCoreApplication>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);
 QJsonObject filter{{"id","crop_filter"},{"name","Crop"},{"uuid","old-filter"},{"settings",QJsonObject{{"left",12}}}};
 QJsonObject source{{"id","color_source"},{"versioned_id","color_source_v3"},{"name","Color"},{"uuid","old"},{"settings",QJsonObject{{"width",320}}},{"filters",QJsonArray{filter}},{"hotkeys",QJsonObject{{"test",QJsonArray{1}}}}};
 ItemCopy item;item.uid="old";item.transform.scale.x=-1;item.transform.scale.y=1;item.transform.pos.x=42;item.crop.left=12;item.visible=false;item.locked=true;item.scale=OBS_SCALE_POINT;
 auto payload=encodePortableSource(source,item);
 auto p=decodePortableSource(payload);CHECK(p&&p->source==source&&p->item.transform.pos.x==42&&p->item.crop.left==12&&!p->item.visible&&p->item.locked&&p->item.scale==OBS_SCALE_POINT);
 auto fresh=preparePortableSource(source,"Color 2");CHECK(fresh["uuid"]!=source["uuid"]&&!fresh.contains("hotkeys"));
 CHECK(fresh["filters"].toArray()[0].toObject()["uuid"]!=filter["uuid"]);CHECK(fresh["settings"]==source["settings"]);
 auto o=QJsonDocument::fromJson(payload).object();o["scale_filter"]=99;CHECK(!decodePortableSource(QJsonDocument(o).toJson()));
 o=QJsonDocument::fromJson(payload).object();o["show_transition"]=QJsonObject{{"id","fade_transition"}};CHECK(!decodePortableSource(QJsonDocument(o).toJson()));
 source["id"]="scene";CHECK(!decodePortableSource(encodePortableSource(source,item)));
 return 0;
}
