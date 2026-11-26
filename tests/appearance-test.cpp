#include "../src/appearance.hpp"
#include "../src/row.hpp"
#include <QApplication>
#include <QTemporaryDir>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QApplication app(argc,argv);
 QTemporaryDir tmp; CHECK(tmp.isValid());
 const auto path=tmp.filePath("appearance.ini");
 const QString scene="scene-uuid",source="source-uuid";
 QImage original(220,110,QImage::Format_ARGB32);original.fill(QColor("#e14090"));
 CHECK(original.save(tmp.filePath("art.png")));
 Appearance a; a.color=QColor("#32a891");a.mode="custom";a.image=importArtwork(tmp.filePath("art.png"));
 CHECK(!a.image.isEmpty());
 {AppearanceStore store(path);CHECK(store.save(scene,a));Appearance b;b.mode="star";CHECK(store.save(source,b));}
 QFile::remove(tmp.filePath("art.png"));
 AppearanceStore store(path);auto loaded=store.load(scene);
 CHECK(loaded.color==a.color && loaded.mode=="custom" && loaded.image==a.image);
 CHECK(store.load(source).mode=="star");
 CHECK(store.load("missing").mode=="auto");
 Row row("Scene",true,false,0);
 auto p=appearancePixmap(loaded,row.palette(),row.thumb->size());
 CHECK(!p.isNull() && p.width()==48 && p.height()==24);
 CHECK(p.toImage().pixelColor(20,10)==QColor("#e14090"));
 CHECK(!appearancePixmap(store.load(source),row.palette(),QSize(36,22)).isNull());
 CHECK(appearancePixmap(Appearance{},row.palette(),QSize(36,22)).isNull());
 row.setAccent(loaded.color);CHECK(row.accent()==loaded.color);
 bool edited=false;row.onAppearance=[&]{edited=true;};
 row.show();app.processEvents();
 QContextMenuEvent event(QContextMenuEvent::Mouse,QPoint(1,1),QPoint(1,1));
 QApplication::sendEvent(row.name,&event);CHECK(edited);
 CHECK(store.save(scene,Appearance{}));CHECK(store.load(scene).mode=="auto"&&!store.load(scene).color.isValid());
 CHECK(store.load(source).mode=="star");
 CHECK(importArtwork(tmp.filePath("art.png")).isEmpty());
 return 0;
}
