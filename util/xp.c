#include <stdio.h>
#include "../structs.h"
main()
{
   FILE *fl;
   struct char_file_u player;
   int pos, num, n=0;

   if(!(fl=fopen("../lib/players","r"))){
      perror("list");
      exit();
   }
   for(num=1,pos=0;;pos++,num++){
      fread(&player, sizeof(player), 1, fl);
      if(feof(fl))
         exit();
      if(player.level < 49)
        if(player.points.exp > 100000000){
          printf("%4d  %-10s%11.0f\n",
            player.level,player.name,player.points.exp);
        }
   }
   fclose(fl);
}
