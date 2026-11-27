#include "../src/row.hpp"
#include <QApplication>
#include "widget-events.hpp"
#include <cassert>
int main(int argc,char**argv) {
 QApplication app(argc,argv);
 int activated=0,expanded=0,properties=0;
 Row scene("Scene",true,false,0);
 scene.addHover("Activate","arrow-right",[&]{++activated;});
 scene.onBody=[&]{++expanded;};
 scene.show(); app.processEvents();
 TestEvents::mouseClick(scene.name,Qt::LeftButton);
 if(expanded!=1 || activated!=0) return 1;
 scene.hoverButtons[0]->click();
 if(activated!=1 || expanded!=1) return 2;
 Row item("Source",false,false,1);
 item.addSecondary("Properties","properties",[&]{++properties;});
 item.onBody=[&]{item.setExpanded(!item.expanded());};
 item.show(); app.processEvents();
 if(item.expanded()) return 3;
 TestEvents::mouseClick(item.name,Qt::LeftButton);
 if(!item.expanded() || item.secondary->isHidden()) return 4;
 item.secondaryButtons[0]->click();
 if(properties!=1 || !item.expanded()) return 5;
 TestEvents::mouseClick(item.name,Qt::LeftButton);
 if(item.expanded())return 6;
 // A cancelled press must not expand a row on its eventual release.
 TestEvents::mousePress(item.name,Qt::LeftButton);
 item.cancelGesture();
 TestEvents::mouseRelease(item.name,Qt::LeftButton);
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
 QPalette pal;pal.setColor(QPalette::Base,QColor("#242424"));pal.setColor(QPalette::WindowText,QColor("#eeeeee"));
 pal.setColor(QPalette::Highlight,QColor("#774466"));pal.setColor(QPalette::HighlightedText,Qt::white);
 Row hidden("Hidden source",false,false,1);hidden.setPalette(pal);
 hidden.setBranchColor(QColor("#e05090"),true,true);
 hidden.addHover("Show source","eye-off",[]{},true);
 QPixmap art(20,20);art.fill(Qt::white);hidden.thumb->setPixmap(art);
 hidden.resize(320,44);hidden.show();app.processEvents();
 QApplication::sendEvent(&hidden,&leave);
 const auto full=hidden.inkColor();const auto visibleArt=hidden.thumb->grab().toImage().pixelColor(hidden.thumb->width()/2,hidden.thumb->height()/2);
 hidden.setSourceVisible(false);app.processEvents();
 if(hidden.inkColor()==full||surfaceContrast(hidden.inkColor(),sceneSurfaceRoles(pal,QColor("#e05090")).container)<4.5)return 13;
 if(hidden.thumb->grab().toImage().pixelColor(hidden.thumb->width()/2,hidden.thumb->height()/2)==visibleArt)return 14;
 if(hidden.hoverButtons.first()->isHidden()||!hidden.hoverButtons.first()->isEnabled())return 15;
 hidden.setSelected(true);if(surfaceContrast(hidden.inkColor(),pal.color(QPalette::Highlight))<4.5)return 16;
 hidden.setSelected(false);hidden.setSourceVisible(true);app.processEvents();
 if(hidden.inkColor()!=full||hidden.thumb->grab().toImage().pixelColor(hidden.thumb->width()/2,hidden.thumb->height()/2)!=visibleArt)return 17;
 QFont base=app.font();base.setPointSizeF(12);
 auto sceneFont=rowTypography(base,TypographyRole::Scene);
 auto sourceFont=rowTypography(base,TypographyRole::Source);
 auto metaFont=rowTypography(base,TypographyRole::Status);
 auto tabFont=rowTypography(base,TypographyRole::Tab);
 if(sceneFont.weight()<=sourceFont.weight()||sceneFont.pointSizeF()<sourceFont.pointSizeF()||metaFont.pointSizeF()<10||tabFont.weight()<=sourceFont.weight())return 18;
 base.setPixelSize(16);
 if(rowTypography(base,TypographyRole::Status).pixelSize()<13)return 19;
 return 0;
}
