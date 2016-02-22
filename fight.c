/* ************************************************************************
*  File: fight.c , Combat module.                         Part of DIKUMUD *
*  Usage: Combat system and messages.                                     *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************* */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"

#define NTAUNT 14

static char *taunt[]={
  "HOORAY!! I just killed %s!",
  "HA HA %s! You stink!",
  "I am happy to report that %s is dead!",
  "Long live %s! HA, HA, HA!!",
  "HA! Looks like %s just kicked the bucket!",
  "Bid a fond farewell to poor old dead %s!",
  "My sympathies to the %s family!",
  "Our friend %s has bitten the proverbial bullet!",
  "Looks like a quick trip to the Temple for %s!",
  "I sure hope %s likes reading that menu!",
  "My old pal %s just bought the farm!",
  "Funeral services for %s will be held at the city dump!",
  "Farewell to dear old %s, who won't be missed!",
  "Die %s, die!"
};

/* Structures */

struct char_data *combat_list = 0;     /* head of l-list of fighting chars */
struct char_data *combat_next_dude = 0; /* Next dude global trick           */


/* External structures */

extern struct index_data *mob_index;
extern struct weapon_spell_list wsplist[];
extern int pcdeaths,mobdeaths;
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data  *object_list;

/* External procedures */

char *fread_string(FILE *f1);
void stop_follower(struct char_data *ch);
void do_flee(struct char_data *ch, char *argument, int cmd);
void hit(struct char_data *ch, struct char_data *victim, int type);
void rescuesub(struct char_data *ch, struct char_data *victim);

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit",   "hits"},             /* TYPE_HIT      */
  {"pound", "pounds"},           /* TYPE_BLUDGEON */
  {"pierce","pierces"},         /* TYPE_PIERCE   */
  {"slash", "slashes"},          /* TYPE_SLASH    */
  {"whip",  "whips"},             /* TYPE_WHIP     */
  {"blast", "blasts"},
  {"splat", "splats"},
  {"spank", "spanks"},           /* TYPE_STING    */
  {"crush", "crushes"},          /* TYPE_CRUSH    */
  {"burn" , "burns"}
};


/* The Fight related routines */

void weapon_effects(struct char_data *ch, struct char_data *vict,
 struct obj_data *weapon);

void appear(struct char_data *ch)
{
  act("$n slowly fade into existence.", FALSE, ch,0,0,TO_ROOM);
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);
  REMOVE_BIT(ch->specials.affected_by, AFF_INVISIBLE);
}

void load_messages(void)
{
 FILE *f1;
 int i,type;
 struct message_type *messages;
 char chk[100];

 if (!(f1 = fopen(MESS_FILE, "r"))){
  perror("read messages");
  exit(0);
 }

 for (i = 0; i < MAX_MESSAGES; i++)
 { 
  fight_messages[i].a_type = 0;
  fight_messages[i].number_of_attacks=0;
  fight_messages[i].msg = 0;
 }

 fscanf(f1, " %s \n", chk);

 while(*chk == 'M') {
  fscanf(f1," %d\n", &type);
  for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type!=type) &&
   (fight_messages[i].a_type); i++);
  if(i>=MAX_MESSAGES){
   log("Too many combat messages.");
   exit(0);
  }
  CREATE(messages,struct message_type,1);
  fight_messages[i].number_of_attacks++;
  fight_messages[i].a_type=type;
  messages->next=fight_messages[i].msg;
  fight_messages[i].msg=messages;
  messages->die_msg.attacker_msg      = fread_string(f1);
  messages->die_msg.victim_msg        = fread_string(f1);
  messages->die_msg.room_msg          = fread_string(f1);
  messages->miss_msg.attacker_msg     = fread_string(f1);
  messages->miss_msg.victim_msg       = fread_string(f1);
  messages->miss_msg.room_msg         = fread_string(f1);
  messages->hit_msg.attacker_msg      = fread_string(f1);
  messages->hit_msg.victim_msg        = fread_string(f1);
  messages->hit_msg.room_msg          = fread_string(f1);
  messages->god_msg.attacker_msg      = fread_string(f1);
  messages->god_msg.victim_msg        = fread_string(f1);
  messages->god_msg.room_msg          = fread_string(f1);
  fscanf(f1, " %s \n", chk);
 }
 fclose(f1);
}

