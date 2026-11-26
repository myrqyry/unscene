#include "../src/row.hpp"
#include <QApplication>
#include <QTest>
#include <cassert>
int main(int argc,char**argv) {
 QApplication app(argc,argv);
 int activated=0,expanded=0,properties=0;
 Row scene("Scene",true,false,0);
 scene.addHover("Activate","arrow-right",[&]{++activated;});
 scene.onBody=[&]{++expanded;};
 scene.show(); app.processEvents();
 QTest::mouseClick(scene.name,Qt::LeftButton);
 if(expanded!=1 || activated!=0) return 1;
 scene.hoverButtons[0]->click();
 if(activated!=1 || expanded!=1) return 2;
 Row item("Source",false,false,1);
 item.addSecondary("Properties","properties",[&]{++properties;});
 item.onBody=[&]{item.setExpanded(!item.expanded());};
 item.show(); app.processEvents();
 if(item.expanded()) return 3;
 QTest::mouseClick(item.name,Qt::LeftButton);
 if(!item.expanded() || item.secondary->isHidden()) return 4;
 item.secondaryButtons[0]->click();
 if(properties!=1 || !item.expanded()) return 5;
 QTest::mouseClick(item.name,Qt::LeftButton);
 if(item.expanded())return 6;
 // A cancelled press must not expand a row on its eventual release.
 QTest::mousePress(item.name,Qt::LeftButton);
 item.cancelGesture();
 QTest::mouseRelease(item.name,Qt::LeftButton);
 if(item.expanded())return 7;
 Row live("Program",true,false,0);
 live.addSceneActivation(true,false,false,[&]{++activated;});
 live.show();app.processEvents();
 QEvent leave(QEvent::Leave);QApplication::sendEvent(&live,&leave);
 auto*indicator=live.hoverButtons.first();
 if(indicator->isHidden()||indicator->accessibleName()!="Live scene")return 8;
 indicator->click();
 if(activated!=1)return 9;
 Row studio("Program",true,false,0);
 studio.addSceneActivation(true,true,false,[&]{++activated;});
 studio.show();app.processEvents();QApplication::sendEvent(&studio,&leave);
 if(studio.hoverButtons.first()->isHidden())return 10;
 studio.hoverButtons.first()->click();if(activated!=2)return 11;
 Row inactive("Other",true,false,0);
 inactive.addSceneActivation(false,false,false,[&]{++activated;});
 inactive.show();app.processEvents();QApplication::sendEvent(&inactive,&leave);
 if(!inactive.hoverButtons.first()->isHidden())return 12;
 return 0;
}
