#include "../src/row.hpp"
#include "../src/motion.hpp"
#include <QApplication>
#include <QTest>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QApplication app(argc,argv);
 QWidget host;auto*l=new QVBoxLayout(&host);
 auto*content=new QWidget;content->setFixedHeight(60);
 auto*reveal=new Reveal(content,&host,true);l->addWidget(reveal);reveal->setOpen(false,0);
 host.resize(300,220);host.show();app.processEvents();
 CHECK(reveal->isHidden());
 reveal->setOpen(true,160);QTest::qWait(45);
 CHECK(reveal->height()>0&&reveal->height()<60&&reveal->moving());CHECK(content->graphicsEffect()!=nullptr);
 reveal->setOpen(false,160);QTest::qWait(220);
 CHECK(reveal->isHidden()&&!reveal->moving()&&content->graphicsEffect()==nullptr);
 reveal->setOpen(true,0);app.processEvents();CHECK(reveal->height()==60);
 Row row("Source",false,false,1);row.addSecondary("Filters","filters",[]{});
 row.setMotionDuration(140);row.show();app.processEvents();
 row.setExpanded(true);QTest::qWait(40);CHECK(row.expanded()&&row.motionActive());
 row.setExpanded(false);QTest::qWait(200);CHECK(!row.expanded()&&row.secondary->isHidden()&&!row.motionActive());
 row.setExpanded(true);row.setMotionDuration(0);app.processEvents();CHECK(!row.motionActive()&&!row.secondary->isHidden());
 return 0;
}
