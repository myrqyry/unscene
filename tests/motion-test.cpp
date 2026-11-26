#include "../src/row.hpp"
#include "../src/motion.hpp"
#include "../src/reorder-motion.hpp"
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
 QWidget list;auto*stack=new ReorderLayout(&list);stack->setContentsMargins(0,0,0,0);stack->setSpacing(0);
 auto*a=new QWidget;auto*b=new QWidget;auto*c=new QWidget;
 for(auto*w:{a,b,c}){w->setFixedHeight(40);stack->addWidget(w);}stack->addStretch();
 list.resize(200,200);list.show();app.processEvents();
 CHECK(a->y()==0&&b->y()==40&&c->y()==80);
 auto origins=stack->positions();stack->removeWidget(a);stack->insertWidget(2,a);stack->activate();stack->animateFrom(origins,150);
 CHECK(stack->moving()&&a->y()==0&&b->y()==40&&a->property("unsceneReordering").toBool());
 QTest::qWait(45);CHECK(a->y()>0&&a->y()<80);
 stack->invalidate();stack->activate();CHECK(a->y()<80);
 QTest::qWait(180);CHECK(!stack->moving()&&a->y()==80&&b->y()==0&&c->y()==40&&!a->property("unsceneReordering").toBool());
 origins=stack->positions();stack->removeWidget(a);stack->insertWidget(0,a);stack->activate();stack->animateFrom(origins,150);
 QTest::qWait(35);CHECK(stack->moving());stack->finish();CHECK(!stack->moving()&&a->y()==0);
 origins=stack->positions();stack->removeWidget(a);stack->insertWidget(2,a);stack->activate();stack->animateFrom(origins,0);CHECK(!stack->moving()&&a->y()==80);
 return 0;
}
