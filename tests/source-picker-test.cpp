#include "../src/source-picker.hpp"
#include <QApplication>
#include "widget-events.hpp"
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QApplication app(argc,argv);
 SourcePicker picker("Gameplay",{{"uuid-1","Camera",false,{},"camera"},{"uuid-2","Image",false,{},"image"},{"uuid-3","Image 2",false,{},"image"}},{{"image_source","Image",true,{},"image"},{"camera","Camera",true,{},"camera"}},nullptr);
 picker.show();app.processEvents();
 auto*filter=picker.findChild<QLineEdit*>("sourceFilter");
 auto*list=picker.findChild<QListWidget*>("sourceChoices");
 auto*tabs=picker.findChild<QTabBar*>("sourceMode");
 auto*types=picker.findChild<QComboBox*>("sourceExistingType");
 auto*existing=picker.findChild<QListWidget*>("sourceExistingChoices");
 auto*visible=picker.findChild<QCheckBox*>("sourceVisible");
 auto*add=picker.findChild<QPushButton*>("sourceAddExisting");
 CHECK(filter&&list&&tabs&&types&&existing&&visible&&add);
 CHECK(tabs->currentIndex()==0&&tabs->tabText(0)=="Add new"&&list->count()==2);
 CHECK(visible->isChecked()&&picker.initialVisibility());
 list->setCurrentRow(0);CHECK(picker.choice().create);
 tabs->setCurrentIndex(1);CHECK(existing->count()==3);
 types->setCurrentIndex(types->findData("image"));CHECK(existing->count()==2);
 existing->item(0)->setSelected(true);existing->item(1)->setSelected(true);
 CHECK(picker.choices().size()==2&&add->isEnabled());
 filter->setText("Image 2");CHECK(existing->count()==1&&picker.choices().size()==1&&picker.choice().id=="uuid-3");
 filter->setText("missing");CHECK(existing->count()==0&&!add->isEnabled()&&picker.choice().id.isEmpty());
 visible->setChecked(false);CHECK(!picker.initialVisibility());
 filter->clear();existing->item(0)->setSelected(true);existing->item(1)->setSelected(true);
 add->click();CHECK(picker.result()==QDialog::Accepted&&picker.choices().size()==2);
 SourcePicker create("Scene",{},{{"color","Color",true,{},"color"}},nullptr);
 create.show();app.processEvents();
 auto*newList=create.findChild<QListWidget*>("sourceChoices");
 CHECK(newList&&newList->count()==1);
 auto point=newList->visualItemRect(newList->item(0)).center();
 QMouseEvent press(QEvent::MouseButtonPress,QPointF(point),QPointF(newList->viewport()->mapToGlobal(point)),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
 QApplication::sendEvent(newList->viewport(),&press);
 QMouseEvent release(QEvent::MouseButtonRelease,QPointF(point),QPointF(newList->viewport()->mapToGlobal(point)),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
 QApplication::sendEvent(newList->viewport(),&release);
 CHECK(create.result()==QDialog::Accepted&&create.choice().id=="color");
 const QRect work(-500,20,500,320),dock(-490,30,180,250);
 const auto rect=sourcePickerRect(work,dock);
 CHECK(work.contains(rect)&&rect.width()<=380&&rect.height()<=400);
 return 0;
}
