#include "../src/source-picker.hpp"
#include <QApplication>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QApplication app(argc,argv);
 SourcePicker picker("Gameplay",{{"uuid-1","Camera"},{"uuid-2","Image"}},{{"image_source","Image"},{"color_source","Color Source"}},nullptr);
 picker.show();app.processEvents();
 auto*filter=picker.findChild<QLineEdit*>("sourceFilter");
 auto*list=picker.findChild<QListWidget*>("sourceChoices");
 auto*tabs=picker.findChild<QTabBar*>("sourceMode");
 CHECK(filter&&list&&tabs);
 CHECK(list->count()==2);
 filter->setText("cam");CHECK(list->count()==1);
 list->setCurrentRow(0);CHECK(picker.choice().id=="uuid-1"&&!picker.choice().create);
 tabs->setCurrentIndex(1);CHECK(filter->text().isEmpty()&&list->count()==2);
 filter->setText("color");CHECK(list->count()==1);
 list->setCurrentRow(0);CHECK(picker.choice().id=="color_source"&&picker.choice().create);
 filter->setText("nothing");CHECK(list->count()==0&&picker.choice().id.isEmpty());
 return 0;
}
