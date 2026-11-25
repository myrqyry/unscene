#include "../src/snapshot-policy.hpp"
#include "../src/navigation.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);
 CHECK(!canCapture(SnapshotMode::CachedOnly,{}));
 CHECK(canCapture(SnapshotMode::IdleOnly,{}));
 for(int bit=0;bit<4;++bit){
  OutputState state{};switch(bit){case 0:state.streaming=true;break;case 1:state.recording=true;break;case 2:state.replay=true;break;case 3:state.virtualCamera=true;break;}
  CHECK(!canCapture(SnapshotMode::IdleOnly,state));
 }
 QTemporaryDir tmp;CHECK(tmp.isValid());auto path=tmp.filePath("settings.ini");
 {NavigationStore store(path);CHECK(store.snapshotMode()==SnapshotMode::CachedOnly);CHECK(store.saveSnapshotMode(SnapshotMode::IdleOnly));}
 {NavigationStore store(path);CHECK(store.snapshotMode()==SnapshotMode::IdleOnly);}
 return 0;
}
