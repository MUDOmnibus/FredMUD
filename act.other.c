/* ************************************************************************
*  file: act.other.c , Implementation of commands.        Part of DIKUMUD *
*  Usage : Other commands.                                                *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************* */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"

/* extern variables */

extern dig_room, dig_vnum;
extern int top_of_world;
extern struct index_data *obj_index;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct spell_info_type spell_info[];

/* extern procedures */

#ifdef NEEDS_STRDUP
char *strdup(char *s);
#endif
void stash_char(struct char_data *ch, char *filename);
void wipe_stash(char *filename);
void hit(struct char_data *ch, struct char_data *victim, int type);
void do_shout(struct char_data *ch, char *argument, int cmd);
void do_quit(struct char_data *ch, char *argument, int cmd)
{
  void die(struct char_data *ch);

  if (IS_NPC(ch) || !ch->desc)
    return;
  if (GET_POS(ch) == POSITION_FIGHTING) {
    send_to_char("No way! You are fighting.\n\r", ch);
    return;
  }
  if (GET_POS(ch) < POSITION_STUNNED) {
    send_to_char("You die before your time!\n\r", ch);
    die(ch);
    return;
  }
  wipe_stash(GET_NAME(ch));
  act("Goodbye, friend.. Come back soon!", FALSE, ch, 0, 0, TO_CHAR);
  act("$n has left the game.", TRUE, ch,0,0,TO_ROOM);
  extract_char(ch); /* Char is saved in extract char */
  ch->in_room = NOWHERE;
}

void do_not_here(struct char_data *ch, char *argument, int cmd)
{
  send_to_char("Sorry, but you cannot do that here!\n\r",ch);
}

void do_sneak(struct char_data *ch, char *argument, int cmd)
{
  struct affected_type af;
  byte percent;

  send_to_char("Ok, you'll try to move silently for a while.\n\r", ch);
  if (IS_AFFECTED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);
  percent=number(1,101); /* 101% is a complete failure */
  if(percent > ch->skills[SKILL_SNEAK].learned)
    return;
  af.type = SKILL_SNEAK;
  af.duration = GET_LEVEL(ch);
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_SNEAK;
  affect_to_char(ch, &af);
}

void do_hide(struct char_data *ch, char *argument, int cmd)
{
  byte percent;

  send_to_char("You attempt to hide yourself.\n\r", ch);
  if (IS_AFFECTED(ch, AFF_HIDE))
    REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);
  percent=number(1,101); /* 101% is a complete failure */
  if(percent > (ch->skills[SKILL_HIDE].learned + GET_DEX(ch)))
    return;
  SET_BIT(ch->specials.affected_by, AFF_HIDE);
}

