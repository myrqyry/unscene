#include "../src/context-appearance.hpp"
#include "../src/row.hpp"
#include <QApplication>
#include <QTemporaryDir>
#include <QFile>
#include <iostream>
#define CHECK(x) do{if(!(x)){std::cerr<<"line "<<__LINE__<<": "<<#x<<"\n";return 1;}}while(0)
int main(int argc,char**argv){
 QApplication app(argc,argv);QTemporaryDir temp;
 using namespace unscene;
 const auto path=temp.filePath("context-notes.json");
 ContextNotes a(path),b(path);
 const auto key=noteKey("source","abc"),item=noteKey("item","abc","owner",42);
 CHECK(key==QStringLiteral("[\"source\",\"abc\",\"\",\"-1\"]"));
 CHECK(a.put(key,{},"literal <b> text")&&b.reload()&&b.get(key)=="literal <b> text");
 CHECK(b.put(item,{},"placement")&&!a.put(item,{},"stale"));
 CHECK(a.reload()&&a.get(item)=="placement");
 CHECK(a.put(key,a.get(key),{})&&b.reload()&&b.get(key).isEmpty()&&b.get(item)=="placement");
 QFile bad(path);CHECK(bad.open(QIODevice::WriteOnly|QIODevice::Truncate));bad.write("broken");bad.close();
 CHECK(!a.reload()&&!a.put(key,{},"do not overwrite"));
 CHECK(noteTooltip("<b>\n")=="<span>&lt;b&gt;<br></span>");
 QPalette pal;pal.setColor(QPalette::Base,QColor("#222222"));pal.setColor(QPalette::AlternateBase,QColor("#444444"));pal.setColor(QPalette::WindowText,Qt::white);
 auto prefs=RowStripes::parse(QJsonObject{{"scenes",false},{"sources",true},{"strength",50}});
 CHECK(prefs&&prefs->sources&&!prefs->scenes);
 CHECK(!RowStripes::parse(QJsonObject{{"strength",201}}));
 CHECK(prefs->color(pal,QColor("#333333"),false)==QColor("#333333"));
 CHECK(prefs->color(pal,QColor("#333333"),true)==QColor("#444444"));
 Row r("Test",false,false,0);r.setPalette(pal);r.resize(300,36);r.show();app.processEvents();
 int edits=0;r.setNote("literal <b> text",[&]{++edits;});app.processEvents();
 auto*badge=r.findChild<QToolButton*>("unscene-note");CHECK(badge&&badge->isVisible()&&badge->toolTip().contains("&lt;b&gt;"));
 badge->click();CHECK(edits==1&&!r.expanded());
 r.setNote({},{});CHECK(!badge->isVisible());
 r.setBranchColor({},true,true);r.setStripe(*prefs,true,true);app.processEvents();
 auto bg=r.idleSurface();CHECK(surfaceContrast(bg,r.inkColor())>=4.5);
 auto striped=r.grab().toImage();r.setStripe(*prefs,true,false);app.processEvents();
 CHECK(striped!=r.grab().toImage());
 r.setSelected(true);CHECK(surfaceContrast(r.inkColor(),pal.color(QPalette::Highlight))>=4.5);
 return 0;
}
