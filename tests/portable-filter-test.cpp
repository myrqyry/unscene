#include "../src/portable-filter.hpp"
#include <QCoreApplication>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);
 QJsonObject a{{"name","Crop"},{"id","crop_filter"},{"versioned_id","crop_filter"},{"uuid","old"},{"enabled",false},{"settings",QJsonObject{{"left",15}}}};
 QJsonObject b=a;b["name"]="Color";b["id"]="color_filter";b["versioned_id"]="color_filter";
 auto single=decodePortableFilters(encodePortableFilters({a}));CHECK(single&&*single==QJsonArray{a});
 auto stack=decodePortableFilters(encodePortableFilters({a,b}));CHECK(stack&&*stack==QJsonArray({a,b}));
 CHECK(!decodePortableFilters("{}"));CHECK(!decodePortableFilters("[1]"));
 CHECK(!decodePortableFilters(QByteArray(PortableFilterMaxBytes+1,'x')));
 CHECK(!decodePortableFilters(R"({"format":"unscene.filters","version":2,"filters":[]})"));
 QSet<QString> names{"Crop","Crop 2"};
 auto copy=preparePortableFilter(a,names);
 CHECK(copy["name"].toString()=="Crop 3"&&copy["uuid"]!=a["uuid"]);
 CHECK(copy["settings"]==a["settings"]&&copy["enabled"]==a["enabled"]);
 CHECK(names.contains("Crop 3"));
 auto malformed=a;malformed["settings"]=123;CHECK(!decodePortableFilters(encodePortableFilters({malformed})));
 malformed=a;malformed["filters"]=QJsonArray{b};CHECK(!decodePortableFilters(encodePortableFilters({malformed})));
 return 0;
}