void update_pos( struct char_data *victim )
{

 if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POSITION_STUNNED)) return;
 else if (GET_HIT(victim) > 0 ) GET_POS(victim) = POSITION_STANDING;
 else if (GET_HIT(victim) <= -11) GET_POS(victim) = POSITION_DEAD;
 else if (GET_HIT(victim) <= -6) GET_POS(victim) = POSITION_MORTALLYW;
 else if (GET_HIT(victim) <= -3) GET_POS(victim) = POSITION_INCAP;
 else GET_POS(victim) = POSITION_STUNNED;

}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data *ch, struct char_data *vict)
{
 assert(!ch->specials.fighting);

 ch->next_fighting = combat_list;
 combat_list = ch;

 if(IS_AFFECTED(ch,AFF_SLEEP))
  affect_from_char(ch,SPELL_SLEEP);
 ch->specials.fighting = vict;
 GET_POS(ch) = POSITION_FIGHTING;
}

/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
 struct char_data *tmp;

 if(!(ch->specials.fighting)) return;
 if (ch == combat_next_dude)
  combat_next_dude = ch->next_fighting;
 if (combat_list == ch)
    combat_list = ch->next_fighting;
 else {
  for (tmp = combat_list; tmp && (tmp->next_fighting != ch); 
   tmp = tmp->next_fighting);
  if (!tmp) {
   log("Char fighting not found Error (fight.c, stop_fighting)");
   abort();
  }
  tmp->next_fighting = ch->next_fighting;
 }
 ch->next_fighting = 0;
 ch->specials.fighting = 0;
 GET_POS(ch) = POSITION_STANDING;
 update_pos(ch);
}

#define MAX_NPC_CORPSE_TIME 3
#define MAX_PC_CORPSE_TIME 48
#define MAX_POOF_TIME 8

void make_corpse(struct char_data *ch)
{
 struct obj_data *corpse, *o, *xo;
 struct obj_data *money; 
 char buf[MAX_STRING_LENGTH];
 int i;
#ifdef NEEDS_STRDUP
 char *strdup(char *source);
#endif
 struct obj_data *create_money( int amount );
 struct extra_descr_data *new_descr;

 if(!IS_NPC(ch)){
  if(GET_LEVEL(ch) >= (IMO>>4)){
   i=number(0,MAX_WEAR-1);
   CREATE(corpse, struct obj_data, 1);
   clear_object(corpse);
   corpse->obj_flags.wear_flags=1;
   corpse->obj_flags.weight=1000;
   corpse->name = strdup("gravestone");
   corpse->in_room = NOWHERE;
   corpse->item_number = NOWHERE;
   sprintf(buf,"A Gravestone for %s",GET_NAME(ch));
   corpse->short_description = strdup(buf);
   sprintf(buf,"%s's gravestone is here.",GET_NAME(ch));
   corpse->description = strdup(buf);
   CREATE(new_descr, struct extra_descr_data, 1);
   new_descr->keyword = strdup("gravestone");
   sprintf(buf,"It reads: Here lies %s, poor soul.\n\r",GET_NAME(ch));
   new_descr->description = strdup(buf);
   new_descr->next = corpse->ex_description;
   corpse->ex_description = new_descr;
   corpse->next = object_list;
   object_list = corpse;
   obj_to_room(corpse, ch->in_room);
  }
  return;
 }
/*
   Old corpse system here.
*/
 CREATE(corpse, struct obj_data, 1);
 clear_object(corpse);
 corpse->item_number = NOWHERE;
 corpse->in_room = NOWHERE;
 sprintf(buf,"corpse %s",GET_NAME(ch));
 corpse->name = strdup(buf);
 sprintf(buf, "Corpse of %s is lying here.", 
   (IS_NPC(ch) ? ch->player.short_descr : GET_NAME(ch)));
 corpse->description = strdup(buf);
 sprintf(buf, "Corpse of %s",
   (IS_NPC(ch) ? ch->player.short_descr : GET_NAME(ch)));
 corpse->short_description = strdup(buf);
 corpse->contains = ch->carrying;
 if((GET_GOLD(ch)>0)&&(IS_NPC(ch)||ch->desc)) {
  money = create_money(GET_GOLD(ch));
  GET_GOLD(ch)=0;
  obj_to_obj(money,corpse);
 }
 corpse->obj_flags.type_flag = ITEM_CONTAINER;
 corpse->obj_flags.wear_flags = ITEM_TAKE;
 corpse->obj_flags.extra_flags = ITEM_NO_PUT_IN;
 corpse->obj_flags.value[0] = 0; /* You can't store stuff in a corpse */
 corpse->obj_flags.value[2] = mob_index[ch->nr].virtual;
 corpse->obj_flags.value[3] = 1; /* corpse identifyer */
 corpse->obj_flags.weight = GET_WEIGHT(ch)+IS_CARRYING_W(ch);
 if (IS_NPC(ch))
  corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
 else
  corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;

 for (i=0; i<MAX_WEAR; i++)
  if (xo=ch->equipment[i]){
   obj_to_obj(unequip_char(ch, i), corpse);
   if(IS_SET(xo->obj_flags.extra_flags,ITEM_POOF)){
     SET_BIT(xo->obj_flags.extra_flags,ITEM_POOFSOON);
     if(GET_ITEM_TYPE(xo) == ITEM_WEAPON)
       xo->obj_flags.timer=24*number(1,7);
     else if(GET_ITEM_TYPE(xo) == ITEM_BOMB) {
       xo->obj_flags.timer = number(6,24);
     } else {
       xo->obj_flags.timer=MAX_POOF_TIME;
     }
   }
  }
 ch->carrying = 0;
 IS_CARRYING_N(ch) = 0;
 IS_CARRYING_W(ch) = 0;
 corpse->next = object_list;
 object_list = corpse;
 for(o = corpse->contains; o; o->in_obj = corpse, o = o->next_content);
 object_list_new_owner(corpse, 0);
 obj_to_room(corpse, ch->in_room);
 for(o=corpse->contains;o;o=xo){
  xo=o->next_content;
  if(GET_ITEM_TYPE(o)==ITEM_BOMB){
   obj_from_obj(o);
   obj_to_room(o,ch->in_room);
   act("An explosive device rolls out of the corpse.",FALSE,ch,0,0,TO_ROOM);
  }
 }
}


