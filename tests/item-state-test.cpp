#include "../src/item-state.hpp"
#include <QCoreApplication>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);
 ItemStateUndo u{"Collection","parent","source","visibility",42,1,0};
 auto v=ItemStateUndo::decode(u.encode());CHECK(v&&v->encode()==u.encode());
 CHECK(!ItemStateUndo::decode("{}"));
 u.next=2;CHECK(!ItemStateUndo::decode(u.encode()));
 u.field="scale";u.next=OBS_SCALE_AREA;CHECK(ItemStateUndo::decode(u.encode()));
 u.next=OBS_SCALE_AREA+1;CHECK(!ItemStateUndo::decode(u.encode()));
 u.field="blend";u.next=OBS_BLEND_DARKEN;CHECK(ItemStateUndo::decode(u.encode()));
 u.field="bogus";CHECK(!ItemStateUndo::decode(u.encode()));
 u.field="locked";u.expected=0;u.next=1;u.id=0;CHECK(!ItemStateUndo::decode(u.encode()));
 return 0;
}
