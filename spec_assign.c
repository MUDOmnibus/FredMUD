/* ************************************************************************
*  file: spec_assign.c , Special module.                  Part of DIKUMUD *
*  Usage: Procedures assigning function pointers.                         *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************* */

#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "db.h"

extern struct room_data *world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
void boot_the_shops();
void assign_the_shopkeepers();

/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

  int levelguard(struct char_data *ch, int cmd, char *arg);
  int guild(struct char_data *ch, int cmd, char *arg);
  int fitness(struct char_data *ch, int cmd, char *arg);
  int xpstore(struct char_data *ch, int cmd, char *arg);
  int rooster(struct char_data *ch, int cmd, char *arg);
  int fido(struct char_data *ch, int cmd, char *arg);
  int janitor(struct char_data *ch, int cmd, char *arg);
  int mayor(struct char_data *ch, int cmd, char *arg);
  int loader(struct char_data *ch, int cmd, char *arg);
  int dad(struct char_data *ch, int cmd, char *arg);
  int bot(struct char_data *ch, int cmd, char *arg);
  int mom(struct char_data *ch, int cmd, char *arg);
  int grandpa(struct char_data *ch, int cmd, char *arg);
  int unclefred(struct char_data *ch, int cmd, char *arg);
  int adaptor(struct char_data *ch, int cmd, char *arg);
  int defuser(struct char_data *ch, int cmd, char *arg);
  int builder(struct char_data *ch, int cmd, char *arg);
  int mudhead(struct char_data *ch, int cmd, char *arg);
  int metaphysician(struct char_data *ch, int cmd, char *arg);
  int snake(struct char_data *ch, int cmd, char *arg);
  int thief(struct char_data *ch, int cmd, char *arg);
  int magic_user(struct char_data *ch, int cmd, char *arg);
  int hunter(struct char_data *ch, int cmd, char *arg);
  int wilbur(struct char_data *ch, int cmd, char *arg);
  int sneaker(struct char_data *ch, int cmd, char *arg);
  int rusher(struct char_data *ch, int cmd, char *arg);
  int lolth(struct char_data *ch, int cmd, char *arg);
  int tree_gate(struct char_data *ch, int cmd, char *arg);
  int little_dragon(struct char_data *ch, int cmd, char *arg);
  int big_dragon(struct char_data *ch, int cmd, char *arg);
  int kickbasher(struct char_data *ch, int cmd, char *arg);
  int cloner(struct char_data *ch, int cmd, char *arg);
  int highwayman(struct char_data *ch, int cmd, char *arg);
  int shooter(struct char_data *ch, int cmd, char *arg);
  int sleeper(struct char_data *ch, int cmd, char *arg);
  int charmer(struct char_data *ch, int cmd, char *arg);
  int plasterman(struct char_data *ch, int cmd, char *arg);
  int teleporter(struct char_data *ch, int cmd, char *arg);
  int druid(struct char_data *ch, int cmd, char *arg);
  int buddy(struct char_data *ch, int cmd, char *arg);
  int vampire(struct char_data *ch, int cmd, char *arg);
  int barker(struct char_data *ch, int cmd, char *arg);
  int dealer(struct char_data *ch, int cmd, char *arg);
  int poker(struct char_data *ch, int cmd, char *arg);
  int viper(struct char_data *ch, int cmd, char *arg);
  int samhill(struct char_data *ch, int cmd, char *arg);

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
  int i;

  mob_index[real_mobile(8500)].func = defuser;
  mob_index[real_mobile(8600)].func = levelguard;
  mob_index[real_mobile(8601)].func = levelguard;

  mob_index[real_mobile(10010)].func = adaptor;
  mob_index[real_mobile(10011)].func = adaptor;
  mob_index[real_mobile(10015)].func = adaptor;
  mob_index[real_mobile(10016)].func = adaptor;
  mob_index[real_mobile(10020)].func = builder;

  mob_index[real_mobile(1313)].func = samhill;
  mob_index[real_mobile(1302)].func = rooster;
  mob_index[real_mobile(1471)].func = barker;
  mob_index[real_mobile(1472)].func = dealer;
  mob_index[real_mobile(1473)].func = poker;
  mob_index[real_mobile(1312)].func = janitor;
  mob_index[real_mobile(3061)].func = janitor;
  mob_index[real_mobile(3062)].func = fido;
  mob_index[real_mobile(3066)].func = fido;
  mob_index[real_mobile(8006)].func = fido;

  /* Suburbs */

  mob_index[real_mobile(12011)].func = shooter;
  mob_index[real_mobile(12020)].func = loader;
  mob_index[real_mobile(12039)].func = hunter;

  /* Island */

  mob_index[real_mobile(2800)].func = shooter;
  mob_index[real_mobile(2801)].func = shooter;
  mob_index[real_mobile(2809)].func = shooter;
  mob_index[real_mobile(2816)].func = snake;
  mob_index[real_mobile(2818)].func = snake;

  /* Drow */
  mob_index[real_mobile(5110)].func = lolth;

  /* Campus */

  mob_index[real_mobile(2415)].func = thief;
  mob_index[real_mobile(2428)].func = thief;
  mob_index[real_mobile(2450)].func = charmer;
  mob_index[real_mobile(2460)].func = cloner;

  mob_index[real_mobile(2101)].func = hunter;
  mob_index[real_mobile(2102)].func = hunter;

  /* Hecate, Proserpina and Hypnos */

  mob_index[real_mobile(2105)].func = kickbasher;
  mob_index[real_mobile(2113)].func = kickbasher;
  mob_index[real_mobile(2116)].func = kickbasher;
  mob_index[real_mobile(2120)].func = loader;
  mob_index[real_mobile(2122)].func = sleeper;
  mob_index[real_mobile(2124)].func = big_dragon;

  /* Scott's */

  mob_index[real_mobile(1401)].func = buddy;

  /* Midgaard */

  mob_index[real_mobile(3200)].func = mudhead;
  mob_index[real_mobile(1513)].func = plasterman;
  mob_index[real_mobile(1575)].func = teleporter;
  mob_index[real_mobile(1530)].func = xpstore;
  mob_index[real_mobile(1536)].func = loader;
  mob_index[real_mobile(1544)].func = thief;
  mob_index[real_mobile(3020)].func = guild;
  mob_index[real_mobile(3021)].func = fitness;
  mob_index[real_mobile(3143)].func = mayor;
  mob_index[real_mobile(3150)].func = mom;
  mob_index[real_mobile(3160)].func = dad;
  mob_index[real_mobile(1304)].func = bot;
  mob_index[real_mobile(1308)].func = unclefred;
  mob_index[real_mobile(1309)].func = shooter;
  mob_index[real_mobile(1314)].func = wilbur;
  mob_index[real_mobile(1315)].func = kickbasher;
  mob_index[real_mobile(1316)].func = kickbasher;
  mob_index[real_mobile(3151)].func = grandpa;
  mob_index[real_mobile(1310)].func = rusher;
  mob_index[real_mobile(3147)].func = thief;
  mob_index[real_mobile(1320)].func = hunter;
  mob_index[real_mobile(1564)].func = sneaker;

  /* Dragons */

  mob_index[real_mobile(1305)].func = big_dragon;
  mob_index[real_mobile(6112)].func = little_dragon;
  mob_index[real_mobile(7040)].func = little_dragon;
  mob_index[real_mobile(15000)].func = little_dragon;
  mob_index[real_mobile(15017)].func = little_dragon;
  mob_index[real_mobile(18610)].func = little_dragon;

  /* Trees */

  mob_index[real_mobile(15101)].func = tree_gate;
  mob_index[real_mobile(15102)].func = tree_gate;
  mob_index[real_mobile(15103)].func = tree_gate;
  mob_index[real_mobile(15104)].func = tree_gate;
  mob_index[real_mobile(15105)].func = tree_gate;
  mob_index[real_mobile(15106)].func = tree_gate;

  /* Abyss */

  mob_index[real_mobile(15615)].func = charmer;

  /* Heal's */

  mob_index[real_mobile(8118)].func = kickbasher;
  mob_index[real_mobile(8216)].func = kickbasher;

  /* Arachnos */

  /* Tomb */

  /* MORIA */

  mob_index[real_mobile(4000)].func = snake;
  mob_index[real_mobile(4001)].func = snake;
  mob_index[real_mobile(4053)].func = snake;
  mob_index[real_mobile(4055)].func = rusher;
  mob_index[real_mobile(4103)].func = thief;
  mob_index[real_mobile(4102)].func = snake;

  /* SEWERS */

  mob_index[real_mobile(7006)].func = snake;

  /* FOREST */

  mob_index[real_mobile(6113)].func = snake;
  mob_index[real_mobile(6114)].func = snake;

  /* Queen spider */

  mob_index[real_mobile(6114)].func = snake;

  /* Cube */


  /* Trail */

  mob_index[real_mobile(16500)].func = highwayman;
  mob_index[real_mobile(16520)].func = highwayman;
  mob_index[real_mobile(16504)].func = loader;
  mob_index[real_mobile(16507)].func = loader;
  mob_index[real_mobile(16510)].func = loader;
  mob_index[real_mobile(16601)].func = snake;
  mob_index[real_mobile(16605)].func = druid;
  mob_index[real_mobile(16606)].func = druid;
  mob_index[real_mobile(16608)].func = druid;
  mob_index[real_mobile(16610)].func = teleporter;
  mob_index[real_mobile(16612)].func = snake;
  mob_index[real_mobile(16613)].func = sleeper;
  mob_index[real_mobile(16621)].func = viper;

  mob_index[real_mobile(17104)].func = snake;
  mob_index[real_mobile(17210)].func = vampire;
  mob_index[real_mobile(17211)].func = vampire;

  mob_index[real_mobile(17409)].func = thief;

  mob_index[real_mobile(1311)].func = metaphysician;

  boot_the_shops();
  assign_the_shopkeepers();
}

