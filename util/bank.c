#include <stdio.h>
#include <ctype.h>
#include "../structs.h"
#define TOLOWER(c)  (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
int str_cmp(char *str1, char *str2)
{
   for (; *str1 || *str2; str1++, str2++)
      if (TOLOWER(*str1) != TOLOWER(*str2))
         return(1);
   return(0);
}
void del(char *filename)
{
   FILE *fl,*fo;
   struct char_file_u player;
   int pos,num,now,t,end,sum;

   t=time(0);
   if (!(fl = fopen(filename, "r"))) {
      perror("bank");
      exit();
   }
   sum=0;
   for (num = 1, pos = 0;; pos++, num++) {
      fread(&player, sizeof(player), 1, fl);
      if (feof(fl)) {
         exit();
      }
      if(player.bank > 1000000)
        printf("%4d: %-20s %d\n",num,player.name,player.bank);
   }
   fclose(fl);
}
main(int argc, char **argv)
{
   del("../lib/players");
}
