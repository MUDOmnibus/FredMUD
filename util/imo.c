
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
   if (!(fl = fopen(filename, "r")))
   {
      perror("list");
      exit();
   }

   fprintf(stderr,"%d\n",sizeof(player));
   now=time(0);
   for (num = 1, pos = 0;; pos++, num++) {
      fread(&player, sizeof(player), 1, fl);
      if (feof(fl))
         exit();
      if(player.level > 800)
      printf("%4d %-15s%9d%5d\n",
        num,
        player.name,
        (now-player.last_logon)/3600,
        player.level);
   }
   fclose(fl);
}
main(int argc, char **argv)
{
   del((argc > 1) ? argv[1] : "../lib/players");
}