void do_steal(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  struct obj_data *obj;
  char victim_name[240];
  char obj_name[240];
  char buf[240];
  int percent, bits;
  bool equipment = FALSE;
  int gold, eq_pos;
  bool ohoh = FALSE;
  extern int nostealflag;

  argument = one_argument(argument, obj_name);
  one_argument(argument, victim_name);

  if (!(victim = get_char_room_vis(ch, victim_name))) {
    send_to_char("Steal what from who?\n\r", ch);
    return;
  } else if (victim == ch) {
    send_to_char("Come on now, that's rather stupid!\n\r", ch);
    return;
  }
  if(IS_SET(world[ch->in_room].room_flags,LAWFUL))
    return;
  if((!IS_NPC(victim))&&
     (!IS_SET(world[ch->in_room].room_flags,PK_ROOM))&&
     (nostealflag)){
    act("Oops..", FALSE, ch,0,0,TO_CHAR);
    act("$n tried to steal something from you!",FALSE,ch,0,victim,TO_VICT);
    act("$n tries to rob $N.", TRUE, ch, 0, victim, TO_NOTVICT);
    return;
  }
  if(victim->specials.fighting) {
    send_to_char("You can't steal from a fighting person, too alert!\n\r",ch);
    return;
  }
  percent=number(1,101)+(GET_DEX(victim)/10)-(GET_DEX(ch)/10);
  if(victim->specials.alignment > 0)
    percent+=((victim->specials.alignment)>>5);
  if (str_cmp(obj_name, "coins") && str_cmp(obj_name,"gold")) {
    if (!(obj = get_obj_in_list_vis(victim, obj_name, victim->carrying))) {
      for (eq_pos = 0; (eq_pos < MAX_WEAR); eq_pos++)
        if (victim->equipment[eq_pos] &&
           (isname(obj_name, victim->equipment[eq_pos]->name)) &&
            CAN_SEE_OBJ(ch,victim->equipment[eq_pos])) {
          obj = victim->equipment[eq_pos];
          break;
        }
      if (!obj) {
        act("$E has not got that item.",FALSE,ch,0,victim,TO_CHAR);
        return;
      } else { /* It is equipment */
        send_to_char("Steal the equipment? Impossible!\n\r", ch);
        return;
      }
    } else {  /* obj found in inventory */
      percent += GET_OBJ_WEIGHT(obj); /* Make heavy harder */
      if (AWAKE(victim) && (percent > ch->skills[SKILL_STEAL].learned)) {
        ohoh = TRUE;
        act("Oops..", FALSE, ch,0,0,TO_CHAR);
        act("$n tries to rob $N.", TRUE, ch, 0, victim, TO_NOTVICT);
        if(!IS_NPC(victim))
          act("$n tried to rob you.",TRUE,ch,0,victim,TO_VICT);
      } else { /* Steal the item */
        if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
          if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch)) {
            obj_from_char(obj);
            obj_to_char(obj, ch);
            send_to_char("Got it!\n\r", ch);
            if(!IS_NPC(victim))
              act("$n robbed you!",TRUE,ch,0,victim,TO_VICT);
          }
        } else
          send_to_char("You cannot carry that much.\n\r", ch);
      }
    }
  } else { /* Steal some coins */
    if (AWAKE(victim) && (percent > ch->skills[SKILL_STEAL].learned)) {
      ohoh = TRUE;
      act("Oops..", FALSE, ch,0,0,TO_CHAR);
      act("$n tries to steal gold from $N.",TRUE,ch,0,victim,TO_NOTVICT);
      if(!IS_NPC(victim))
        act("$n tried to rob you.",TRUE,ch,0,victim,TO_VICT);
    } else {
      /* Steal some gold coins */
      gold = (int) ((GET_GOLD(victim)*number(1,25))/100);
      if (gold > 0) {
        GET_GOLD(ch) += gold;
        GET_GOLD(victim) -= gold;
        sprintf(buf, "Bingo! You got %d gold coins.\n\r", gold);
        send_to_char(buf, ch);
        if(!IS_NPC(victim))
          act("$n robbed you.",TRUE,ch,0,victim,TO_VICT);
      } else {
        send_to_char("You couldn't get any gold...\n\r", ch);
      }
    }
  }
  if (ohoh && AWAKE(victim))
    if (!IS_SET(victim->specials.act, ACT_NICE_THIEF))
      hit(victim, ch, TYPE_UNDEFINED);
}

bool do_practice(struct char_data *ch, char *arg, int cmd)
{
  int i,j;
  char buf[MAX_STRING_LENGTH],buf2[128];
  extern char *spells[];
  extern struct spell_info_type spell_info[MAX_SPL_LIST];

  strcpy(buf,"Report card:\n\r");
  for(i=j=0; *spells[i] != '\n';i++) {
    if(! *spells[i]) continue;
    if(! ch->skills[i+1].learned) continue;
    if(spell_info[i+1].min_level > GET_LEVEL(ch)) continue;
    sprintf(buf2,"%-25s%4d%s",spells[i],
      ch->skills[i+1].learned,
      (j) ? "\n\r" : " | ");
    j=1-j;
    strcat(buf,buf2);
  }
  if(j) strcat(buf,"\n\r");
  send_to_char(buf, ch);
  return(TRUE);
}

void do_brief(struct char_data *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return;
  if (IS_SET(ch->specials.act, PLR_BRIEF)) {
    send_to_char("Brief mode off.\n\r", ch);
    REMOVE_BIT(ch->specials.act, PLR_BRIEF);
  } else {
    send_to_char("Brief mode on.\n\r", ch);
    SET_BIT(ch->specials.act, PLR_BRIEF);
  }
}

