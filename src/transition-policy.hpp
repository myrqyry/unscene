#pragma once
#include <QString>
#include <QtGlobal>
struct SceneOverride {QString name;int duration=300;};
inline int transitionDuration(int value,int fallback){return qBound(50,value<=0?fallback:value,20000);}
inline SceneOverride chooseSceneOverride(const SceneOverride&incoming,const SceneOverride&outgoing){return incoming.name.isEmpty()?outgoing:incoming;}
