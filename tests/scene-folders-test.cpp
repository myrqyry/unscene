#include "../src/scene-folders.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);
 SceneFolders f;
 auto a=f.add("Streaming",{});auto b=f.add("Games",a);
 CHECK(!a.isEmpty()&&!b.isEmpty()&&a!=b);
 CHECK(f.moveScene("scene-1",b));CHECK(f.parentOfScene("scene-1")==b);
 CHECK(!f.moveFolder(a,b));CHECK(!f.moveFolder(a,a));
 CHECK(!f.moveScene("scene-2","missing"));
 CHECK(f.rename(b,"Gameplay"));CHECK(f.path(b)=="Streaming / Gameplay");
 auto decoded=SceneFolders::decode(f.encode());CHECK(decoded.encode()==f.encode());
 CHECK(decoded.remove(a));CHECK(decoded.parentOfFolder(b).isEmpty());
 CHECK(decoded.parentOfScene("scene-1")==b);
 CHECK(decoded.remove(b));CHECK(decoded.parentOfScene("scene-1").isEmpty());
 QTemporaryDir tmp;CHECK(tmp.isValid());
 {SceneFolderStore store(tmp.filePath("folders.ini"));CHECK(store.save("A",f));}
 SceneFolderStore store(tmp.filePath("folders.ini"));CHECK(store.load("A").encode()==f.encode());
 CHECK(store.load("B").folders().isEmpty());CHECK(!store.save("",f));
 CHECK(SceneFolders::decode(R"({"folders":[{"id":"folder:a","name":"a","parent":"folder:b"},{"id":"folder:b","name":"b","parent":"folder:a"}],"scenes":{"s":"folder:a"}})").folders().isEmpty());
 return 0;
}
