#include "../src/theme.hpp"
#include "../src/row.hpp"
#include <QApplication>
#include <QTest>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QApplication app(argc,argv);
 QPalette p; p.setColor(QPalette::WindowText,QColor("#ff33aa"));p.setColor(QPalette::Highlight,QColor("#224466"));p.setColor(QPalette::HighlightedText,QColor("#ffffff"));
 auto a=glyphPixmap("eye",p.color(QPalette::WindowText),16).toImage();
 auto b=glyphPixmap("eye-off",p.color(QPalette::WindowText),16).toImage();
 CHECK(a!=b);
 QImage empty(8,8,QImage::Format_RGBA8888);empty.fill(Qt::transparent);CHECK(!hasVisiblePixels(empty));empty.setPixelColor(3,2,Qt::white);CHECK(hasVisiblePixels(empty));
 auto c=glyphPixmap("eye",QColor("#33ff77"),16).toImage();CHECK(a!=c);
 CHECK(!glyphPixmap("lock",Qt::white,16).isNull());
 CHECK(!glyphPixmap("unlock",Qt::white,16).isNull());
 Row r("Long name which should elide at narrow sizes",true,false,0);
 r.setPalette(p);r.setSelected(true);r.setDisclosure(true,false);r.resize(180,32);
 r.addHover("Activate","arrow-right",[]{});
 r.show();app.processEvents();
 CHECK(r.name->text().contains(QChar(0x2026)));
 CHECK(r.name->toolTip()=="Long name which should elide at narrow sizes");
 CHECK(r.name->palette().color(QPalette::WindowText)==p.color(QPalette::HighlightedText));
 r.setSelected(false);CHECK(r.name->palette().color(QPalette::WindowText)==p.color(QPalette::WindowText));
 CHECK(r.hoverButtons.first()->accessibleName()=="Activate");
 return 0;
}
