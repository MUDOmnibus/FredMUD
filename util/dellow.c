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
   int pos, num, now;
   long t,end;

   t=time(0);
   if (!(fl = fopen(filename, "r"))) {
      perror("list");
      exit();
   }
   now=time(0);
   fo=fopen("x","w+b");
   for (num = 1, pos = 0;; pos++, num++) {
      fread(&player, sizeof(player), 1, fl);
      if(feof(fl)) exit();
      if(player.level < 2) continue;
      fwrite(&player, sizeof(player), 1, fo);
   }
   fclose(fl);
}
main(int argc, char **argv)
{
   del("../lib/players");
}
