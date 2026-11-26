#include "../src/filter-panel.hpp"
#include <QApplication>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QApplication app(argc,argv);FilterPanel panel;
 QVector<FilterEntry> entries={{"a","Gain","gain",true,true},{"b","Crop","crop",false,false},{"c","Color","color",true,false}};
 panel.setEntries(entries);auto*list=panel.findChild<QListWidget*>("filterStack");CHECK(list&&list->count()==3);
 QString id,action;panel.onAction=[&](QString a,QString i){action=a;id=i;};
 list->item(0)->setToolTip("updated tooltip");CHECK(action.isEmpty());
 list->item(0)->setText("updated display");CHECK(action.isEmpty());
 list->setCurrentRow(1);list->item(1)->setCheckState(Qt::Checked);CHECK(id=="b"&&action=="enable");
 list->item(1)->setCheckState(Qt::Unchecked);CHECK(id=="b"&&action=="disable");
 action.clear();list->item(1)->setToolTip("new tooltip");CHECK(action.isEmpty());
 panel.setEntries({entries[2],entries[1],entries[0]});CHECK(panel.selectedId()=="b");
 for(auto*b:panel.findChildren<QToolButton*>())if(b->accessibleName()=="Enable filter"){action.clear();b->click();CHECK(action=="enable"&&id=="b");}
 CHECK(list->currentItem()->text().endsWith("(disabled)"));
 CHECK(filterCompatible(OBS_SOURCE_VIDEO,OBS_SOURCE_VIDEO));
 CHECK(!filterCompatible(OBS_SOURCE_VIDEO,OBS_SOURCE_AUDIO));
 CHECK(!filterCompatible(OBS_SOURCE_VIDEO,OBS_SOURCE_ASYNC_VIDEO));
 CHECK(filterCompatible(OBS_SOURCE_ASYNC_VIDEO,OBS_SOURCE_ASYNC_VIDEO));
 CHECK(filterCompatible(OBS_SOURCE_AUDIO|OBS_SOURCE_VIDEO,OBS_SOURCE_AUDIO));
 return 0;
}
