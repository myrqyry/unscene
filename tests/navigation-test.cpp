#include "../src/navigation.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
struct Node{QString key,title;int depth;};
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);
 QVector<Node> nodes={{"a","Gameplay",0},{"a/1","Overlays",1},{"a/1/2","Webcam",2},{"a/3","Game",1},{"b","Chatting",0},{"b/4","Camera",1}};
 auto results=filterHierarchy(nodes,"  WEBcam ");
 CHECK(results.size()==3&&results[0].key=="a"&&results[1].key=="a/1"&&results[2].key=="a/1/2");
 results=filterHierarchy(nodes,"chat");CHECK(results.size()==2&&results[1].key=="b/4");
 CHECK(filterHierarchy(QVector<Node>{{"a","match",0},{"a/1","match",1},{"a/2","other",1}},"match").size()==3);
 CHECK(filterHierarchy(nodes,"missing").isEmpty());CHECK(filterHierarchy(nodes," ").size()==6);
 QTemporaryDir tmp;CHECK(tmp.isValid());
 auto path=tmp.filePath("navigation.ini");
 {NavigationStore store(path);CHECK(!store.opened("A").has_value());CHECK(store.saveOpened("A",{"a","a/1"}));CHECK(store.saveOpened("B",{}));CHECK(store.saveDensity(Density::Large,Density::Compact));}
 NavigationStore store(path);
 CHECK(store.opened("A").value()==QSet<QString>({"a","a/1"}));CHECK(store.opened("B").has_value()&&store.opened("B")->isEmpty());
 CHECK(store.sceneDensity()==Density::Large&&store.sourceDensity()==Density::Compact);
 CHECK(thumbnailSize(Density::Compact,true)==QSize(20,20));CHECK(thumbnailSize(Density::Large,true).width()>thumbnailSize(Density::Normal,true).width());
 CHECK(thumbnailSize(Density::Large,false).width()>thumbnailSize(Density::Normal,false).width());
 return 0;
}
