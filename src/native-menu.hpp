#pragma once
#include <QAbstractItemModel>
#include <QString>
#include <QVector>
#include <cstdint>

struct NativeRow{QString parent;int64_t id;QString uid,title;};
inline bool canUseNativeMenu(const QString&target,const QString&editing,const QString&nativeCurrent){
 return !target.isEmpty()&&target==editing&&target==nativeCurrent;
}
// The native model only publishes names. Verify the complete visible sequence,
// then locate the exact item by owning scene and item ID, not source name.
inline int nativeRowIndex(const QVector<NativeRow>&rows,const QAbstractItemModel*model,const QString&parent,int64_t id,const QString&uid){
 if(!model||model->rowCount()!=rows.size())return -1;
 int result=-1;
 for(int i=0;i<rows.size();++i){
  auto name=model->index(i,0).data(Qt::AccessibleTextRole);
  if(!name.isValid()||name.toString()!=rows[i].title)return -1;
  if(rows[i].parent==parent&&rows[i].id==id&&rows[i].uid==uid){
   if(result!=-1)return -1;
   result=i;
  }
 }
 return result;
}
