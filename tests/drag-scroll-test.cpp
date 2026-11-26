#include "../src/drag-scroll.hpp"
#include <QCoreApplication>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);
 QRect view(0,0,200,300);
 CHECK(dragScrollStep(view,{100,150},40)==0);
 CHECK(dragScrollStep(view,{100,0},40)==-20);
 CHECK(dragScrollStep(view,{100,299},40)==20);
 CHECK(dragScrollStep(view,{100,20},40)<0);
 CHECK(dragScrollStep(view,{100,279},40)>0);
 CHECK(dragScrollStep(view,{-1,299},40)==0);
 CHECK(dragScrollStep(view,{100,300},40)==0);
 CHECK(dragScrollStep(QRect(0,0,100,20),{50,10},40)==0);
 CHECK(dragScrollStep(QRect(),{0,0},40)==0);
 return 0;
}
