#pragma once
#include <QSettings>
#include <QImageReader>
#include <QBuffer>
#include <QFileInfo>
#include <QColor>
#include <QPalette>
#include <QPixmap>
#include <QPainter>
#include <QSvgRenderer>
#include <QHash>
#include <QSize>

struct Appearance {
 QString mode="auto";
 QColor color;
 QByteArray image;
};
inline const QHash<QString,QString>& iconPaths(){
 static const QHash<QString,QString> paths={
  {"star","<path d='m12 3 2.8 5.7 6.2.9-4.5 4.4 1.1 6.2L12 17.3l-5.6 2.9 1.1-6.2L3 9.6l6.2-.9Z'/>"},
  {"heart","<path d='M20.8 8.6c0 4.2-8.8 10.1-8.8 10.1S3.2 12.8 3.2 8.6a4.8 4.8 0 0 1 8.8-2.5 4.8 4.8 0 0 1 8.8 2.5Z'/>"},
  {"camera","<rect x='3' y='6' width='13' height='12' rx='2'/><path d='m16 10 5-3v10l-5-3'/>"},
  {"game","<rect x='3' y='7' width='18' height='10' rx='4'/><path d='M8 10v4m-2-2h4m6-1h.1m2 2h.1'/>"},
  {"music","<path d='M9 17V5l11-2v12M9 8l11-2'/><ellipse cx='6' cy='17' rx='3' ry='2'/><ellipse cx='17' cy='15' rx='3' ry='2'/>"},
  {"mic","<rect x='9' y='3' width='6' height='12' rx='3'/><path d='M5 11a7 7 0 0 0 14 0M12 18v3m-4 0h8'/>"},
  {"chat","<path d='M4 4h16v13H10l-6 4Z'/><path d='M8 9h8m-8 4h5'/>"},
  {"screen","<rect x='3' y='4' width='18' height='13' rx='2'/><path d='M12 17v4m-4 0h8'/>"},
  {"image","<rect x='3' y='3' width='18' height='18' rx='2'/><circle cx='8' cy='8' r='1.5'/><path d='m3 17 5-5 4 4 4-6 5 6'/>"},
  {"folder","<path d='M3 6h7l2 3h9v11H3Z'/>"},
  {"moon","<path d='M20 15A9 9 0 0 1 9 4a9 9 0 1 0 11 11Z'/>"},
  {"bolt","<path d='m13 3-9 11h7l-1 7 10-12h-7Z'/>"},
  {"layers","<path d='m12 3 9 5-9 5-9-5Zm-9 9 9 5 9-5M3 16l9 5 9-5'/>"}
 };return paths;
}
inline QByteArray importArtwork(const QString &path){
 QFileInfo file(path);if(!file.isFile()||file.size()>16*1024*1024)return {};
 QImageReader reader(path);reader.setAutoTransform(true);
 const auto size=reader.size();
 if(!size.isValid()||size.width()>8192||size.height()>8192||qint64(size.width())*size.height()>32000000)return {};
 reader.setScaledSize(size.scaled(QSize(256,256),Qt::KeepAspectRatio));
 auto image=reader.read();if(image.isNull())return {};
 if(image.width()>256||image.height()>256)image=image.scaled(256,256,Qt::KeepAspectRatio,Qt::SmoothTransformation);
 QByteArray data;QBuffer buffer(&data);buffer.open(QIODevice::WriteOnly);
 if(!image.save(&buffer,"PNG"))return {};return data;
}
class AppearanceStore {
 QSettings settings;
 static QString key(const QString &uid){return "appearance/"+QString::fromLatin1(uid.toUtf8().toHex());}
public:
 explicit AppearanceStore(const QString &path):settings(path,QSettings::IniFormat){}
 Appearance load(const QString &uid){
  Appearance a;if(uid.isEmpty())return a;
  const auto prefix=key(uid);
  a.mode=settings.value(prefix+"/mode","auto").toString();
  a.color=QColor(settings.value(prefix+"/color").toString());
  a.image=settings.value(prefix+"/image").toByteArray();
  if(a.image.size()>512*1024)a.image.clear();
  if(a.mode!="auto"&&a.mode!="custom"&&!iconPaths().contains(a.mode))a.mode="auto";
  if(a.mode=="custom"&&a.image.isEmpty())a.mode="auto";
  return a;
 }
 bool save(const QString &uid,const Appearance&a){
  if(uid.isEmpty())return false;
  auto prefix=key(uid);
  if(a.mode=="auto"&&!a.color.isValid())settings.remove(prefix);
  else{
   settings.setValue(prefix+"/mode",a.mode);
   if(a.color.isValid())settings.setValue(prefix+"/color",a.color.name());else settings.remove(prefix+"/color");
   if(a.mode=="custom")settings.setValue(prefix+"/image",a.image);else settings.remove(prefix+"/image");
  }
  settings.sync();return settings.status()==QSettings::NoError;
 }
};
inline QPixmap appearancePixmap(const Appearance&a,const QPalette &palette,const QSize &size){
 if(a.mode=="custom"){
  auto pix=QPixmap::fromImage(QImage::fromData(a.image,"PNG"));
  if(!pix.isNull())return pix.scaled(size,Qt::KeepAspectRatio,Qt::SmoothTransformation);
 }
 const auto path=iconPaths().value(a.mode);
 if(path.isEmpty())return {};
 const int side=qMin(size.width(),size.height())-4;if(side<=0)return {};
 auto color=palette.color(QPalette::WindowText);
 // Color is a row accent; use native text color for predictable icon contrast.
 auto svg=QString("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%1' stroke-width='1.8' stroke-linejoin='round' stroke-linecap='round'>%2</svg>").arg(color.name(),path);
 QSvgRenderer renderer(svg.toUtf8());QPixmap pix(side,side);pix.fill(Qt::transparent);
 QPainter painter(&pix);renderer.render(&painter);return pix;
}
