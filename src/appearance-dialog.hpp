#pragma once
#include "appearance.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QFormLayout>

class AppearanceDialog:public QDialog {
 Appearance draft;
 QComboBox *mode;
 QLabel *preview;
 QPushButton *colorButton,*importButton;
 void updatePreview(){
  auto pix=appearancePixmap(draft,palette(),QSize(96,64));
  preview->setPixmap(pix);
  if(pix.isNull())preview->setText("Automatic thumbnail");
  colorButton->setText(draft.color.isValid()?draft.color.name():"Theme default");
  if(draft.color.isValid()){
   QPixmap swatch(16,16);swatch.fill(draft.color);colorButton->setIcon(QIcon(swatch));
  }else colorButton->setIcon(QIcon());
  importButton->setText(draft.mode=="custom"&&!draft.image.isEmpty()?"Replace image…":"Choose image…");
 }
public:
 AppearanceDialog(const QString&title,const Appearance&initial,QWidget*parent):QDialog(parent),draft(initial){
  setWindowTitle("Appearance — "+title);setMinimumWidth(310);
  auto*l=new QVBoxLayout(this);
  preview=new QLabel(this);preview->setAlignment(Qt::AlignCenter);preview->setMinimumHeight(70);l->addWidget(preview);
  auto*form=new QFormLayout;
  mode=new QComboBox(this);mode->addItem("Automatic snapshot / source icon","auto");
  const QStringList ids={"star","heart","camera","game","music","mic","chat","screen","image","folder","moon","bolt","layers"};
  for(const auto&id:ids){Appearance a;a.mode=id;auto label=id;label[0]=label[0].toUpper();mode->addItem(QIcon(appearancePixmap(a,palette(),QSize(24,24))),label,id);}
  mode->addItem("Custom image or icon file","custom");
  mode->setCurrentIndex(qMax(0,mode->findData(draft.mode)));
  form->addRow("Artwork",mode);
  importButton=new QPushButton(this);form->addRow("",importButton);
  auto*colors=new QHBoxLayout;colorButton=new QPushButton(this);colors->addWidget(colorButton);
  auto*clearColor=new QPushButton("Clear",this);colors->addWidget(clearColor);form->addRow("Row color",colors);
  l->addLayout(form);
  auto*hint=new QLabel("Applies to this scene or source everywhere in Unscene. Does not change OBS output.",this);hint->setWordWrap(true);l->addWidget(hint);
  auto*buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Reset,this);l->addWidget(buttons);
  auto chooseImage=[this]{
   auto path=QFileDialog::getOpenFileName(this,"Choose artwork",QString(),"Images (*.png *.jpg *.jpeg *.webp *.bmp *.svg *.ico);;All files (*)",nullptr,QFileDialog::DontUseNativeDialog);
   if(path.isEmpty())return false;
   auto image=importArtwork(path);
   if(image.isEmpty()){QMessageBox::warning(this,"Unscene","Could not read this image, or it exceeds the import limit.");return false;}
   draft.image=image;draft.mode="custom";
   {QSignalBlocker blocker(mode);mode->setCurrentIndex(mode->findData("custom"));}
   updatePreview();return true;
  };
  connect(mode,&QComboBox::currentIndexChanged,this,[this,chooseImage](int){
   const auto next=mode->currentData().toString();
   if(next=="custom"&&draft.image.isEmpty()){
    if(!chooseImage()){QSignalBlocker blocker(mode);mode->setCurrentIndex(mode->findData(draft.mode));}return;
   }
   draft.mode=next;updatePreview();
  });
  connect(importButton,&QPushButton::clicked,this,[chooseImage]{chooseImage();});
  connect(colorButton,&QPushButton::clicked,this,[this]{
   auto color=QColorDialog::getColor(draft.color.isValid()?draft.color:palette().color(QPalette::Highlight),this,"Row color",QColorDialog::DontUseNativeDialog);
   if(color.isValid()){draft.color=color;updatePreview();}
  });
  connect(clearColor,&QPushButton::clicked,this,[this]{draft.color=QColor();updatePreview();});
  connect(buttons->button(QDialogButtonBox::Reset),&QPushButton::clicked,this,[this]{
   draft=Appearance{};{QSignalBlocker blocker(mode);mode->setCurrentIndex(0);}updatePreview();
  });
  connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
  connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
  updatePreview();
 }
 Appearance value()const{return draft;}
};
