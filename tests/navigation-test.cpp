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
 CHECK(store.motionDuration()==150);CHECK(store.saveMotion(0));{NavigationStore reloaded(path);CHECK(reloaded.motionDuration()==0);CHECK(reloaded.saveMotion(90));} CHECK(store.motionDuration()==90);
 CHECK(!store.searchAllTabs());
 CHECK(store.hideSingleTab());
 CHECK(!showSceneTabStrip(1,store.hideSingleTab()));
 CHECK(!showSceneTabStrip(0,store.hideSingleTab()));
 CHECK(showSceneTabStrip(2,store.hideSingleTab()));
 CHECK(store.saveHideSingleTab(false));
 {NavigationStore again(path);CHECK(!again.hideSingleTab());CHECK(showSceneTabStrip(1,again.hideSingleTab()));CHECK(again.saveHideSingleTab(true));}
 CHECK(store.hideSingleTab());
 CHECK(store.saveSearchAllTabs(true));
 CHECK(store.saveFavorites("A",{"a","b"}));
 CHECK(store.saveFavorites("B",{"c"}));
 {NavigationStore again(path);CHECK(again.searchAllTabs());CHECK(again.favorites("A")==QSet<QString>({"a","b"}));CHECK(again.favorites("B")==QSet<QString>({"c"}));}
 CHECK(favoriteFirst(QStringList({"c","b","a","d"}),QSet<QString>({"a","c"}))==QStringList({"c","a","b","d"}));
 return 0;
}
