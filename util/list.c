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

   now=time(0);
   for (num = 1, pos = 0;; pos++, num++) {
      fread(&player, sizeof(player), 1, fl);
      if (feof(fl))
         exit();
      printf("%3d:%-15s%4d|%7d|%7d|%7d|%4d%4d%4d%4d%4d%5d%5x\n",
        num,player.name,
        player.level,
        player.points.max_hit,
        player.points.max_mana,
        player.points.max_move,
        player.abilities.str,
        player.abilities.intel,
        player.abilities.wis,
        player.abilities.dex,
        player.abilities.con,
        (now-player.last_logon)/3600,
        player.act);
   }
   fclose(fl);
}
main(int argc, char **argv)
{
   del((argc > 1) ? argv[1] : "../lib/players");
}
