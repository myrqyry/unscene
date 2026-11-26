#pragma once
#include <QColor>
#include <QPalette>
#include <QtGlobal>
#include <cmath>

// Material-inspired tonal roles in OKLab, anchored to the OBS palette.
// This is not Google's HCT/Expressive scheme: keep the chosen scene hue
// recognizable, and preserve the host theme rather than replacing it.
namespace SceneColor {
struct Lab {double l,a,b;};
inline double linear(double v){return v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4);}
inline double srgb(double v){return v<=.0031308?12.92*v:1.055*std::pow(v,1/2.4)-.055;}
inline Lab lab(const QColor&c){
 const double r=linear(c.redF()),g=linear(c.greenF()),b=linear(c.blueF());
 const double l=std::cbrt(.4122214708*r+.5363325363*g+.0514459929*b);
 const double m=std::cbrt(.2119034982*r+.6806995451*g+.1073969566*b);
 const double s=std::cbrt(.0883024619*r+.2817188376*g+.6299787005*b);
 return {.2104542553*l+.7936177850*m-.0040720468*s,1.9779984951*l-2.4285922050*m+.4505937099*s,.0259040371*l+.7827717662*m-.8086757660*s};
}
inline QColor tone(double light,double hue,double chroma){
 light=qBound(0.0,light,1.0);
 // Reduce chroma rather than clipping individual RGB channels.
 for(int i=0;i<40;++i){
  double a=std::cos(hue)*chroma,b=std::sin(hue)*chroma;
  double l=light+.3963377774*a+.2158037573*b,m=light-.1055613458*a-.0638541728*b,s=light-.0894841775*a-1.2914855480*b;
  l=l*l*l;m=m*m*m;s=s*s*s;
  double r=4.0767416621*l-3.3077115913*m+.2309699292*s,g=-1.2684380046*l+2.6097574011*m-.3413193965*s,bl=-.0041960863*l-.7034186147*m+1.7076147010*s;
  if(r>=-1e-7&&g>=-1e-7&&bl>=-1e-7&&r<=1.0000001&&g<=1.0000001&&bl<=1.0000001)
   return QColor::fromRgbF(srgb(qBound(0.0,r,1.0)),srgb(qBound(0.0,g,1.0)),srgb(qBound(0.0,bl,1.0)));
  chroma*=.78;
 }
 return QColor::fromRgbF(srgb(light*light*light),srgb(light*light*light),srgb(light*light*light));
}
inline double luminance(const QColor&c){return .2126*linear(c.redF())+.7152*linear(c.greenF())+.0722*linear(c.blueF());}
}
inline double surfaceContrast(const QColor&a,const QColor&b){
 auto x=SceneColor::luminance(a),y=SceneColor::luminance(b);return (qMax(x,y)+.05)/(qMin(x,y)+.05);
}
inline QColor contrastInk(const QColor&surface,const QColor&preferred,double minimum=4.5){
 if(surfaceContrast(surface,preferred)>=minimum)return preferred;
 return surfaceContrast(surface,Qt::white)>surfaceContrast(surface,Qt::black)?QColor(Qt::white):QColor(Qt::black);
}
struct SceneSurfaceRoles {QColor container,header,accent,onContainer,onHeader;};
inline SceneSurfaceRoles sceneSurfaceRoles(const QPalette&palette,const QColor&seed){
 using namespace SceneColor;
 auto base=palette.color(QPalette::Base),fg=palette.color(QPalette::WindowText);
 auto bg=lab(base),key=lab(seed.isValid()?seed:base);
 bool dark=luminance(base)<.35;
 double c=std::hypot(key.a,key.b),hue=std::atan2(key.b,key.a);
 double containerTone=qBound(.06,bg.l+(dark?.045:-.033),.97);
 double headerTone=qBound(.09,bg.l+(dark?.105:-.085),.96);
 double neutralChroma=qMin(c*.30,.045),headerChroma=qMin(c*.52,.078);
 SceneSurfaceRoles out;
 out.container=tone(containerTone,hue,neutralChroma);
 out.header=tone(headerTone,hue,headerChroma);
 out.onContainer=contrastInk(out.container,fg);
 out.onHeader=contrastInk(out.header,fg);
 out.accent=tone(dark?.76:.44,hue,qMin(c,.16));
 if(surfaceContrast(out.container,out.accent)<3)out.accent=contrastInk(out.container,out.accent,3);
 return out;
}
