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
   int pos,num,now,t,end,i;

   t=time(0);
   if (!(fl = fopen(filename, "r"))) {
      perror("list");
      exit();
   }
   for (num = 1, pos = 0;; pos++, num++) {
      fread(&player, sizeof(player), 1, fl);
      if (feof(fl)) {
         exit();
      }
      i=player.points.max_hit;
      if(i > 999) printf("%s %d\n",player.name,i);
   }
   fclose(fl);
}
main(int argc, char **argv)
{
   del("/ge/dm/lib/players");
}
