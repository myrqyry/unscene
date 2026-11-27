#pragma once
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QLineEdit>
#include <QTabBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QComboBox>
#include <QStackedWidget>
#include <QLabel>
#include <QVector>
#include <QSet>
#include <QIcon>
#include <QScreen>
#include <QGuiApplication>
#include <QSignalBlocker>
#include <QShortcut>
#include <QStyle>
#include <algorithm>

struct SourceChoice {QString id,label;bool create=false;QIcon icon;QString type;};
inline QRect sourcePickerRect(const QRect&work,const QRect&dock){
 if(work.isEmpty())return {};
 const int w=qMax(1,qMin(380,work.width()-16)),h=qMax(1,qMin(400,work.height()-16));
 const bool left=dock.center().x()<work.center().x();
 return {qBound(work.left(),left?dock.right()-7:dock.left()-w+8,work.right()-w+1),
         qBound(work.top(),dock.bottom()-h+9,work.bottom()-h+1),w,h};
}
class SourcePicker:public QDialog {
 QVector<SourceChoice> existing,types;
 QLineEdit*filter;
 QListWidget*list,*existingList;
 QTabBar*tabs;
 QComboBox*typeFilter;
 QStackedWidget*pages;
 QCheckBox*visible;
 QPushButton*add;
 QLabel*empty;
 static void compact(QListWidget*w,QSize icons){
  w->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Expanding);w->setMinimumSize(0,0);
  w->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
  w->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);w->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  w->setTextElideMode(Qt::ElideRight);w->setIconSize(icons);w->setUniformItemSizes(true);w->setSpacing(0);
  w->setStyleSheet("QListWidget::item {padding: 2px 5px; min-height: 18px;}");
 }
 static void sort(QVector<SourceChoice>&values){
  std::stable_sort(values.begin(),values.end(),[](const auto&a,const auto&b){return QString::localeAwareCompare(a.label,b.label)<0;});
 }
 static QListWidgetItem* append(QListWidget*w,const SourceChoice&v,int height){
  auto*i=new QListWidgetItem(v.icon,v.label,w);i->setData(Qt::UserRole,v.id);i->setToolTip(v.label);
  i->setSizeHint(QSize(0,qMax(height,w->fontMetrics().height()+6)));return i;
 }
 void updateSelection(){add->setEnabled(!existingList->selectedItems().isEmpty());}
 void populateExisting(){
  QSet<QString> selected;for(auto*i:existingList->selectedItems())selected.insert(i->data(Qt::UserRole).toString());
  const QSignalBlocker block(existingList);existingList->clear();
  const auto type=typeFilter->currentData().toString(),query=filter->text().trimmed();
  for(const auto&v:existing){
   if((!type.isEmpty()&&v.type!=type)||!v.label.contains(query,Qt::CaseInsensitive))continue;
   auto*i=append(existingList,v,44);i->setSelected(selected.contains(v.id));
  }
  // Filtering never retains hidden selections. A sole match is convenient
  // to add with Enter; multiple matches require explicit selection.
  if(existingList->count()==1&&existingList->selectedItems().isEmpty())existingList->item(0)->setSelected(true);
  empty->setVisible(!existingList->count());updateSelection();
 }
