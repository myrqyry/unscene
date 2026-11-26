#include "../src/inspector.hpp"
#include <QApplication>
#include <QTemporaryDir>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QApplication app(argc,argv);
 TransformState t; t.info.pos.x=123.5;t.info.scale.x=-1;t.info.rot=15;t.info.bounds_type=OBS_BOUNDS_SCALE_INNER;t.info.bounds.x=640;t.crop.left=12;
 auto round=TransformState::decode(t.encode());CHECK(round&&round->encode()==t.encode());
 CHECK(!TransformState::decode(QByteArray("{}")));
 QTemporaryDir tmp;CHECK(tmp.isValid());PresetStore store(tmp.filePath("presets.ini"));
 CHECK(store.save("Wide / test",t));CHECK(store.names()==QStringList{"Wide / test"});
 {PresetStore reloaded(tmp.filePath("presets.ini"));CHECK(reloaded.load("Wide / test")->encode()==t.encode());}
 Inspector panel(tmp.filePath("presets.ini"));panel.show();panel.load("Image","Scene",t,false);
 auto*pos=panel.findChild<QDoubleSpinBox*>("positionX");CHECK(pos&&pos->value()==123.5);
 CHECK(!panel.dirty());pos->setValue(321);CHECK(panel.dirty());CHECK(panel.draft().info.pos.x==321);
 CHECK(panel.draft().info.scale.x==-1&&panel.draft().crop.left==12);
 panel.load("Image","Scene",t,true);CHECK(!panel.dirty()&&!pos->isEnabled());
 t.info.scale.y=0.000000123456f;panel.load("Image","Scene",t,false);CHECK(!panel.dirty());pos->setValue(456);CHECK(panel.draft().info.scale.y==t.info.scale.y);
 auto*properties=panel.findChild<QToolButton*>("inspectorProperties");auto*filters=panel.findChild<QToolButton*>("inspectorFilters");
 CHECK(properties&&filters&&properties->isVisible()&&filters->isVisible());
 int calls=0;panel.onProperties=[&]{++calls;};panel.onFilters=[&]{++calls;};properties->click();filters->click();CHECK(calls==2);
 auto*tabs=panel.findChild<QTabWidget*>();CHECK(tabs);panel.setMotionDuration(150);
 tabs->setCurrentIndex(1);CHECK(panel.motionActive()&&tabs->currentWidget()->graphicsEffect());
 tabs->setCurrentIndex(2);CHECK(!tabs->widget(1)->graphicsEffect()&&panel.motionActive());
 panel.setMotionDuration(0);CHECK(!panel.motionActive()&&!tabs->currentWidget()->graphicsEffect());
 CHECK(panel.dirty()&&panel.draft().info.pos.x==456);
 CHECK(store.remove("Wide / test"));CHECK(store.names().isEmpty());
 return 0;
}
