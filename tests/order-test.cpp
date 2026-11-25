#include "../src/order.hpp"
#include <QCoreApplication>
#define CHECK(x) do{if(!(x)){qCritical("failed: %s",#x);return 1;}}while(false)
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);
 QStringList rows{"a","b","c","d"};
 CHECK(moveRelative(rows,"a","c",false)==QStringList({"b","c","a","d"}));
 CHECK(moveRelative(rows,"d","b",true)==QStringList({"a","d","b","c"}));
 CHECK(moveRelative(rows,"a","b",true)==rows);
 CHECK(!moveRelative(rows,"a","a",true));
 CHECK(!moveRelative(rows,"a","missing",true));
 CHECK(!moveRelative(QStringList({"a","a","b"}),"a","b",true));
 CHECK(sameOrderMembers(rows,QStringList({"d","b","a","c"})));
 CHECK(!sameOrderMembers(rows,QStringList({"a","b","b","d"})));
 CHECK(!sameOrderMembers(rows,QStringList({"a","b","c","e"})));
 OrderUndo u{"collection","parent",rows,QStringList({"b","a","c","d"})};
 auto decoded=OrderUndo::decode(u.encode());CHECK(decoded&&decoded->encode()==u.encode());
 CHECK(!OrderUndo::decode("{}"));
 u.next={"a","a","c","d"};CHECK(!OrderUndo::decode(u.encode()));
 return 0;
}