public:
 SourcePicker(const QString&target,QVector<SourceChoice>e,QVector<SourceChoice>t,QWidget*parent):QDialog(parent),existing(std::move(e)),types(std::move(t)){
  setWindowTitle("Add Source");resize(380,400);
  if(parent){setWindowFlags(Qt::Popup|Qt::FramelessWindowHint);setWindowModality(Qt::WindowModal);}
  auto*l=new QVBoxLayout(this);l->setContentsMargins(6,4,6,6);l->setSpacing(4);l->setSizeConstraint(QLayout::SetNoConstraint);
  auto*heading=new QHBoxLayout;heading->setContentsMargins(4,0,2,0);
  auto*label=new QLabel("Add source — "+target,this);label->setTextFormat(Qt::PlainText);label->setToolTip(label->text());
  label->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);auto font=label->font();font.setWeight(QFont::DemiBold);label->setFont(font);
  heading->addWidget(label,1);
  auto*close=new QToolButton(this);close->setAutoRaise(true);close->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
  close->setToolTip("Close Add Source");close->setAccessibleName(close->toolTip());heading->addWidget(close);
  connect(close,&QToolButton::clicked,this,&QDialog::reject);l->addLayout(heading);
  tabs=new QTabBar(this);tabs->setObjectName("sourceMode");tabs->addTab("Add new");tabs->addTab("Add existing");tabs->setExpanding(true);tabs->setDrawBase(false);l->addWidget(tabs);
  pages=new QStackedWidget(this);pages->setMinimumSize(0,0);pages->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Expanding);l->addWidget(pages,1);
  list=new QListWidget(pages);list->setObjectName("sourceChoices");compact(list,{16,16});sort(types);
  for(const auto&v:types)append(list,v,22);
  pages->addWidget(list);
  auto*page=new QWidget(pages);auto*layout=new QVBoxLayout(page);layout->setContentsMargins(0,0,0,0);layout->setSpacing(4);
  typeFilter=new QComboBox(page);typeFilter->setObjectName("sourceExistingType");typeFilter->setAccessibleName("Existing source type");
  typeFilter->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);typeFilter->addItem("All source types",QString{});
  sort(existing);QSet<QString> present;for(const auto&v:existing)present.insert(v.type);
  for(const auto&v:types)if(!v.type.isEmpty()&&present.remove(v.type))typeFilter->addItem(v.icon,v.label,v.type);
  QStringList remaining=present.values();remaining.sort(Qt::CaseInsensitive);for(const auto&type:remaining)if(!type.isEmpty())typeFilter->addItem(type,type);
  layout->addWidget(typeFilter);
  filter=new QLineEdit(page);filter->setObjectName("sourceFilter");filter->setPlaceholderText("Filter by source name");filter->setClearButtonEnabled(true);filter->setAccessibleName("Filter existing sources");layout->addWidget(filter);
  existingList=new QListWidget(page);existingList->setObjectName("sourceExistingChoices");compact(existingList,{64,36});existingList->setSelectionMode(QAbstractItemView::MultiSelection);layout->addWidget(existingList,1);
  empty=new QLabel("No matching sources",page);empty->setAlignment(Qt::AlignCenter);layout->addWidget(empty);
  add=new QPushButton("Add existing",page);add->setObjectName("sourceAddExisting");layout->addWidget(add);
  pages->addWidget(page);
  visible=new QCheckBox("Make source visible",this);visible->setObjectName("sourceVisible");visible->setChecked(true);l->addWidget(visible);
  connect(tabs,&QTabBar::currentChanged,this,[this](int index){pages->setCurrentIndex(index);if(index==1){populateExisting();filter->setFocus();}else list->setFocus();});
  connect(filter,&QLineEdit::textChanged,this,[this]{populateExisting();});
  connect(typeFilter,&QComboBox::currentIndexChanged,this,[this]{populateExisting();});
  connect(existingList,&QListWidget::itemSelectionChanged,this,[this]{updateSelection();});
  auto create=[this](QListWidgetItem*i){if(!i||tabs->currentIndex()!=0)return;list->setCurrentItem(i);accept();};
  connect(list,&QListWidget::itemClicked,this,create);
  for(auto key:{Qt::Key_Return,Qt::Key_Enter}){
   auto*shortcut=new QShortcut(QKeySequence(key),list);shortcut->setContext(Qt::WidgetShortcut);connect(shortcut,&QShortcut::activated,this,[this,create]{create(list->currentItem());});
  }
  connect(add,&QPushButton::clicked,this,[this]{if(!choices().isEmpty())accept();});
  connect(filter,&QLineEdit::returnPressed,this,[this]{if(add->isEnabled())add->click();});
  if(list->count())list->setCurrentRow(0);populateExisting();list->setFocus();
  if(parent){
   auto dock=QRect(parent->mapToGlobal(QPoint(0,0)),parent->size());
   auto*screen=QGuiApplication::screenAt(dock.center());if(!screen)screen=parent->screen();
   auto work=screen?screen->availableGeometry():dock;
   setGeometry(sourcePickerRect(work,dock));
  }
 }
 QVector<SourceChoice> choices()const{
  QVector<SourceChoice> out;
  auto appendChoice=[&](QListWidgetItem*i,const QVector<SourceChoice>&values){
   if(!i)return;const auto id=i->data(Qt::UserRole).toString();
   for(const auto&v:values)if(v.id==id){out.append(v);break;}
  };
  if(tabs->currentIndex()==0)appendChoice(list->currentItem(),types);
  else for(int i=0;i<existingList->count();++i)if(existingList->item(i)->isSelected())appendChoice(existingList->item(i),existing);
  return out;
 }
 SourceChoice choice()const{const auto all=choices();return all.isEmpty()?SourceChoice{}:all.first();}
 bool initialVisibility()const{return visible->isChecked();}
};
