#pragma once
#include <QRect>
#include <QtGlobal>

// Logical pixels per GUI timer tick. Keep a neutral middle even in short docks.
inline int dragScrollStep(const QRect&view,QPoint point,int preferredEdge){
 if(!view.contains(point)||view.height()<24||preferredEdge<=0)return 0;
 const int edge=qMin(preferredEdge,view.height()/3);
 const int top=point.y()-view.top(),bottom=view.bottom()-point.y();
 constexpr int maxPixelsPerTick=20;
 auto speed=[edge](int distance){return qMax(1,(maxPixelsPerTick*(edge-distance)+edge-1)/edge);};
 if(top<edge)return -speed(top);
 if(bottom<edge)return speed(bottom);
 return 0;
}
