#pragma once
// Shared Aftermarket v1 note format, adapted from myrqyry/aftermarket
// src/context-notes.cpp (GPL-3.0-or-later). Both plugins can run alone.
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QLockFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPalette>
#include <functional>
#include <memory>
#include <optional>
namespace unscene {
namespace {
constexpr qsizetype maxText=4096;
constexpr qint64 maxFile=4*1024*1024;
bool validKey(const QString &key){
 const auto a=QJsonDocument::fromJson(key.toUtf8()).array();
 if(a.size()!=4||!a[0].isString()||!a[1].isString()||!a[2].isString()||!a[3].isString())return false;
 const auto kind=a[0].toString(),id=a[1].toString(),owner=a[2].toString(),item=a[3].toString();
 auto token=[](const QString &v){return !v.isEmpty()&&v.size()<=128&&!v.contains(QChar(0));};
 if(!token(id))return false;
 if(kind=="source"||kind=="scene")return owner.isEmpty()&&item=="-1";
 if(kind=="filter")return token(owner)&&item=="-1";
 bool ok=false;const auto n=item.toLongLong(&ok);
 return kind=="item"&&token(owner)&&ok&&n>=0&&QString::number(n)==item;
}
 }
class ContextNotes {
 QString path_;
 QJsonObject notes_;
 std::function<void()> changed_;
 bool read(QJsonObject &)const;
public:
 explicit ContextNotes(QString path);
 QString get(const QString &key)const;
 bool reload();
 void onChanged(std::function<void()> fn){changed_=std::move(fn);}
 bool put(const QString &key,const QString &expected,const QString &text);
};
inline QString noteKey(const QString &kind,const QString &id,const QString &owner={},qint64 item=-1){
 const auto key=QString::fromUtf8(QJsonDocument(QJsonArray{kind,id,owner,QString::number(item)}).toJson(QJsonDocument::Compact));
 return validKey(key)?key:QString{};
}
inline ContextNotes::ContextNotes(QString path):path_(std::move(path)){reload();}
inline QString ContextNotes::get(const QString &key)const{return notes_.value(key).toString();}
inline bool ContextNotes::read(QJsonObject &out)const{
 out={};QFile f(path_);if(!f.exists())return true;
 if(!f.open(QIODevice::ReadOnly)||f.size()>maxFile)return false;
 const auto doc=QJsonDocument::fromJson(f.read(maxFile+1));
 const auto root=doc.object();if(root.value("version").toInt()!=1||!root.value("notes").isObject())return false;
 out=root.value("notes").toObject();
 for(auto it=out.begin();it!=out.end();++it)
  if(!validKey(it.key())||!it.value().isString()||it.value().toString().isEmpty()||it.value().toString().size()>maxText||it.value().toString().contains(QChar(0)))return false;
 return true;
}
inline bool ContextNotes::reload(){QJsonObject next;if(!read(next))return false;if(notes_!=next){notes_=next;if(changed_)changed_();}return true;}
inline bool ContextNotes::put(const QString &key,const QString &expected,const QString &text){
 if(!validKey(key)||text.size()>maxText||text.contains(QChar(0))||!QDir().mkpath(QFileInfo(path_).absolutePath()))return false;
 QLockFile lock(path_+".lock");if(!lock.tryLock(0))return false;
 QJsonObject next;if(!read(next)||next.value(key).toString()!=expected)return false;
 if(text.trimmed().isEmpty())next.remove(key);else next.insert(key,text);
 const auto bytes=QJsonDocument(QJsonObject{{"version",1},{"notes",next}}).toJson(QJsonDocument::Compact);
 if(bytes.size()>maxFile)return false;
 QSaveFile f(path_);f.setDirectWriteFallback(false);
 if(!f.open(QIODevice::WriteOnly)||!f.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner)||f.write(bytes)!=bytes.size()||!f.commit())return false;
 notes_=next;if(changed_)changed_();return true;
}
inline QString noteTooltip(const QString &text){return "<span>"+text.toHtmlEscaped().replace("\n","<br>")+"</span>";}
inline void editContextNote(QWidget *parent,std::shared_ptr<ContextNotes> notes,const QString &key,const QString &name,
                     std::function<bool()> valid,std::function<void()> changed){
 if(!parent||key.isEmpty()||!valid||!valid()||!notes)return;
 if(!notes->reload()){
  auto *error=new QMessageBox(QMessageBox::Warning,"Private note","The note store could not be read. Existing data has not been changed.",QMessageBox::Ok,parent);
  error->setAttribute(Qt::WA_DeleteOnClose);error->open();return;
 }
 const auto existing=notes->get(key);
 auto *dialog=new QDialog(parent);dialog->setObjectName("unscene-note-editor");
 dialog->setWindowTitle("Private note");dialog->setAttribute(Qt::WA_DeleteOnClose);
 auto *layout=new QVBoxLayout(dialog);
 auto *title=new QLabel(name,dialog);title->setTextFormat(Qt::PlainText);title->setWordWrap(true);layout->addWidget(title);
 auto *editor=new QPlainTextEdit(dialog);editor->setObjectName("unscene-note-text");editor->setPlainText(existing);editor->setPlaceholderText("Write a private note…");layout->addWidget(editor);
 auto *feedback=new QLabel("Private local note, shared with Aftermarket. Clear the text and save to remove.",dialog);feedback->setWordWrap(true);layout->addWidget(feedback);
 auto *buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,dialog);layout->addWidget(buttons);
 QObject::connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
 QObject::connect(buttons,&QDialogButtonBox::accepted,dialog,[dialog,editor,feedback,notes,key,existing,valid,changed]{
  const auto value=editor->toPlainText();
  if(!valid()){feedback->setText("Target no longer available. Nothing saved.");return;}
  if(!notes->put(key,existing,value)){feedback->setText("Could not save. Check the 4096-character limit or reopen if another edit changed this note.");return;}
  if(changed)changed();dialog->accept();
 });
 dialog->resize(390,230);dialog->open();editor->setFocus();
}

