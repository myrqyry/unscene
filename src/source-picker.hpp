#pragma once
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QLineEdit>
#include <QTabBar>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QVector>
#include <QIcon>
#include <algorithm>

struct SourceChoice {QString id,label;bool create=false;QIcon icon;};
class SourcePicker:public QDialog {
 QVector<SourceChoice> existing,types;
 QLineEdit*filter;
 QListWidget*list;
 QTabBar*tabs;
 QDialogButtonBox*buttons;
 void populate(){
  list->clear();
  auto values=tabs->currentIndex()==0?existing:types;
  std::sort(values.begin(),values.end(),[](const auto&a,const auto&b){return QString::localeAwareCompare(a.label,b.label)<0;});
  for(const auto&v:values){
   if(!v.label.contains(filter->text().trimmed(),Qt::CaseInsensitive))continue;
   auto*item=new QListWidgetItem(v.icon,v.label,list);item->setData(Qt::UserRole,v.id);
  }
  if(list->count())list->setCurrentRow(0);
  buttons->button(QDialogButtonBox::Ok)->setEnabled(list->currentItem()!=nullptr);
 }
public:
 SourcePicker(const QString&target,QVector<SourceChoice>e,QVector<SourceChoice>t,QWidget*parent):QDialog(parent),existing(std::move(e)),types(std::move(t)){
  setWindowTitle("Add source");resize(340,420);
  auto*l=new QVBoxLayout(this);
  auto*label=new QLabel("Add to "+target,this);label->setTextFormat(Qt::PlainText);l->addWidget(label);
  tabs=new QTabBar(this);tabs->setObjectName("sourceMode");tabs->addTab("Existing");tabs->addTab("New source");tabs->setExpanding(true);l->addWidget(tabs);
  filter=new QLineEdit(this);filter->setObjectName("sourceFilter");filter->setPlaceholderText("Find a source…");filter->setClearButtonEnabled(true);l->addWidget(filter);
  list=new QListWidget(this);list->setObjectName("sourceChoices");list->setIconSize(QSize(20,20));list->setUniformItemSizes(true);l->addWidget(list,1);
  buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this);buttons->button(QDialogButtonBox::Ok)->setText("Add");l->addWidget(buttons);
  connect(filter,&QLineEdit::textChanged,this,[this]{populate();});
  connect(tabs,&QTabBar::currentChanged,this,[this]{filter->clear();populate();});
  connect(list,&QListWidget::currentItemChanged,this,[this]{buttons->button(QDialogButtonBox::Ok)->setEnabled(list->currentItem()!=nullptr);});
  connect(list,&QListWidget::itemDoubleClicked,this,[this]{if(!choice().id.isEmpty())accept();});
  connect(buttons,&QDialogButtonBox::accepted,this,[this]{if(!choice().id.isEmpty())accept();});
  connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
  populate();
  if(existing.isEmpty())tabs->setCurrentIndex(1);
  filter->setFocus();
 }
 SourceChoice choice()const{
  auto*i=list->currentItem();if(!i)return {};
  return {i->data(Qt::UserRole).toString(),i->text(),tabs->currentIndex()==1,i->icon()};
 }
};
