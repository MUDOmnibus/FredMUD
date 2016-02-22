#include <stdio.h>
#include <ctype.h>
#include "../structs.h"
#include "old.h"
void doit(char *filename)
{
   FILE *fl,*fo;
   struct old_char_file_u a;
   struct char_file_u b;
   int i, pos, num, now;
   long t,end;

   if (!(fl = fopen(filename, "r"))) {
      perror("list");
      exit();
   }
   fo=fopen("x","w+b");
   for (num = 1, pos = 0;; pos++, num++) {
      fread(&a, sizeof(a), 1, fl);
      if(feof(fl)) exit();
      b.abilities = a.abilities;
      b.points.mana = a.points.mana;
      b.points.max_mana = a.points.max_mana;
      b.points.hit = a.points.hit;
      b.points.max_hit = a.points.max_hit;
      b.points.move = a.points.move;
      b.points.max_move = a.points.max_move;
      b.points.armor = a.points.armor;
      b.points.gold = a.points.gold;
      b.points.exp = a.points.exp;
      b.points.hitroll = a.points.hitroll;
      b.points.damroll = a.points.damroll;
      b.points.kills = a.points.kills;
      b.points.deaths = a.points.deaths;
      b.points.metapts = a.metapts;
      for(i=0;i<MAX_SKILLS;i++)
        b.skills[i] = a.skills[i];
      for(i=0;i<MAX_AFFECT;i++)
        b.affected[i] = a.affected[i];
      b.level = a.level;
      b.sex = a.sex;
      b.birth = a.birth;
      b.played = a.played;
      b.weight = a.weight;
      b.height = a.height;
      b.load_room = a. load_room;
      strcpy(b.title,a.title);
      strcpy(b.description,a.description);
      strcpy(b.name,a.name);
      strcpy(b.pwd,a.pwd);
      b.spells_to_learn = a.spells_to_learn;
      b.alignment = a.alignment;
      b.last_logon = a.last_logon;
      b.act = a.act;
      b.kills = a.kills;
      b.deaths = a.deaths;
      b.points.bank = (long long) a.bank;
      b.points.bank *= (long long)1000;
      for(i=0;i<3;i++)
        b.conditions[i]=a.conditions[i];
      fwrite(&b, sizeof(b), 1, fo);
   }
   fclose(fl);
}
main(int argc, char **argv)
{
  doit("oldx");
}
