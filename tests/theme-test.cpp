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
 r.setSelected(false);CHECK(r.name->palette().color(QPalette::WindowText)==sceneSurfaceRoles(p,{}).onHeader);
 CHECK(r.hoverButtons.first()->accessibleName()=="Activate");
 // A stylesheet geometry change must affect real widget dimensions and paint.
 QWidget host;auto*layout=new QVBoxLayout(&host);auto*button=new GlyphButton("eye",&host);
 layout->addWidget(button);host.show();app.processEvents();
 host.setStyleSheet("QToolButton { icon-size: 12px; padding: 2px; border: 1px solid #2468ac; border-radius: 0px; background: #123456; }");
 app.processEvents();const auto small=button->sizeHint();
 CHECK(button->themedIconSize()==QSize(12,12));
 CHECK(button->grab().toImage().pixelColor(3,3)==QColor("#123456"));
 host.setStyleSheet("QToolButton { icon-size: 24px; padding: 9px; border: 3px solid #2468ac; border-radius: 0px; background: #123456; }");
 app.processEvents();CHECK(button->themedIconSize()==QSize(24,24));
 CHECK(button->sizeHint().width()>small.width()+10);
 CHECK(button->sizeHint().height()>small.height()+10);
 CHECK(button->width()>=button->sizeHint().width());
 const auto square=button->grab().toImage();
 host.setStyleSheet("QToolButton { icon-size: 24px; padding: 9px; border: 3px solid #2468ac; border-radius: 18px; background: #123456; }");
 app.processEvents();
 CHECK(button->grab().toImage().pixelColor(1,1)!=square.pixelColor(1,1));
 QListView native;native.setStyleSheet("QListView::item { padding: 9px; min-height: 55px; border: 2px solid transparent; }");
 Row styled("Source",false,false,1);styled.setNativeList(&native);styled.show();app.processEvents();
 CHECK(styled.sizeHint().height()>=55);
 // Seed-derived tonal roles must adapt to both theme modes.
 for(auto base:{QColor("#20242a"),QColor("#f3ebe2")}){
  QPalette palette;palette.setColor(QPalette::Base,base);palette.setColor(QPalette::WindowText,base.lightness()<128?QColor("#e1dce8"):QColor("#57506e"));
  for(auto seed:{QColor("#ff0044"),QColor("#00ff44"),QColor("#0055ff"),QColor("#ffffff"),QColor("#000000")}){
   auto roles=sceneSurfaceRoles(palette,seed);
   CHECK(roles.container!=roles.header);
   CHECK(surfaceContrast(roles.container,roles.onContainer)>=4.5);
   CHECK(surfaceContrast(roles.header,roles.onHeader)>=4.5);
   CHECK(surfaceContrast(roles.container,roles.accent)>=3.0);
   CHECK(base.lightness()<128?roles.container.lightness()>base.lightness():roles.container.lightness()<base.lightness());
  }
 }
 // Theme-default scenes must use the native accent and track theme changes.
 QPalette defaultTheme;
 defaultTheme.setColor(QPalette::Base,QColor("#242424"));
 defaultTheme.setColor(QPalette::WindowText,QColor("#e5e5e5"));
 defaultTheme.setColor(QPalette::Highlight,QColor("#da5291"));
 auto pink=sceneSurfaceRoles(defaultTheme,{});
 CHECK(pink.container==sceneSurfaceRoles(defaultTheme,defaultTheme.color(QPalette::Highlight)).container);
 Row defaultHeader("Theme scene",true,false,0);
 defaultHeader.setPalette(defaultTheme);defaultHeader.resize(260,40);defaultHeader.show();app.processEvents();
 QEvent leave(QEvent::Leave);QApplication::sendEvent(&defaultHeader,&leave); CHECK(defaultHeader.grab().toImage().pixelColor(12,20).rgb()==pink.header.rgb());
 defaultTheme.setColor(QPalette::Highlight,QColor("#43bda9"));
 auto teal=sceneSurfaceRoles(defaultTheme,{});
 CHECK(teal.container!=pink.container&&teal.header!=pink.header);
 defaultHeader.setPalette(defaultTheme);app.processEvents();
 CHECK(defaultHeader.grab().toImage().pixelColor(12,20).rgb()==teal.header.rgb());
 CHECK(!defaultHeader.accent().isValid());
 Row defaultChild("Child",false,false,1);defaultChild.setBranchColor({},true,true);
 defaultChild.setPalette(defaultTheme);defaultChild.resize(260,40);defaultChild.show();app.processEvents();
 QApplication::sendEvent(&defaultChild,&leave);
 CHECK(defaultChild.grab().toImage().pixelColor(12,20).rgb()==teal.container.rgb());
 defaultChild.setPalette(p);app.processEvents();
 CHECK(defaultChild.grab().toImage().pixelColor(12,20).rgb()==sceneSurfaceRoles(p,{}).container.rgb());
 const QColor custom("#cc9955");
 defaultHeader.setAccent(custom);app.processEvents();
 auto before=sceneSurfaceRoles(defaultTheme,custom).header;
 defaultTheme.setColor(QPalette::Highlight,QColor("#5599ff"));defaultHeader.setPalette(defaultTheme);app.processEvents();
 CHECK(defaultHeader.grab().toImage().pixelColor(12,20).rgb()==before.rgb());
 // Branch surfaces inherit their owning scene, not the source UUID.
 QWidget canvas;canvas.setStyleSheet("QWidget#branchFixture {background:#20242a;}");canvas.setObjectName("branchFixture");
 Row branch("Child",false,false,1,&canvas);branch.setPalette(p);branch.setBranchColor(QColor("#e05050"),true,true);
 branch.resize(260,44);canvas.resize(260,44);canvas.show();app.processEvents();
 auto red=canvas.grab().toImage();
 CHECK(red.pixelColor(12,22).red()>red.pixelColor(12,22).blue());
 CHECK(red.pixelColor(0,0)!=red.pixelColor(12,22));
 branch.setBranchColor(QColor("#5080e0"),true,true);app.processEvents();
 auto blue=canvas.grab().toImage();CHECK(blue.pixelColor(12,22).blue()>blue.pixelColor(12,22).red());
 branch.setBranchColor({},true,true);app.processEvents();CHECK(canvas.grab().toImage().pixelColor(12,22)!=blue.pixelColor(12,22));
 host.setStyleSheet("QToolButton { icon-size: 24px; padding: 9px; border: 3px solid #2468ac; border-radius: 0px; background: #123456; }");
 app.processEvents();auto squareBefore=button->grab().toImage();
 button->setCircular(true);app.processEvents();CHECK(button->sizeHint().width()==button->sizeHint().height());
 auto circular=button->grab().toImage();
 CHECK(circular.pixelColor(1,1)!=squareBefore.pixelColor(1,1));
 // Row controls use the owning surface, not the global gray button fill.
 QWidget tonalHost;tonalHost.setStyleSheet("QWidget#tonalHost {background:#512d45;} QToolButton {background:#777777; padding:5px;}");
 tonalHost.setObjectName("tonalHost");auto*tonalLayout=new QHBoxLayout(&tonalHost);
 auto*chevron=new GlyphButton("right",&tonalHost);chevron->setCircular(true);tonalLayout->addWidget(chevron);
 chevron->setTonalColors(QColor("#512d45"),QColor("#f9d9ed"));
 tonalHost.show();app.processEvents();QApplication::sendEvent(chevron,&leave);
 const auto rest=tonalHost.grab().toImage();const QPoint sample=chevron->pos()+QPoint(chevron->width()/2,3);
 CHECK(rest.pixelColor(sample)==QColor("#512d45"));
 QEnterEvent enter(QPointF(10,10),QPointF(10,10),QPointF(10,10));QApplication::sendEvent(chevron,&enter);
 CHECK(tonalHost.grab().toImage().pixelColor(sample)!=rest.pixelColor(sample));
 QApplication::sendEvent(chevron,&leave);
 auto*live=new GlyphButton("live",&tonalHost);live->setCircular(true);live->setTonalColors(QColor("#512d45"),QColor("#f9d9ed"));tonalLayout->addWidget(live);
 app.processEvents();QApplication::sendEvent(live,&leave);
 CHECK(tonalHost.grab().toImage().pixelColor(live->pos()+QPoint(live->width()/2,3))!=QColor("#512d45"));
 return 0;
}