/* When ch kills victim */
void change_alignment(struct char_data *ch, struct char_data *victim)
{
 int al;
 
 if(IS_AFFECTED(ch,AFF_HOLDALIGN))
  return;
 al=(7*GET_ALIGNMENT(ch)-GET_ALIGNMENT(victim))/8;
 if(al < -1000) al = -1000;
 if(al >  1000) al =  1000;
 GET_ALIGNMENT(ch)=al;
}

void death_cry(struct char_data *ch)
{
 struct char_data *victim;
 int door, was_in;

 act("Your blood freezes as you hear $ns death cry.", FALSE, ch,0,0,TO_ROOM);
 was_in = ch->in_room;

 for (door = 0; door <= 5; door++) {
  if (CAN_GO(ch, door)) {
   ch->in_room = world[was_in].dir_option[door]->to_room;
   act("Your blood freezes as you hear someones death cry.",
    FALSE,ch,0,0,TO_ROOM);
   ch->in_room = was_in;
  }
 }
}

void raw_kill(struct char_data *ch)
{
 int i,j,n;
 struct char_data *tmp;

 if (ch->specials.fighting)
  stop_fighting(ch);
 death_cry(ch);
 if(IS_NPC(ch))
   mobdeaths++;
 else
   pcdeaths++;
 if(IS_SET(ch->specials.act,ACT_TRIBBLE)){
   n = ch->nr;
   j=number(0,10)>>2;
   for(i=0;i<j;i++){
     tmp = read_mobile(n,0);
     if(tmp)
       char_to_room(tmp,ch->in_room);
   }
 } else {
   make_corpse(ch);
 }
 if(IS_NPC(ch))
   extract_char(ch);
 else{
   ch->points.gold>>=1;
   do_rent(ch,0,0);
 }
}
void die(struct char_data *ch)
{
 int pen;
 struct affected_type *hjp;

 if(!IS_NPC(ch)){
   gain_exp(ch, -(int)(GET_EXP(ch)>>1));
   GET_META(ch) /= 2;
   pen = 1 + ch->points.max_hit/50;
   if (pen < 0)
     return;
   ch->points.max_hit -= pen;
   pen = 1 + ch->points.max_mana/50;
   if (pen < 0)
     return;
   ch->points.max_mana -= pen;
   pen = 1 + ch->points.max_move/50;
   if (pen < 0)
     return;
   ch->points.max_move -= pen;
   for(hjp = ch->affected; hjp; hjp = hjp->next)
     affect_remove(ch, hjp );
 }
 raw_kill(ch);
}
void group_gain(struct char_data *ch, struct char_data *victim, int levsum)
{
 char buf[256];
 int share, shareunit, t;
 struct char_data *k;
 struct follow_type *f;

 if(!levsum)
  return;
 shareunit = GET_EXP(victim)/levsum;
 if (!(k=ch->master))
  k=ch;
 if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room)) {
  if(levsum > 0){
   t = GET_LEVEL(k);
   share = shareunit * t;
  } else
   share = 0;
  sprintf(buf, "You receive your share of experience, which is worth %d.",
   share);
  act(buf, FALSE, k, 0, 0, TO_CHAR);
  gain_exp(k,share);
  change_alignment(k,victim);
 }
 for (f=k->followers;f;f=f->next) {
  if(IS_AFFECTED(f->follower,AFF_GROUP)&&(f->follower->in_room==ch->in_room)){
   if(levsum > 0){
    t = GET_LEVEL(f->follower);
    share = shareunit * t;
   } else
    share = 0;
   sprintf(buf,"You receive your share of experience, which is worth %d.",
    share);
   act(buf,FALSE,f->follower,0,0,TO_CHAR);
   gain_exp(f->follower,share);
   change_alignment(f->follower, victim);
  }
 }
}