/* assign special procedures to objects */
void assign_objects(void)
{
  int drinkmachine(struct char_data *ch, int cmd, char *arg);

  obj_index[real_object(1000)].func = drinkmachine;
  obj_index[real_object(1001)].func = drinkmachine;
}

/* assign special procedures to rooms */
void assign_rooms(void)
{
  int dump(struct char_data *ch, int cmd, char *arg);
  int pet_shops(struct char_data *ch, int cmd, char *arg);
  int hospital(struct char_data *ch, int cmd, char *arg);
  int level_gate(struct char_data *ch, int cmd, char *arg);
  int size_gate(struct char_data *ch, int cmd, char *arg);
  int portal(struct char_data *ch, int cmd, char *arg);
  int bank(struct char_data *ch, int cmd, char *arg);

  world[real_room(3002)].funct = bank;
  world[real_room(1453)].funct = level_gate;
  world[real_room(17200)].funct = level_gate;
  world[real_room(2346)].funct = level_gate;
  world[real_room(3300)].funct = level_gate;
  world[real_room(5298)].funct = level_gate;
  world[real_room(6001)].funct = level_gate;
  world[real_room(2431)].funct = size_gate;

  world[real_room(3031)].funct = pet_shops;
  world[real_room(3060)].funct = hospital;
  world[real_room(2158)].funct = portal;

  world[real_room(3030)].funct = dump;
  world[real_room(1514)].funct = dump;
}

