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
 return item.expanded()?6:0;
}