char *rep_string(char *str, char *weapon, int dam)
{
 static char buf[256];
 static char tmp[32];
 char *cp;

 cp = buf;

 for (; *str; str++) {
  if (*str == '#') {
   switch(*(++str)) {
    case 'W' : 
     for (; *weapon; *(cp++) = *(weapon++));
     break;
    default :
     *(cp++) = '#';
     break;
   }
  } else {
   *(cp++) = *str;
  }
  *cp = 0;
 } /* For */
 sprintf(tmp," (%d Dam).",dam);
 strcat(buf,tmp);
 return(buf);
}

void dam_message(int dam, struct char_data *ch, struct char_data *victim,
                 int w_type)
{
 struct obj_data *wield;
 char *buf;
 int t;

 static struct dam_weapon_type {
  char *to_room;
  char *to_char;
  char *to_victim;
 } dam_weapons[] = {

  {"$n misses $N with $s #W",
   "You miss $N with your #W",
   "$n misses you with $s #W" },   /* 0 */

   {"$n tickles $N with $s #W",
    "You tickle $N as you #W $M",
    "$n tickles you as $e #W you" },   /* 1 */

   {"$n barely #W $N",
    "You barely #W $N",
    "$n barely #W you"}, /* 2 */

  {"$n #W $N",
    "You #W $N",
    "$n #W you"},  /* 3 */

  {"$n #W $N hard",
   "You #W $N hard",
    "$n #W you hard"},

  {"$n #W $N very hard",
   "You #W $N very hard",
   "$n #W you very hard"},  /* 5 */

  {"$n #W $N extremely hard",
   "You #W $N extremely hard",
   "$n #W you extremely hard"},

  {"$n MASSACRES $N with $s #W",
   "You MASSACRE $N with your #W",
   "$n MASSACRES you with $s #W"}, /* 7 */

  {"$n PULVERIZES $N with $s #W",
   "You PULVERIZE $N with your #W",
   "$n PULVERIZES you with $s #W"},

  {"$n ANNIHILATES $N with $s #W",
   "You ANNIHILATE $N with your #W",
   "$n ANNIHILATES you with $s #W"},

  {"$n OBLITERATES $N with $s #W",
   "You OBLITERATE $N with your #W",
   "$n OBLITERATES you with $s #W"},  /* 10 */

  {"$n DISINTEGRATES $N with $s #W",
   "You DISINTEGRATE $N with your #W",
   "$n DISINTEGRATES you with $s #W"},

  {"$n BIFURCATES $N with $s #W",
   "You BIFURCATE $N with your #W",
   "$n BIFURCATES you with $s #W"},   /* 12 */

  {"$n VARIEGATES $N with $s #W",
   "You VARIEGATE $N with your #W",
   "$n VARIEGATES you with $s #W"},

  {"$n LAMINATES $N with $s #W",
   "You LAMINATE $N with your #W",
   "$n LAMINATES you with $s #W"},    /* 14 */

  {"$n MASTICATES $N with $s #W",
   "You MASTICATE $N with your #W",
   "$n MASTICATES you with $s #W"},

  {"$n REGURGITATES $N with $s #W",
   "You REGURGITATE $N with your #W",
   "$n REGURGITATES you with $s #W"}
 };
 w_type -= TYPE_HIT;   /* Change to base of table with text */
 wield = ch->equipment[WIELD];
 if (dam == 0) {
  t=0;
 } else if (dam <= 2) {
  t=1;
 } else if (dam <= 4) {
  t=2;
 } else if (dam <= 8) {
  t=3;
 } else if (dam <= 12) {
  t=4;
 } else if (dam <= 18) {
  t=5;
 } else if (dam <= 26) {
  t=6;
 } else if (dam <= 36) {
  t=7;
 } else if (dam <= 48) {
  t=8;
 } else if (dam <= 64) {
  t=9;
 } else if (dam <= 99) {
  t=10;
 } else if (dam <= 399) {
  t=11;
 } else if (dam <= 1599) {
  t=12;
 } else if (dam <= 4999) {
  t=13;
 } else if (dam <= 19999) {
  t=14;
 } else if (dam <= 99999) {
  t=15;
 } else {
  t=16;
 }
 buf=rep_string(dam_weapons[t].to_room,attack_hit_text[w_type].singular,dam);
 act(buf,FALSE,ch,wield,victim,TO_NOTVICT);
 buf=rep_string(dam_weapons[t].to_char,attack_hit_text[w_type].singular,dam);
 act(buf,FALSE,ch,wield,victim,TO_CHAR);
 buf=rep_string(dam_weapons[t].to_victim,attack_hit_text[w_type].singular,dam);
 act(buf,FALSE,ch,wield,victim,TO_VICT);
}