void do_compact(struct char_data *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return;
  if (IS_SET(ch->specials.act, PLR_COMPACT)) {
    send_to_char("You are now in the uncompacted mode.\n\r", ch);
    REMOVE_BIT(ch->specials.act, PLR_COMPACT);
  } else {
    send_to_char("You are now in compact mode.\n\r", ch);
    SET_BIT(ch->specials.act, PLR_COMPACT);
  }
}

void do_verybrief(struct char_data *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return;
  if (IS_SET(ch->specials.act, PLR_VERYBRIEF)) {
    send_to_char("You will see fight messages.\n\r", ch);
    REMOVE_BIT(ch->specials.act, PLR_VERYBRIEF);
  } else {
    send_to_char("You will not see fight messages.\n\r", ch);
    SET_BIT(ch->specials.act, PLR_VERYBRIEF);
  }
}

void do_group(struct char_data *ch, char *argument, int cmd)
{
  char name[256],buf[256];
  struct char_data *victim, *k;
  struct follow_type *f;
  bool found;

  one_argument(argument, name);

  if (!*name) {
    if (!IS_AFFECTED(ch, AFF_GROUP)) {
      send_to_char("But you are a member of no group?!\n\r", ch);
    } else {
      send_to_char("Your group consists of:\n\r", ch);
      if (ch->master)
        k = ch->master;
      else
        k = ch;
      if (IS_AFFECTED(k, AFF_GROUP)){
        sprintf(buf,"%-16s* %d/%d HP, %d/%d MA, %d/%d MV\n\r",
          GET_NAME(k),
          GET_HIT(k),GET_MAX_HIT(k),
          GET_MANA(k),GET_MAX_MANA(k),
          GET_MOVE(k),GET_MAX_MOVE(k));
        send_to_char(buf,ch);
      }
      for(f=k->followers; f; f=f->next)
        if (IS_AFFECTED(f->follower, AFF_GROUP)){
          sprintf(buf,"%-16s  %d/%d HP, %d/%d MA, %d/%d MV\n\r",
            GET_NAME(f->follower),
            GET_HIT(f->follower),GET_MAX_HIT(f->follower),
            GET_MANA(f->follower),GET_MAX_MANA(f->follower),
            GET_MOVE(f->follower),GET_MAX_MOVE(f->follower));
          send_to_char(buf,ch);
        }
    }
    return;
  }

  if (!(victim = get_char_room_vis(ch, name))) {
    send_to_char("No one here by that name.\n\r", ch);
  } else {

    if (ch->master) {
      act("You can not enroll group members without being head of a group.",
         FALSE, ch, 0, 0, TO_CHAR);
      return;
    }

    found = FALSE;

    if (victim == ch)
      found = TRUE;
    else {
      for(f=ch->followers; f; f=f->next) {
        if (f->follower == victim) {
          found = TRUE;
          break;
        }
      }
    }
    
    if (found) {
      if (IS_AFFECTED(victim, AFF_GROUP)) {
        act("$n has been kicked out of the group!", FALSE, victim, 0, ch, TO_ROOM);
        act("You are no longer a member of the group!", FALSE, victim, 0, 0, TO_CHAR);
        REMOVE_BIT(victim->specials.affected_by, AFF_GROUP);
      } else {
        act("$n is now a group member.", FALSE, victim, 0, 0, TO_ROOM);
        act("You are now a group member.", FALSE, victim, 0, 0, TO_CHAR);
        SET_BIT(victim->specials.affected_by, AFF_GROUP);
      }
    } else {
      act("$N must follow you, to enter the group", FALSE, ch, 0, victim, TO_CHAR);
    }
  }
}
void do_quaff(struct char_data *ch, char *argument, int cmd)
{
  char buf[100];
  struct obj_data *temp;
  int i;
  bool equipped;

  equipped = FALSE;

  one_argument(argument,buf);

  if (!(temp = get_obj_in_list_vis(ch,buf,ch->carrying))) {
    temp = ch->equipment[HOLD];
    equipped = TRUE;
    if ((temp==0) || !isname(buf, temp->name)) {
      act("You do not have that item.",FALSE,ch,0,0,TO_CHAR);
      return;
    }
  }
  if (temp->obj_flags.type_flag!=ITEM_POTION) {
    act("You can only quaff potions.",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  act("$n quaffs $p.", TRUE, ch, temp, 0, TO_ROOM);
  act("You quaff $p which dissolves.",FALSE,ch,temp,0,TO_CHAR);
  for (i=1; i<4; i++)
    if (temp->obj_flags.value[i] >= 1)
      if(spell_info[temp->obj_flags.value[i]].spell_pointer)
        ((*spell_info[temp->obj_flags.value[i]].spell_pointer)
          ((short) temp->obj_flags.value[0], ch, "", SPELL_TYPE_POTION, ch, 0));
  if(equipped)
    unequip_char(ch, HOLD);
  extract_obj(temp);
}


void do_recite(struct char_data *ch, char *argument, int cmd)
{
  char buf[100];
  struct obj_data *scroll, *obj;
  struct char_data *victim;
  int i, bits;
  bool equipped;

  equipped = FALSE;
  obj = 0;
  victim = 0;

  argument = one_argument(argument,buf);

  if (!(scroll = get_obj_in_list_vis(ch,buf,ch->carrying))) {
    scroll = ch->equipment[HOLD];
    equipped = TRUE;
    if ((scroll==0) || !isname(buf, scroll->name)) {
      act("You do not have that item.",FALSE,ch,0,0,TO_CHAR);
      return;
    }
  }

  if (scroll->obj_flags.type_flag!=ITEM_SCROLL) {
    act("Recite is normally used for scroll's.",FALSE,ch,0,0,TO_CHAR);
    return;
  }

  if (IS_SET(world[ch->in_room].room_flags,NO_MAGIC)){
    send_to_char("Your magic is powerless here.\n\r",ch);
    return;
  }
  if (*argument) {
    bits = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM |
        FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &victim, &obj);
    if (bits == 0) {
      send_to_char("No such thing around to recite the scroll on.\n\r", ch);
      return;
    }
  } else {
    victim = ch;
  }
  act("$n recites $p.", TRUE, ch, scroll, 0, TO_ROOM);
  act("You recite $p which dissolves.",FALSE,ch,scroll,0,TO_CHAR);
  for (i=1; i<4; i++)
    if (scroll->obj_flags.value[i] >= 1)
      if(spell_info[scroll->obj_flags.value[i]].spell_pointer)
        ((*spell_info[scroll->obj_flags.value[i]].spell_pointer)
          ((short) scroll->obj_flags.value[0], ch, "", SPELL_TYPE_SCROLL,
           victim, obj));
  if (equipped)
    unequip_char(ch, HOLD);
  extract_obj(scroll);
}

void do_use(struct char_data *ch, char *argument, int cmd)
{
  char buf[100];
  struct char_data *tmp_char;
  struct obj_data *tmp_object, *stick;

  int bits;

  argument = one_argument(argument,buf);

  if (ch->equipment[HOLD] == 0 ||
      !isname(buf, ch->equipment[HOLD]->name)) {
    act("You do not hold that item in your hand.",FALSE,ch,0,0,TO_CHAR);
    return;
  }

  stick = ch->equipment[HOLD];

  if (stick->obj_flags.type_flag == ITEM_STAFF)
  {
    act("$n taps $p three times on the ground.",TRUE, ch, stick, 0,TO_ROOM);
    act("You tap $p three times on the ground.",FALSE,ch, stick, 0,TO_CHAR);
    if (stick->obj_flags.value[2] > 0) {
      stick->obj_flags.value[2]--;
      if(spell_info[stick->obj_flags.value[3]].spell_pointer)
        ((*spell_info[stick->obj_flags.value[3]].spell_pointer)
        (stick->obj_flags.value[0], ch, "", SPELL_TYPE_STAFF, 0, 0));
    } else {
      send_to_char("The staff seems powerless.\n\r", ch);
    }
  } else if (stick->obj_flags.type_flag == ITEM_WAND) {

    bits = generic_find(argument, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
                        FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);
    if (bits) {
      if (bits == FIND_CHAR_ROOM) {
        act("$n point $p at $N.", TRUE, ch, stick, tmp_char, TO_ROOM);
        act("You point $p at $N.",FALSE,ch, stick, tmp_char, TO_CHAR);
      } else {
        act("$n point $p at $P.", TRUE, ch, stick, tmp_object, TO_ROOM);
        act("You point $p at $P.",FALSE,ch, stick, tmp_object, TO_CHAR);
      }

      if (stick->obj_flags.value[2] > 0) { /* Is there any charges left? */
        stick->obj_flags.value[2]--;
        if(spell_info[stick->obj_flags.value[3]].spell_pointer)
          ((*spell_info[stick->obj_flags.value[3]].spell_pointer) ((short) stick->obj_flags.value[0], ch, "", SPELL_TYPE_WAND, tmp_char, tmp_object));
      } else {
        send_to_char("The wand seems powerless.\n\r", ch);
      }
    } else {
      send_to_char("What should the wand be pointed at?\n\r", ch);
    }
  } else {
    send_to_char("Use is normally only for wand's and staff's.\n\r", ch);
  }
}
void do_dig(struct char_data *ch, char *argument, int cmd)
{
  int i,j,cost,newroom,oldroom,olddir,newdir;
  char buf[MAX_STRING_LENGTH];
  struct obj_data *obj;

  if(GET_LEVEL(ch) < IMO){
    obj=ch->equipment[HOLD];
    if((!obj)||(GET_ITEM_TYPE(obj) != ITEM_SHOVEL)){
      send_to_char("You have to hold a shovel to dig.\n\r",ch);
      return;
    }
    if(obj->obj_flags.value[3] <= 0){
      send_to_char("Your equipment is worn out.\n\r",ch);
      return;
    }
    if(IS_SET(world[ch->in_room].room_flags,NO_DIG)){
      send_to_char("The ground is too hard to dig here.\n\r",ch);
      return;
    }
    if(world[ch->in_room].sector_type==SECT_WATER_NOSWIM){
      send_to_char("You can't DIG in water.\n\r",ch);
      return;
    }
  }
  cost = (1 << ch->specials.holes);
  if(GET_GOLD(ch) < cost){
    send_to_char("You can't afford to dig another hole!\n\r",ch);
    return;
  }
  GET_GOLD(ch) -= cost;
  (ch->specials.holes)++;
  oldroom=ch->in_room;
  for(;isspace(*argument);++argument);
  switch(argument[0]){
    case 'n': olddir=0; newdir=2; break;
    case 'e': olddir=1; newdir=3; break;
    case 's': olddir=2; newdir=0; break;
    case 'w': olddir=3; newdir=1; break;
    case 'u': olddir=4; newdir=5; break;
    case 'd': olddir=5; newdir=4; break;
    default :
      {
        send_to_char("Can't dig THAT way!\n\r",ch);
        return;
      }
  }
  if((GET_LEVEL(ch) < IMO) && (olddir != 5)){
    if(obj->obj_flags.value[2]==0){
      send_to_char("Your equipment is only good for digging down.\n\r",ch);
      return;
    }
    if(olddir == 4){
      send_to_char("Your equipment can't dig UP.\n\r",ch);
      return;
    }
  }
  if(EXIT(ch,olddir)){
    send_to_char("There's already an exit in that direction.\n\r",ch);
    return;
  }
  newroom=dig_room;
  if(newroom==0){
    send_to_char("Unable to dig for some reason...\n\r",ch);
    return;
  }
  if(GET_LEVEL(ch) < IMO)
    --obj->obj_flags.value[3];
  for(;isalpha(*argument);++argument);
  for(;isspace(*argument);++argument);
  free(world[newroom].name);
  if(isupper(*argument))
    sprintf(buf,"%s",argument);
  else
    sprintf(buf,"A Room Dug by %s",GET_NAME(ch));
  world[newroom].room_flags = 16396;
  world[newroom].name=(char *)strdup(buf);
  free(world[newroom].description);
  sprintf(buf,"You are in a room dug by %s.\n\r",
    (GET_LEVEL(ch) > IMO+1) ? "natural forces" : GET_NAME(ch));
  world[newroom].description=(char *)strdup(buf);
  CREATE(world[newroom].dir_option[newdir],struct room_direction_data,1);
  world[newroom].dir_option[newdir]->general_description=0;
  world[newroom].dir_option[newdir]->keyword=0;
  world[newroom].dir_option[newdir]->exit_info=0;
  world[newroom].dir_option[newdir]->key=(-1);
  world[newroom].dir_option[newdir]->to_room=oldroom;
  CREATE(world[oldroom].dir_option[olddir],struct room_direction_data,1);
  world[oldroom].dir_option[olddir]->general_description=0;
  world[oldroom].dir_option[olddir]->keyword=0;
  world[oldroom].dir_option[olddir]->exit_info=0;
  world[oldroom].dir_option[olddir]->key=(-1);
  world[oldroom].dir_option[olddir]->to_room=newroom;
  act("You dig for a while.", FALSE, ch, 0, 0, TO_CHAR);
  act("$n digs like crazy for while.", TRUE, ch,0,0,TO_ROOM);
  dig_room++;
  dig_vnum++;
}
void do_split(struct char_data *ch, char *argument, int cmd)
{
  char name[256],buf[256];
  struct char_data *k;
  struct follow_type *f;
  int m,n;

  one_argument(argument, name);

  if (!*name) {
    send_to_char("Split what?\n\r",ch);
    return;
  }
  if (!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("You must belong to a group to split.\n\r", ch);
    return;
  }
  m=atoi(name);
  if(m <= 0){
    send_to_char("Seems like you don't quite have the hang of this.\n\r",ch);
    return;
  }
  if(m > GET_GOLD(ch)){
    send_to_char("Hard to split what you don't have.\n\r",ch);
    return;
  }
  if (ch->master)
    k = ch->master;
  else
    k = ch;
  n=0;
  if (IS_AFFECTED(k, AFF_GROUP))
    ++n;
  for(f=k->followers; f; f=f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP))
      ++n;
  if(n < 2){
    send_to_char("Why bother?\n\r",ch);
    return;
  }
  if (IS_AFFECTED(k, AFF_GROUP)){
    sprintf(buf,"%d coins %s",m/n,GET_NAME(k));
    do_give(ch,buf,0);
  }
  for(f=k->followers; f; f=f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP)){
      sprintf(buf,"%d coins %s",m/n,GET_NAME(f->follower));
      do_give(ch,buf,0);
  }
}
void do_xyzzy(struct char_data *ch, char *argument, int cmd)
{
  if (IS_SET(world[ch->in_room].room_flags,NO_MAGIC))
    ch->specials.recall_room=0;
  else
    ch->specials.recall_room=ch->in_room;
  send_to_char("OK!\n\r",ch);
  return;
}
void do_tag(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char victim_name[240];

  if(!*argument){
    send_to_char("Tag whom?\n\r",ch);
    return;
  }
  one_argument(argument, victim_name);
  if (!(victim = get_char_room_vis(ch, victim_name))) {
    send_to_char("Where'd that bugger go?\n\r", ch);
    return;
  } else if (victim == ch) {
    send_to_char("Come on now, that's rather stupid.\n\r", ch);
    return;
  } else if (IS_NPC(victim)) {
    send_to_char("Sorry, monsters can't play tag.\n\r", ch);
    return;
  } else if ((GET_LEVEL(ch) < IMO) && (!ch->specials.it)){
    send_to_char("You aren't IT!\n\r",ch);
    return;
  } else if (GET_LEVEL(victim) >= IMO) {
    send_to_char("It is pointless to tag a so-called immortal.\n\r",ch);
    return;
  } else if (victim->specials.it){
    act("$N is already it!",FALSE,ch,0,victim,TO_CHAR);
    return;
  }
  if(IS_SET(world[ch->in_room].room_flags,LAWFUL)){
    send_to_char("Sorry, no tagging allowed in here.\n\r",ch);
    return;
  }
  if(ch->specials.it)
    ch->specials.it = 0;
  victim->specials.it = 1;
  act("You tag $N!",TRUE,ch,0,victim,TO_CHAR);
  act("$n tags you!",TRUE,ch,0,victim,TO_VICT);
  act("$n tags $N!",TRUE,ch,0,victim,TO_NOTVICT);
}
void do_heal(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_INPUT_LENGTH];
  int avail,used,factor,newmax;

  if(IS_NPC(ch))
    return;
  one_argument(argument,buf);
  avail=GET_MANA(ch);
  if (!*buf) {
    used=avail;
    if(used < 1){
      send_to_char("Wouldn't be prudent.\n\r",
        ch);
      return;
    }
  } else if (isdigit(*buf)) {
    used=atoi(buf);
    if(used > avail){
      send_to_char("Nice try, but you don't have that much.\n\r",
        ch);
      return;
    } else if(used < 1){
      send_to_char("Why bother?\n\r",ch);
      return;
    }
  } else {
    send_to_char("How much of your mana do you want to use for healing?\n\r",
      ch);
    return;
  }
  factor=number(2,3+(GET_LEVEL(ch)/111));
  newmax = GET_HIT(ch)+factor*used;
  if((GET_HIT(ch) > 1000000) && (newmax < 0))
    newmax = 0x7fffffff;
  GET_HIT(ch)=MIN(GET_MAX_HIT(ch),newmax);
  GET_MANA(ch)-=used;
  act("You heal yourself.",TRUE,ch,0,0,TO_CHAR);
  act("$n heals $mself.",TRUE,ch,0,0,TO_ROOM);
}
void do_piss(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *victim;

  if(IS_NPC(ch))
    return;
  one_argument(argument,buf);
  if(*buf){
    victim = get_char_room_vis(ch, buf);
    if(victim){
      act("You piss on $N.",TRUE,ch,0,victim,TO_CHAR);
      act("$n pisses on you, most everyone else.",TRUE,ch,0,victim,TO_VICT);
      act("$n pisses on $N, and it splatters on everyone else in the room.",
        TRUE,ch,0,victim,TO_NOTVICT);
      if(IS_NPC(victim))
        do_report(victim,"a",0);
    } else {
      act("You piss all over the room.",TRUE,ch,0,0,TO_CHAR);
      act("$n pisses on nobody in particular.",TRUE,ch,0,0,TO_ROOM);
    }
  } else {
    if(IS_SET(world[ch->in_room].room_flags,UNOWNED)){
      if(world[ch->in_room].name)
        free(world[ch->in_room].name);
      sprintf(buf,"A Room Marked by %s",GET_NAME(ch));
      world[ch->in_room].name=strdup(buf);
      REMOVE_BIT(world[ch->in_room].room_flags,UNOWNED);
    }
    act("You piss all over the room.",TRUE,ch,0,0,TO_CHAR);
    act("$n pisses all over the place.",TRUE,ch,0,0,TO_ROOM);
  }
  return;
}
void make_puke(struct char_data *ch)
{
  extern struct obj_data *object_list;
  struct obj_data *pu;
  char buf[MAX_STRING_LENGTH];
#ifdef NEEDS_STRDUP
  char *strdup(char *source);
#endif

  if(GET_COND(ch,FULL)==0){
    send_to_char("Your stomach is empty.\n\r",ch);
    return;
  }
  if(GET_COND(ch,FULL) > 0)
    --GET_COND(ch,FULL);
  CREATE(pu, struct obj_data, 1);
  clear_object(pu);
  pu->obj_flags.wear_flags=1;
  pu->obj_flags.weight=10;
  pu->name = strdup("puke");
  pu->in_room = NOWHERE;
  pu->item_number = NOWHERE;
  sprintf(buf,"Some %s Puke",
    IS_NPC(ch) ? ch->player.short_descr : GET_NAME(ch));
  pu->short_description = strdup(buf);
  sprintf(buf,"Some %s puke is here.",
    IS_NPC(ch) ? ch->player.short_descr : GET_NAME(ch));
  pu->description = strdup(buf);
  pu->next = object_list;
  object_list = pu;
  obj_to_room(pu, ch->in_room);
}
void do_puke(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *victim;

  if(IS_NPC(ch))
    return;
  one_argument(argument,buf);
  if(*buf){
    victim = get_char_room_vis(ch, buf);
    if(victim){
      act("You puke on $N.",TRUE,ch,0,victim,TO_CHAR);
      act("$n pukes on you.",TRUE,ch,0,victim,TO_VICT);
      act("$n pukes all over $N.",TRUE,ch,0,victim,TO_NOTVICT);
    } else {
      act("You try to puke on someone.",TRUE,ch,0,0,TO_CHAR);
      act("$n tries to puke on someone.",TRUE,ch,0,0,TO_ROOM);
      make_puke(ch);
    }
  } else {
    act("You puke.",TRUE,ch,0,0,TO_CHAR);
    act("$n pukes.",TRUE,ch,0,0,TO_ROOM);
    make_puke(ch);
  }
  return;
}


