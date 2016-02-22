#include <stdio.h>
 
#define ZONEBASE 15000
#define INITROOM 0
#define MAXROOM 90
#define W 25
#define X 6  /* Number of exit direction */
#define RND(X) (rand() % (X))
 
static int rev[]={2,3,0,1,5,4};
static int nbr[]={-W,1,W,-1};
 
int roomctr=0;
int exits[MAXROOM][X];
int mobctr=0;

main()
{
   int i,j,k,d,n,x,y;
 
   freopen("dr.wld","w",stdout);
   srand(time(0));
   for(i=0;i<MAXROOM;++i)
      for(j=0;j<X;++j)
         exits[i][j]=(-1);
   makeexits();
   printall();
}
makeexits()
{
   int i,n;

   for(i=1;i<MAXROOM;++i){
      do n=RND(4); while(exits[i][n] >= 0);
      exits[i][n]=i+1;
      exits[i+1][rev[n]]=i;
   }
   for(i=1;i<MAXROOM;++i){
      do n=RND(4); while(exits[i][n] >= 0);
      exits[i][n]=i;
   }
}
printall()
{
   int i,j,k,n,x,y;
 
   for(n=0;n<MAXROOM;++n){
         for(i=k=0;i<X;++i)
            if(exits[n][i]!=(-1))
               ++k;
         printf("#%d\n",ZONEBASE+n);
         printf("A Long Dark Tunnel~\n");
         printf(" %s\n~\n","You are in a long dark tunnel.");
         printf("%d %d 5\n",ZONEBASE/100,4105);
         for(j=0;j<X;++j)
            if(exits[n][j] >= 0)
               printf("D%d\n~\n~\n0 -1 %d\n",j,ZONEBASE+exits[n][j]);
         printf("S\n");
   }
}
