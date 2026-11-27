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
 CHECK(f.rootForScene("scene-1")==a);
 CHECK(f.rootForScene("unfiled").isEmpty());
 CHECK(f.validTab(a)&&f.validTab({})&&!f.validTab(b));
 CHECK(f.rename(b,"Gameplay"));CHECK(f.path(b)=="Streaming / Gameplay");
 auto decoded=SceneFolders::decode(f.encode());CHECK(decoded.encode()==f.encode());
 CHECK(decoded.remove(a));CHECK(decoded.parentOfFolder(b).isEmpty());
 CHECK(decoded.parentOfScene("scene-1")==b);
 CHECK(decoded.remove(b));CHECK(decoded.parentOfScene("scene-1").isEmpty());
 QTemporaryDir tmp;CHECK(tmp.isValid());
 {SceneFolderStore store(tmp.filePath("folders.ini"));CHECK(store.save("A",f));}
 SceneFolderStore store(tmp.filePath("folders.ini"));CHECK(store.load("A").encode()==f.encode());
 CHECK(store.load("B").folders().isEmpty());
 CHECK(store.saveTab("A",a));CHECK(store.tab("A")==a);CHECK(store.tab("B").isEmpty());
 CHECK(store.saveTab("A",{}));CHECK(store.tab("A").isEmpty());
 auto duplicate=f.duplicateStructure(a,"Recording");
 CHECK(!duplicate.isEmpty()&&f.validTab(duplicate));
 CHECK(f.rootForScene("scene-1")==a);
 CHECK(f.folders().size()==4);
 QString duplicateChild;
 for(const auto&folder:f.folders())if(folder.parent==duplicate)duplicateChild=folder.id;
 CHECK(!duplicateChild.isEmpty());
 CHECK(f.remove(duplicate));CHECK(f.validTab(duplicateChild));
 CHECK(f.remove(duplicateChild));CHECK(!store.save("",f));
 auto c=f.add("Recording",{});
 CHECK(f.rootTabs()==QStringList({a,c}));
 auto before=f.encode();
 CHECK(!f.reorderTabs({c,c}));
 CHECK(!f.reorderTabs({c,b}));
 CHECK(f.encode()==before);
 CHECK(f.reorderTabs({c,a}));
 CHECK(SceneFolders::decode(f.encode()).rootTabs()==QStringList({c,a}));
 CHECK(f.rootForScene("scene-1")==a);
 FolderHistory history;
 auto after=f.encode();history.record(before,after);
 CHECK(history.canUndo()&&!history.canRedo());
 CHECK(history.undoTarget(after)==before);
 CHECK(history.undoTarget("conflicting").isEmpty());
 history.didUndo();
 CHECK(history.redoTarget(before)==after);
 history.didRedo();
 CHECK(history.undoTarget(after)==before);
 history.didUndo();
 history.record(before,QByteArray("new"));
 CHECK(!history.canRedo());
 history.clear();CHECK(!history.canUndo()&&!history.canRedo());
 CHECK(SceneFolders::decode(R"({"folders":[{"id":"folder:a","name":"a","parent":"folder:b"},{"id":"folder:b","name":"b","parent":"folder:a"}],"scenes":{"s":"folder:a"}})").folders().isEmpty());
 return 0;
}
