#include "../src/snapshot-policy.hpp"
#include "../src/navigation.hpp"
#include "../src/thumbnail-cache.hpp"
#include <QGuiApplication>
#include <QTemporaryDir>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QGuiApplication app(argc,argv);
 CHECK(!canCapture(SnapshotMode::CachedOnly,{}));
 CHECK(canCapture(SnapshotMode::IdleOnly,{}));
 for(int bit=0;bit<4;++bit){
  OutputState state{};switch(bit){case 0:state.streaming=true;break;case 1:state.recording=true;break;case 2:state.replay=true;break;case 3:state.virtualCamera=true;break;}
  CHECK(!canCapture(SnapshotMode::IdleOnly,state));
 }
 QTemporaryDir tmp;CHECK(tmp.isValid());auto path=tmp.filePath("settings.ini");
 {NavigationStore store(path);CHECK(store.snapshotMode()==SnapshotMode::CachedOnly);CHECK(store.saveSnapshotMode(SnapshotMode::IdleOnly));}
 {NavigationStore store(path);CHECK(store.snapshotMode()==SnapshotMode::IdleOnly);}
 ThumbnailCache cache(2);
 QPixmap art(2,2);art.fill(Qt::red);
 cache.insert("first",art);cache.insert("second",art);
 CHECK(!cache.lookup("first").isNull());
 cache.insert("third",art);
 CHECK(cache.contains("first")&&cache.contains("third")&&!cache.contains("second"));
 cache.remove("first");CHECK(!cache.contains("first"));
 cache.clear();CHECK(!cache.contains("third"));
 return 0;
}
