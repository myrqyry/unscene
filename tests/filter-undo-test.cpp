#include "../src/filter-undo.hpp"
#include <QCoreApplication>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);
 FilterUndo a{"Collection","parent","filter","enabled",true};
 auto b=FilterUndo::decode(a.encode());CHECK(b&&b->encode()==a.encode());
 CHECK(!FilterUndo::decode("{}"));
 CHECK(!FilterUndo::decode(FilterUndo{"Collection","parent","filter","enabled",123}.encode()));
 CHECK(!FilterUndo::decode(FilterUndo{"Collection","parent","filter","index",-1}.encode()));
 CHECK(!FilterUndo::decode(FilterUndo{"Collection","parent","filter","name",""}.encode()));
 CHECK(FilterUndo::decode(FilterUndo{"Collection","parent","filter","name","New name"}.encode()));
 CHECK(FilterUndo::decode(FilterUndo{"Collection","parent","filter","index",2}.encode()));
 CHECK(FilterUndo::decode(FilterUndo{"Collection","parent","filter","absent",QJsonValue::Null}.encode()));
 QJsonObject saved{{"uuid","filter"},{"name","Crop"},{"id","crop_filter"},{"settings",QJsonObject{{"left",13}}}};
 auto restore=FilterUndo{"Collection","parent","filter","present",QJsonObject{{"index",0},{"source",saved}}};
 CHECK(FilterUndo::decode(restore.encode()));
 saved["uuid"]="wrong";restore.value=QJsonObject{{"index",0},{"source",saved}};CHECK(!FilterUndo::decode(restore.encode()));
 return 0;
}