// Reattach watches after QSaveFile replaces a file; watch the existing
// ancestor as well so a standalone install can create the shared directory.
class SharedFileWatch:public QObject{
 QFileSystemWatcher watcher;QStringList paths;QTimer debounce;std::function<void()> callback;
 void arm(){
  QStringList wanted;
  for(const auto &path:paths){
   if(QFileInfo::exists(path))wanted.append(path);
   QDir dir=QFileInfo(path).absoluteDir();
   while(!dir.exists()&&dir.cdUp()){}
   if(dir.exists())wanted.append(dir.absolutePath());
  }
  wanted.removeDuplicates();
  auto current=watcher.files()+watcher.directories();
  for(const auto &p:current)if(!wanted.contains(p))watcher.removePath(p);
  for(const auto &p:wanted)if(!current.contains(p))watcher.addPath(p);
 }
public:
 SharedFileWatch(QStringList files,std::function<void()> cb,QObject *parent=nullptr):QObject(parent),paths(std::move(files)),callback(std::move(cb)){
  debounce.setSingleShot(true);debounce.setInterval(60);
  connect(&watcher,&QFileSystemWatcher::fileChanged,this,[this]{debounce.start();});
  connect(&watcher,&QFileSystemWatcher::directoryChanged,this,[this]{debounce.start();});
  connect(&debounce,&QTimer::timeout,this,[this]{arm();if(callback)callback();});
  arm();
 }
};
struct RowStripes{
 bool scenes=false,sources=false;int strength=100;QColor base,alternate;
 bool operator==(const RowStripes&b)const{return scenes==b.scenes&&sources==b.sources&&strength==b.strength&&base==b.base&&alternate==b.alternate;}
 static std::optional<RowStripes> parse(const QJsonObject&o){
  RowStripes s;
  for(auto k:{"scenes","sources"})if(o.contains(k)&&!o[k].isBool())return {};
  s.scenes=o["scenes"].toBool();s.sources=o["sources"].toBool();
  if(o.contains("strength")){
   const auto v=o["strength"];const auto d=v.toDouble(-1);
   if(!v.isDouble()||d<0||d>200||d!=int(d))return {};s.strength=int(d);
  }
  for(auto k:{"base","alternate"})if(o.contains(k)){
   auto v=o[k].toString();
   if(!QRegularExpression("^#[0-9a-fA-F]{6}$").match(v).hasMatch())return {};
   (QString(k)=="base"?s.base:s.alternate)=QColor(v);
  }
  return s;
 }
 QColor color(const QPalette&pal,QColor surface,bool odd,bool preserveAccent=true)const{
  const auto native=pal.color(QPalette::Base);
  auto alt=pal.color(QPalette::AlternateBase);
  auto offset=[](QColor from,QColor to,int weight,QColor onto){
   auto channel=[weight](int f,int t,int o){return qBound(0,o+(t-f)*weight/100,255);};
   return QColor(channel(from.red(),to.red(),onto.red()),channel(from.green(),to.green(),onto.green()),channel(from.blue(),to.blue(),onto.blue()));
  };
  if(alt==native)alt=offset(native,pal.color(QPalette::Highlight),12,native);
  const auto b=base.isValid()?base:native,a=alternate.isValid()?alternate:alt;
  const auto start=base.isValid()&&!preserveAccent?base:surface;
  return odd?offset(b,a,strength,start):start;
 }
};
inline std::optional<RowStripes> readStripes(const QString&path){
 QFile f(path);if(!f.exists())return RowStripes{};
 if(!f.open(QIODevice::ReadOnly)||f.size()>4*1024*1024)return {};
 QJsonParseError error;auto d=QJsonDocument::fromJson(f.readAll(),&error);
 if(error.error!=QJsonParseError::NoError||!d.isObject())return {};
 auto rows=d.object().value("listRows");if(rows.isUndefined())return RowStripes{};
 if(!rows.isObject())return {};return RowStripes::parse(rows.toObject());
}
}
