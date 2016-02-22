#include <stdio.h>
 
#define ZONEBASE 10000
#define INITROOM 0
#define S 8
#define MAXROOM (S*S*S)
#define X 6  /* Number of exit direction */
#define RNDDIR (rand() % 6)
 
static int rev[]={2,3,0,1,5,4};
static int nbr[]={ S, 1, -S, -1, S*S, -S*S};
 
int roomctr;
int exits[MAXROOM][X];
 
main()
{
   int i,j,k,d,n,x,y;
 
   srand(time(0));
   for(i=0;i<MAXROOM;++i)
      for(j=0;j<6;++j)
         exits[i][j]=(-1);
   for(i=0;i<MAXROOM;++i){
      for(j=0;j<6;j++){
        k=i+nbr[j];
        if(((i%8)==7)&&(j==1)) continue;
        if(((i%8)==0)&&(j==3)) continue;
        if(((i%64) >= 56)&&(j==0)) continue;
        if(((i%64) < 8)&&(j==2)) continue;
        if((i >= 448)&&(j==4)) continue;
        if((i < 64)&&(j==5)) continue;
        if ((k < 0) || (k >= MAXROOM)) continue;
        exits[i][j]=k;
      }
   }
   printall();
}
printall()
{
   int i,j,n,indoors,x,y;
   char name[4];
 
   name[3]=0;
   for(n=0;n<MAXROOM;++n){
         printf("#%d\n",ZONEBASE+n);
         printf("The Cube~\n");
         printf("This is Cell %03o.",n);
         printf("\n~\n");
         printf("%d 12298 4\n",ZONEBASE/100);
         for(j=0;j<X;++j)
            if(exits[n][j] >= 0)
               printf("D%d\n~\n~\n0 -1 %d\n",j,ZONEBASE+exits[n][j]);
         printf("S\n");
   }
}
