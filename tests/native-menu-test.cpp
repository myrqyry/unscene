#include "../src/native-menu.hpp"
#include <QApplication>
#include <QStandardItemModel>
#include <QListView>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QApplication app(argc,argv);
 QVector<NativeRow> rows={{"scene",3,"uuid","Image"},{"scene",2,"uuid","Image"},{"scene",1,"color","Color"}};
 QStandardItemModel model;
 for(auto&r:rows){auto*i=new QStandardItem;i->setData(r.title,Qt::AccessibleTextRole);model.appendRow(i);}
 CHECK(nativeRowIndex(rows,&model,"scene",2,"uuid")==1);
 CHECK(nativeRowIndex(rows,&model,"scene",3,"uuid")==0);
 CHECK(nativeRowIndex(rows,&model,"other",2,"uuid")==-1);
 CHECK(nativeRowIndex(rows,&model,"scene",2,"wrong")==-1);
 model.item(0)->setData("Different",Qt::AccessibleTextRole);
 CHECK(nativeRowIndex(rows,&model,"scene",2,"uuid")==-1);
 CHECK(canUseNativeMenu("a","a","a"));
 CHECK(!canUseNativeMenu("a","b","b"));
 CHECK(!canUseNativeMenu("a","a","b"));
 CHECK(!canUseNativeMenu("","",""));
 return 0;
}
