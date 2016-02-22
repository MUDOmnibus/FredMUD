#include <stdio.h>
 
#define ZONEBASE 8600

#define S        7
#define MAXROOM (S*S)
#define X        4
#define RND(X) (random() % (X))

static char *dirname[]={
  "north", "east", "south", "west", "up", "down"
};
static int revdir[]={2,3,0,1,5,4};
 
int roomctr;
int exits[MAXROOM][X];
 
main()
{
   int i,j,k,d,n,x,y;
   int col,row;

   srandom(time(0));
   for(i=0;i<MAXROOM;++i)
      for(j=0;j<6;++j)
         exits[i][j]=(-1);
   for(n=0;n<MAXROOM;n++){
     row=(n/S); col=(n%S);
     if(row > 0)     exits[n][0]=n-S;
     if(col < (S-1)) exits[n][1]=n+1;
     if(row < (S-1)) exits[n][2]=n+S;
     if(col > 0)     exits[n][3]=n-1;
   }
   printall();
}
printall()
{
   int i,j,n,d,ld,x,y,f;
   char name[4];
 
   for(n=0;n<MAXROOM;++n){
     printf("#%d\n",ZONEBASE+n);
     printf("Paradise Island~\n");
     printf("You are in paradise.\n");
     printf("~\n");
     printf("%d 32770 6\n",ZONEBASE/100);
     for(j=0;j<X;++j)
       if(exits[n][j] >= 0)
         printf("D%d\n~\n~\n0 -1 %d\n",j,ZONEBASE+exits[n][j]);
     printf("S\n");
   }
}
