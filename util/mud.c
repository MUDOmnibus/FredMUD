#include <stdio.h>
#define ZONEBASE 1000
#define INITROOM 0
#define MAXROOM 1000
#define W 25
#define X 6  /* Number of exit direction */
#define RNDDIR ((rand() % 100)/25)
static int rev[]={2,3,0,1,5,4};
static int nbr[]={-W,1,W,-1};
int roomctr;
int exits[MAXROOM][X];
static char *street[]={
  "First",
  "Second",
  "Third",
  "Fourth",
  "Fifth",
  "Sixth",
  "Main",
  "8th,
  "9th",
  "Tenth",
  "11th",
  "12th",
  "13th"
};
static char *avenue[]={
  "Western",
  "Columbia",
  "Hilliard",
  "Indiana",
  "Jackson",
  "Kentucky",
  "Lakewood",
  "Lexington",
  "Louisiana",
  "Madison",
  "Michigan",
  "Pennsylvania",
  "Eastern"
};
main()
{
   int i,j,k,d,n,x,y;
   srand(time(0));
   for(i=0;i<MAXROOM;++i)
      for(i=0;i<6;++i)
         exits[i][j]=(-1);
   for(y=0;y<W;++y){
      for(x=0;x<W;++x){
         n=y*25+x;
         for(i=0;i<X;++i) exits[n][i]=(-1);
         if((x%2)==0){
            if(y > 0)
               exits[n][0]=n-W;
            if(y < (W-1))
               exits[n][2]=n+W;
         }
         if((y%2)==0){
            if(x > 0)
               exits[n][3]=n-1;
            if(x < (W-1))
               exits[n][1]=n+1;
         }
         if((x%2)&&(y%2)){
            d=RNDDIR;
            k=n+nbr[d];
            exits[n][d]=k;
            exits[k][revdir[d]]=n;
         }
      }
   }
   printall();
}
printall()
{
   int i,j,indoors,x,y;
   for(y=0;y<W;++y){
      for(x=0;x<W;++x){
         n=25*y+x;
         printf("#%d\n",ZONEBASE+n);
         indoors=0;
         if(((x%2)==0) && ((y%2)==0))
            printf("The Corner of %s and %s~\n",street[y/2],avenue[x/2]);
         else if((x%2)==0)
            printf("On %s Avenue~\n",avenue[x/2]);
         else if((y%2)==0)
            printf("On %s Street~\n",street[y/2]);
         else{
            printf("A Store~\n");
            indoors=1;
         }
         scribble(n,indoors);
         printf("%d %d 1\n",ZONEBASE/100,indoors ? 8 : 0);
         for(j=0;j<X;++j)
            if(exits[i][j] >= 0)
               printf("D%d\n~\n~\n0 -1 %d\n",j,ZONEBASE+INITROOM+exits[i][j]);
         printf("S\n");
      }
   }
}
scribble(n,indoors)
int n,indoors;
{
   int i,j,k,w,s;
   printf("  You are %s in Mudville.",
      indoors ? "indoors" : "outdoors");
   printf("\n~\n");
}
