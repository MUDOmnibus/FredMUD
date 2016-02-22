#include <stdio.h>

#define RND(X) (random() % (X))

#define NJOB 11
#define NLPJ  3

struct towermob {
  char *key;
  char *job;
  char *adj[NLPJ];
} joblist[]={
 {"boy", "office boy", {" part-time", "", " old"}},
 {0, "bookkeeper", {" assistant", "", " head"}},
 {0, "secretary", {" young", " legal", " executive"}},
 {0, "accountant", {" junior", " tax", " senior"}},
 {"analyst", "systems analyst", {" temporary", " new", " senior"}},
 {0, "attorney", {" young", " patent", " tax"}},
 {0, "manager", {" office", " sales", " department"}},
 {0, "executive", {" junior", " corporate", " senior"}},
 {"vp", "vice president", {" assistant", "", " executive"}},
 {0, "president", {" former", " current", " recently appointed"}},
 {"director", "of the board", {" member", " member", " chairman"}}
};
main()
{
  int m,n,floor,level;
  int act, aff, ali;
  int vn=11000;
  char buf1[256], buf2[256], buf3[32];

  srandom(getpid());
  for(n=0;n<NJOB;n++)
    for(m=0;m<NLPJ;m++){
      vn++;
      level=1+(m+1)*n*n+RND((n+2)*(n+2));
      sprintf(buf1,"The%s %s",joblist[n].adj[m],joblist[n].job);
      sprintf(buf2,"%s is here.",buf1);
      printf("#%d\n",vn);
      rndstring(buf3);
      printf("%s %s~\n",
        joblist[n].key ? joblist[n].key : joblist[n].job,
        buf3);
      printf("%s~\n%s\n~\n~\n",buf1,buf2);
      act = rndact();
      aff = rndaff();
      ali = RND(2001)-1000;
      printf("%d %d %d S\n",act,aff,ali);
      printf("%d %d %d %dd%d+%d %dd%d+%d\n",
        level,level+1,level+1,
        1,1,(level+7)*(level+5),
        (m+1),(n+m+1)*(level/2),level);
      printf("%d %d\n",n*n+n*m*m,((level+3)*(level+13)*(level+17))/2);
      printf("8 8 %d\n",1+RND(2));
    }
}
rndaff()
{
  int m;

  m = 268435456;
  if(RND(3))
    m |= 8;
  return(m);
}
rndact()
{
  int m;

  m = 0;
  if(!RND(5))
    m += 1024;
  if(!RND(5))
    m += 16384;
  return(m);
}
rndstring(char *buf)
{
  int i;

  for(i=0;i<4;i++){
    buf[i]='a'+RND(26);
    buf[4]=0;
  }
}