int getlevelsum(struct char_data *ch)
{
  struct char_data *ldr;
  struct follow_type *f;
  int room,t,levsum=0;

  room = ch->in_room;
  ldr = (ch->master) ? ch->master : ch;
  if(IS_AFFECTED(ldr,AFF_GROUP) && (ldr->in_room == room)){
    t=GET_LEVEL(ldr);
    levsum=t;
  }
  for(f=ldr->followers; f; f=f->next)
    if(IS_AFFECTED(f->follower,AFF_GROUP) && (f->follower->in_room == room)){
      t=GET_LEVEL(f->follower);
      levsum+=t;
    }
  return(levsum);
}
void damage(struct char_data *ch, struct char_data *victim,
            int dam, int attacktype)
{
 struct char_data *tmp;
 struct affected_type *aff;
 char buf[MAX_STRING_LENGTH];
 struct message_type *messages;
 int i,j,k,nr,max_hit,exp,levdam;
 extern int nokillflag;
 int hit_limit(struct char_data *ch);
 bool lootflag;
 
 if(!victim)
   return;
 if(IS_NPC(ch)==IS_NPC(victim)){
   if(!IS_NPC(ch)){
     if(nokillflag && !IS_SET(world[ch->in_room].room_flags,PK_ROOM)) return;
   } else {
     SET_BIT(ch->specials.act,ACT_SMART);
   }
 }
 if(IS_SET(world[ch->in_room].room_flags,LAWFUL))
   return;
 if(GET_POS(victim) <= POSITION_DEAD){
   stop_fighting(ch);
   return;
 }
 if (ch->in_room != victim->in_room) {
  if(ch->specials.fighting == victim)
   stop_fighting(ch);
  return;
 }
 if(IS_AFFECTED(ch, AFF_CHARM) && (ch->master==victim)){
    send_to_char("Hurt your master?\n\r",ch);
    return;
 }
 if((GET_LEVEL(victim)>=IMO) && !IS_NPC(victim))
  dam=0;
 if (victim != ch) {
  if (GET_POS(victim) > POSITION_STUNNED) {
   if (!(victim->specials.fighting))
    set_fighting(victim, ch);
   GET_POS(victim) = POSITION_FIGHTING;
  }
  if (GET_POS(ch) > POSITION_STUNNED) {
   if (!(ch->specials.fighting))
    set_fighting(ch, victim);
   if (IS_NPC(ch) && IS_NPC(victim) &&
          victim->master &&
       !number(0,6) && IS_AFFECTED(victim, AFF_CHARM) &&
       (victim->master->in_room == ch->in_room)) {
    if (ch->specials.fighting)
     stop_fighting(ch);
    hit(ch, victim->master, TYPE_UNDEFINED);
    return;
   }
  }
  if(IS_AFFECTED(ch,AFF_CHARM) && IS_NPC(ch) && !IS_NPC(victim) && ch->master){
   stop_fighting(ch);
   affect_from_char(ch,SPELL_CHARM_PERSON);
   if(IS_SET(ch->specials.affected_by,AFF_CHARM))
     REMOVE_BIT(ch->specials.affected_by,AFF_CHARM | AFF_GROUP);
   if(ch->in_room == ch->master->in_room)
    hit(ch,ch->master,TYPE_UNDEFINED);
   return;
  }
 }
 if (victim->master == ch)
  stop_follower(victim);
 if (IS_AFFECTED(ch, AFF_INVISIBLE))
  appear(ch);
 if (IS_AFFECTED(victim, AFF_SANCTUARY))
  dam >>= 1;
 if(attacktype < 100){
  if((attacktype < MAXSPELL) || (attacktype >= SPELL_MAGIC_MISSILE_WPN)){
    if(GET_INT(ch) > 75)
      dam=dam+(dam*(GET_INT(ch)-75))/50;
    if(k=victim->specials.magres){
      if(k > 100)
        k=100;
      if(dam < 1000000){
        dam -= (k*dam)/100;
      } else {
        dam -= k*(dam/100);
      }
    }
  }
 }
 dam=MIN(GET_HIT(victim)+13,dam);
 if(dam < 0)
   dam=0;
 GET_HIT(victim)-=dam;
 if(IS_NPC(victim) && (ch != victim))
  gain_exp(ch,dam);
 update_pos(victim);
 if ((attacktype >= TYPE_HIT) && (attacktype < TYPE_SHOOT)) {
  if (!ch->equipment[WIELD]) {
   dam_message(dam, ch, victim, TYPE_HIT);
  } else {
   dam_message(dam, ch, victim, attacktype);
  }
 } else if(attacktype != TYPE_SHOOT) {
 for(i = 0; i < MAX_MESSAGES; i++) {
  if (fight_messages[i].a_type == attacktype) {
   nr=dice(1,fight_messages[i].number_of_attacks);
   for(j=1,messages=fight_messages[i].msg;(j<nr)&&(messages);j++)
    messages=messages->next;
   if (!IS_NPC(victim) && (GET_LEVEL(victim) >= IMO)) {
    act(messages->god_msg.attacker_msg, FALSE, ch,
         ch->equipment[WIELD], victim, TO_CHAR);
    act(messages->god_msg.victim_msg, FALSE, ch,
         ch->equipment[WIELD], victim, TO_VICT);
    act(messages->god_msg.room_msg, FALSE, ch,
         ch->equipment[WIELD], victim, TO_NOTVICT);
   } else if (dam != 0) {
    if (GET_POS(victim) == POSITION_DEAD) {
     act(messages->die_msg.attacker_msg, FALSE, ch,
          ch->equipment[WIELD], victim, TO_CHAR);
     act(messages->die_msg.victim_msg, FALSE, ch,
          ch->equipment[WIELD], victim, TO_VICT);
     act(messages->die_msg.room_msg, FALSE, ch,
           ch->equipment[WIELD], victim, TO_NOTVICT);
    } else {
     act(messages->hit_msg.attacker_msg, FALSE, ch,
          ch->equipment[WIELD], victim, TO_CHAR);
     act(messages->hit_msg.victim_msg, FALSE, ch,
          ch->equipment[WIELD], victim, TO_VICT);
     act(messages->hit_msg.room_msg, FALSE, ch,
          ch->equipment[WIELD], victim, TO_NOTVICT);
    }
   } else { /* Dam == 0 */
    act(messages->miss_msg.attacker_msg, FALSE, ch,
         ch->equipment[WIELD], victim, TO_CHAR);
    act(messages->miss_msg.victim_msg, FALSE, ch,
         ch->equipment[WIELD], victim, TO_VICT);
    act(messages->miss_msg.room_msg, FALSE, ch,
         ch->equipment[WIELD], victim, TO_NOTVICT);
   }
  }
 }
 }
 switch (GET_POS(victim)) {
  case POSITION_MORTALLYW:
   act("$n has been mortally wounded, and begins to stink.", TRUE, victim, 0, 0, TO_ROOM);
   act("You are mortally wounded, and begin to stink.", FALSE, victim, 0, 0, TO_CHAR);
   break;
  case POSITION_INCAP:
   act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
   act("You are incapacitated an will slowly die, if not aided.", FALSE, victim, 0, 0, TO_CHAR);
   break;
  case POSITION_STUNNED:
   act("$n is stunned, but could regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
   act("You're stunned, but could regain consciousness again.", FALSE, victim, 0, 0, TO_CHAR);
   break;
  case POSITION_DEAD:
   act("$n is dead! R.I.P.", TRUE, victim, 0, 0, TO_ROOM);
   act("You are dead!  Sorry...", FALSE, victim, 0, 0, TO_CHAR);
   break;

  default:  /* >= POSITION SLEEPING */
   max_hit=hit_limit(victim);
   if (dam > (max_hit/5))
    act("That Really did HURT!",FALSE, victim, 0, 0, TO_CHAR);
   if (GET_HIT(victim) < (max_hit/5))
    act("You wish that your wounds would stop BLEEDING that much!",
      FALSE,victim,0,0,TO_CHAR);
   if(GET_HIT(victim) < (max_hit/5))
    if(IS_NPC(victim))
     if(IS_SET(victim->specials.act, ACT_WIMPY)){
       if(GET_POS(victim) == POSITION_SITTING)
        do_stand(victim, "", 0);
       do_flee(victim, "", 0);
     } else {
      for(tmp=world[victim->in_room].people;tmp;tmp=tmp->next_in_room)
        if((tmp!=victim)&&(IS_NPC(tmp))&&
           (tmp->specials.fighting==victim->specials.fighting)&&
           (IS_SET(tmp->specials.act, ACT_HELPER))&&
           (GET_HIT(tmp) > (GET_HIT(victim)<<1))&&
           (GET_HIT(tmp) > 0)&&
           (GET_HIT(victim) > 0)){
          rescuesub(tmp,victim);
          break;
        }
     }
   break;  
 }
 if(!IS_NPC(victim) && !(victim->desc)) {
  if (!victim->specials.fighting) {
   act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
   victim->specials.was_in_room = victim->in_room;
   char_from_room(victim);
   char_to_room(victim, 0);
   GET_MANA(victim) = GET_MOVE(victim) = 0;
  }
 }
 if (GET_POS(victim) < POSITION_MORTALLYW)
  if (ch->specials.fighting == victim)
   stop_fighting(ch);
 if (!AWAKE(victim))
  if (victim->specials.fighting)
   stop_fighting(victim);
 if (GET_POS(victim) == POSITION_DEAD) {
  if (IS_NPC(victim)){
   if (IS_AFFECTED(ch, AFF_GROUP)) {
     group_gain(ch,victim,getlevelsum(ch));
   } else {
    exp = GET_EXP(victim);
    gain_exp(ch,exp);
    sprintf(buf,"You get %d experience point(s).\n\r",exp);
    send_to_char(buf,ch);
    change_alignment(ch,victim);
   }
   if(!IS_NPC(ch))
     (ch->points.kills)++;
  }
  if (!IS_NPC(victim)) {
   (victim->points.deaths)++;
   sprintf(buf, "%s killed (%d) by %s at %s",
    GET_NAME(victim), attacktype,
    (IS_NPC(ch) ? ch->player.short_descr : GET_NAME(ch)),
    world[victim->in_room].name);
   log(buf);
   if(IS_NPC(ch)){
    sprintf(buf,taunt[number(0,NTAUNT)],GET_NAME(victim));
    do_shout(ch,buf,0);
   }
  }
  lootflag = IS_NPC(victim) && !IS_NPC(ch) &&
             IS_SET(ch->specials.act, PLR_AUTOLOOT);
  die(victim);
  if(lootflag){
    struct obj_data *o;

    if(o = get_obj_in_list_vis(ch,"corpse",world[ch->in_room].contents))
      do_get(ch,"all corpse",0);
  }
 }
}

void hit(struct char_data *ch, struct char_data *victim, int type)
{

 struct obj_data *wielded = 0;
 struct obj_data *held = 0;
 int w_type,lo,hi,i;
 int off,def,loff,ldef;
 int bam,dam,sum,ff,n;
 char buf[256];
 extern int mischance,hitchance;

 if (ch->in_room != victim->in_room) {
  if(ch->specials.fighting == victim)
   stop_fighting(ch);
  return;
 }
 lo=WIELD;
 if((type==TYPE_UNDEFINED)&&(held=ch->equipment[HOLD])&&
    (held->obj_flags.type_flag == ITEM_WEAPON))
  hi=HOLD;
 else
  hi=WIELD;
 for(i=lo;i<=hi;i++){
  if (ch->equipment[i] &&
     (ch->equipment[i]->obj_flags.type_flag == ITEM_WEAPON)) {
   wielded = ch->equipment[i];
   switch (wielded->obj_flags.value[3]) {
    case 2  : w_type = TYPE_WHIP; break;
    case 3  : w_type = TYPE_SLASH; break;
    case 4  : w_type = TYPE_BLAST; break;
    case 5  : w_type = TYPE_SPLAT; break;
    case 6  : w_type = TYPE_SPANK; break;
    case 7  : w_type = TYPE_BLUDGEON; break;
    case 8  : w_type = TYPE_CRUSH; break;
    case 9  : w_type = TYPE_BURN; break;
    case 11 : w_type = TYPE_PIERCE; break;
    default : w_type = TYPE_HIT; break;
   }
  } else {
   if (IS_NPC(ch) && (ch->specials.attack_type >= TYPE_HIT))
    w_type = ch->specials.attack_type;
   else
    w_type = TYPE_HIT;
  }
  n=number(0,99);
  if(n < mischance){
    sum=(-1);
  } else if(n > hitchance){
    sum=1;
  } else {
    loff = GET_LEVEL(ch); ldef = GET_LEVEL(victim);
    off = loff + GET_STR(ch) + GET_DEX(ch) + GET_HITROLL(ch);
    def = ldef + GET_STR(victim) + GET_DEX(victim) + GET_AC(victim);
    sum = 1 + number(0,loff) + off - def;
  }
  if (AWAKE(victim) && (sum < 0)){
   if (type == SKILL_BACKSTAB)
    damage(ch,victim,0,SKILL_BACKSTAB);
   else
    damage(ch,victim,0,w_type);
  } else {
   dam = GET_DAMROLL(ch) + GET_STR(ch) - GET_CON(victim);
   if(dam < 1) dam=1;
   if (IS_NPC(ch))
    dam += dice(ch->specials.damnodice, ch->specials.damsizedice);
   else
    dam += 1;
   if(wielded){
    dam += dice(wielded->obj_flags.value[1],wielded->obj_flags.value[2]);
    if(IS_SET(wielded->obj_flags.extra_flags,ITEM_SFX)){
      ff=(ch->specials.fighting != NULL);
      weapon_effects(ch,victim,wielded);
      if(ch->specials.fighting){
        if(ch->specials.fighting->in_room != ch->in_room) return;
      } else {
        if(ff) return;
      }
    }
   }
   if (GET_POS(victim) < POSITION_FIGHTING)
    dam *= 1+(POSITION_FIGHTING-GET_POS(victim))/3;
   dam = MAX(1, dam);  /* Not less than 0 damage */
   if (type == SKILL_BACKSTAB) {
    bam = 2+(GET_LEVEL(ch)/10);
    dam *= bam;
    damage(ch, victim, dam, SKILL_BACKSTAB);
   } else
    damage(ch, victim, dam, w_type);
  }
 }
}

/* control the fights going on */
void perform_violence(void)
{
 struct char_data *ch;

 for (ch = combat_list; ch; ch=combat_next_dude) {
  combat_next_dude = ch->next_fighting;
  assert(ch->specials.fighting);
  if (AWAKE(ch) && (ch->in_room==ch->specials.fighting->in_room)) {
   hit(ch, ch->specials.fighting, TYPE_UNDEFINED);
  } else { /* Not in same room */
   stop_fighting(ch);
  }
 }
}
void weapon_effects(struct char_data *ch, struct char_data *vict,
 struct obj_data *weapon)
{
  int n,w,l,s;

  n=weapon->obj_flags.value[0];
  if(!wsplist[n].spellfun) return;
  w=GET_WIS(ch);
  if(number(wsplist[n].lo,wsplist[n].hi) < w){
    s=wsplist[n].shift;
    l=(s) ? wsplist[n].lev+(w>>s) : wsplist[n].lev;
    (*wsplist[n].spellfun)(l,ch,vict,weapon);
  }
}


