#pragma once
#include <obs.h>
#include <QMenu>
#include <QActionGroup>
#include <QVector>
#include <functional>
struct MenuChoice{const char*label;int value;};
inline QVector<MenuChoice> scaleChoices(){return {{"Disable",OBS_SCALE_DISABLE},{"Point",OBS_SCALE_POINT},{"Bilinear",OBS_SCALE_BILINEAR},{"Bicubic",OBS_SCALE_BICUBIC},{"Lanczos",OBS_SCALE_LANCZOS},{"Area",OBS_SCALE_AREA}};}
inline QVector<MenuChoice> blendChoices(){return {{"Normal",OBS_BLEND_NORMAL},{"Additive",OBS_BLEND_ADDITIVE},{"Subtract",OBS_BLEND_SUBTRACT},{"Screen",OBS_BLEND_SCREEN},{"Multiply",OBS_BLEND_MULTIPLY},{"Lighten",OBS_BLEND_LIGHTEN},{"Darken",OBS_BLEND_DARKEN}};}
inline QVector<MenuChoice> blendMethodChoices(){return {{"Default",OBS_BLEND_METHOD_DEFAULT},{"SRGB Off",OBS_BLEND_METHOD_SRGB_OFF}};}
inline QMenu*choiceMenu(QMenu*parent,const QString&title,const QVector<MenuChoice>&choices,int current,std::function<void(int)> apply){
 auto*m=parent->addMenu(title);auto*g=new QActionGroup(m);g->setExclusive(true);
 for(auto c:choices){auto*a=m->addAction(c.label);g->addAction(a);a->setCheckable(true);a->setChecked(c.value==current);QObject::connect(a,&QAction::triggered,m,[apply,c]{apply(c.value);});}
 return m;
}
inline obs_transform_info resetTransform(){
 obs_transform_info t{};t.scale.x=t.scale.y=1;t.alignment=OBS_ALIGN_LEFT|OBS_ALIGN_TOP;t.bounds_type=OBS_BOUNDS_NONE;t.bounds_alignment=OBS_ALIGN_CENTER;return t;
}
