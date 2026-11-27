#include "../src/item-menu.hpp"
#include "../src/transition-policy.hpp"
#include "../src/projector-policy.hpp"
#include <QApplication>
#include <QScreen>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QApplication app(argc,argv);
 QMenu parent;int applied=-1;
 auto*m=choiceMenu(&parent,"Scale",scaleChoices(),int(OBS_SCALE_BICUBIC),[&](int v){applied=v;});
 CHECK(m->actions().size()==6);
 CHECK(m->actions()[3]->isChecked());
 CHECK(applied==-1);
 m->actions()[1]->trigger();CHECK(applied==OBS_SCALE_POINT);
 CHECK(m->actions()[1]->isChecked()&&!m->actions()[3]->isChecked());
 auto*b=choiceMenu(&parent,"Blend",blendChoices(),int(OBS_BLEND_NORMAL),[&](int v){applied=v;});
 CHECK(b->actions().size()==7);b->actions()[4]->trigger();CHECK(applied==OBS_BLEND_MULTIPLY);
 CHECK(blendMethodChoices().size()==2);
 auto t=resetTransform();CHECK(t.scale.x==1&&t.scale.y==1&&t.rot==0&&t.bounds_type==OBS_BOUNDS_NONE);
 CHECK(transitionDuration(0,300)==300);
 CHECK(transitionDuration(1,300)==50);
 CHECK(transitionDuration(30000,300)==20000);
 CHECK(chooseSceneOverride({"Fade",300},{"Cut",100}).name=="Fade");
 CHECK(chooseSceneOverride({},{"Cut",100}).name=="Cut");
 CHECK(chooseSceneOverride({},{}).name.isEmpty());
 const QRect desktop(1920,20,1920,1040);
 auto small=projectorWindowRect({1920,1080},desktop,ProjectorSize::Monitor);
 CHECK(small.size()==QSize(320,180)&&desktop.contains(small));
 auto portrait=projectorWindowRect({1080,1920},desktop,ProjectorSize::Monitor);
 CHECK(portrait.height()==240&&portrait.width()==135&&desktop.contains(portrait));
 auto large=projectorWindowRect({1920,1080},desktop,ProjectorSize::Review);
 CHECK(large.size()==QSize(960,540)&&desktop.contains(large));
 auto tiny=projectorWindowRect({1920,1080},QRect(-400,0,250,160),ProjectorSize::Review);
 CHECK(QRect(-400,0,250,160).contains(tiny)&&tiny.width()<=250&&tiny.height()<=160);
 CHECK(projectorWindowRect({},desktop,ProjectorSize::Monitor).size()==QSize(320,180));
 CHECK(projectorWindowRect({1,100000},desktop,ProjectorSize::Monitor).width()>0);
 CHECK(projectorWindowRect({1920,1080},{},ProjectorSize::Review).isEmpty());
 const auto local=projectorWindowRect({1920,1080},QGuiApplication::primaryScreen()->availableGeometry(),ProjectorSize::Monitor);
 QWidget restored(nullptr,Qt::Window);
 CHECK(restored.restoreGeometry(QByteArray::fromBase64(projectorGeometry(local))));
 CHECK(restored.size()==local.size());
 return 0;
}