static char *specs[]={
  "NONE",
  "thief",
  "kickbasher",
  "hunter",
  "shooter",
  "sleeper",
  "charmer",
  "bot",
  "magic user",
  "plasterman",
  "little dragon",
  "big dragon",
  "buddy",
  "snake",
  "loader",
  "druid",
  "clone",
  "vampire",
  "rooster",
  "sneaker",
  "sam hill",
  "levelguard",
  "wilbur",
  0
};
void do_reassign(struct char_data *ch, char *argument, int cmd)
{
  int num,key;
  char buf1[512],buf2[64],buf3[64];

  half_chop(argument,buf1,buf2);
  if(!*buf1){
    for(num=0;specs[num];num++){
      sprintf(buf2,"%2d: %s\n\r",num,specs[num]);
      strcat(buf1,buf2);
    }
    send_to_char(buf1,ch);
    return;
  }
  one_argument(buf2,buf3);
  if(!*buf3) return;
  num=atoi(buf1);
  if(num <= 0)
    return;
  key=atoi(buf3);
  if((key < 0) || (key > 23))
    return;
  switch(key){
    case  0: mob_index[real_mobile(num)].func = 0; break;
    case  1: mob_index[real_mobile(num)].func = thief; break;
    case  2: mob_index[real_mobile(num)].func = kickbasher; break;
    case  3: mob_index[real_mobile(num)].func = hunter; break;
    case  4: mob_index[real_mobile(num)].func = shooter; break;
    case  5: mob_index[real_mobile(num)].func = sleeper; break;
    case  6: mob_index[real_mobile(num)].func = charmer; break;
    case  7: mob_index[real_mobile(num)].func = bot; break;
    case  8: mob_index[real_mobile(num)].func = magic_user; break;
    case  9: mob_index[real_mobile(num)].func = plasterman; break;
    case 10: mob_index[real_mobile(num)].func = little_dragon; break;
    case 11: mob_index[real_mobile(num)].func = big_dragon; break;
    case 12: mob_index[real_mobile(num)].func = buddy; break;
    case 13: mob_index[real_mobile(num)].func = snake; break;
    case 14: mob_index[real_mobile(num)].func = loader; break;
    case 15: mob_index[real_mobile(num)].func = druid; break;
    case 16: mob_index[real_mobile(num)].func = cloner; break;
    case 17: mob_index[real_mobile(num)].func = vampire; break;
    case 18: mob_index[real_mobile(num)].func = rooster; break;
    case 19: mob_index[real_mobile(num)].func = sneaker; break;
    case 20: mob_index[real_mobile(num)].func = samhill; break;
    case 21: mob_index[real_mobile(num)].func = levelguard; break;
    case 22: mob_index[real_mobile(num)].func = wilbur; break;
  }
  sprintf(buf1,"Set %d to %s.\n\r",num,specs[key]);
  send_to_char(buf1,ch);
}

