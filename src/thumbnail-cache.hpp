#pragma once
#include <QCache>
#include <QPixmap>
#include <QString>
// QCache evicts the least recently used entry when the entry budget is exceeded.
class ThumbnailCache {
 QCache<QString,QPixmap> cache;
public:
 explicit ThumbnailCache(int capacity=512):cache(capacity){}
 bool contains(const QString&key)const{return cache.contains(key);}
 QPixmap lookup(const QString&key)const{auto*image=cache.object(key);return image?*image:QPixmap{};}
 void insert(const QString&key,const QPixmap&image){cache.insert(key,new QPixmap(image));}
 void remove(const QString&key){cache.remove(key);}
 void clear(){cache.clear();}
};
