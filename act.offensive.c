/* ************************************************************************
*  file: act.offensive.c , Implementation of commands.    Part of DIKUMUD *
*  Usage : Offensive commands.                                            *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************* */

#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "limits.h"

/* extern variables */

extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;


void raw_kill(struct char_data *ch);

strip(struct char_data *ch)
{
  int i;
  struct obj_data *o;

  for(i=0;i<MAX_WEAR;i++){
    if(ch->equipment[i]){
      o=unequip_char(ch, i);
      act("$p disappears!",FALSE,ch,o,0,TO_CHAR);
      extract_obj(o);
      GET_GOLD(ch) = 0;
      ch->points.bank = 0;
    }
  }
}
void do_hit(struct char_data *ch, char *argument, int cmd)
{
  char arg[MAX_STRING_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  struct char_data *victim;
  extern int nokillflag;

  one_argument(argument, arg);

  if (*arg) {
    victim = get_char_room_vis(ch, arg);
    if (victim) {
      if (victim == ch) {
        send_to_char("You hit yourself..OUCH!.\n\r", ch);
        act("$n hits $mself, and says OUCH!", FALSE, ch, 0, victim, TO_ROOM);
      } else {
        if(IS_AFFECTED(ch, AFF_CHARM)){
          if(ch->master==victim){
            send_to_char("Hurt your master?\n\r",ch);
            return;
          }
          if(IS_NPC(ch) && !IS_NPC(victim) && ch->master &&
             (ch->in_room == ch->master->in_room)){
            affect_from_char(ch,SPELL_CHARM_PERSON);
            act("$n feels released!",FALSE,ch,0,0,TO_ROOM);
            return;
          }
        }
        if((GET_POS(ch)==POSITION_STANDING) &&
            (victim != ch->specials.fighting)) {
          hit(ch, victim, TYPE_UNDEFINED);
          WAIT_STATE(ch, PULSE_VIOLENCE+2);
        } else {
          send_to_char("You do the best you can!\n\r",ch);
        }
      }
    } else {
      send_to_char("They aren't here.\n\r", ch);
    }
  } else {
    send_to_char("Hit who?\n\r", ch);
  }
}

void do_kill(struct char_data *ch, char *argument, int cmd)
{
  static char arg[MAX_STRING_LENGTH];
  char buf[70];
  struct char_data *victim;

  if ((GET_LEVEL(ch) < (IMO+5)) || IS_NPC(ch)) {
    do_hit(ch, argument, 0);
    return;
  }
  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("Kill who?\n\r", ch);
  } else {
    if (!(victim = get_char_room_vis(ch, arg)))
       send_to_char("They aren't here.\n\r", ch);
    else
       if (ch == victim)
        send_to_char("Your mother would be so sad.. :(\n\r", ch);
       else {
        if(GET_LEVEL(victim) > (IMO+4)) return;
        act("You chop $M to pieces! Ah! The blood!",
          FALSE, ch, 0, victim, TO_CHAR);
        act("$N chops you to pieces!", FALSE, victim, 0, ch, TO_CHAR);
        act("$n brutally slays $N", FALSE, ch, 0, victim, TO_NOTVICT);
        raw_kill(victim);
      }
  }
}
void do_switch(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *v1 , *v2;
  char name[256];

  if(!(v1 = ch->specials.fighting)){
    send_to_char("You aren't even fighting, you piss brained idiot!\n\r",ch);
    return;
  }
  one_argument(argument, name);
  if (!(v2 = get_char_room_vis(ch, name))) {
    send_to_char("There's nobody here by that name, you moron!\n\r",ch);
    return;
  }
  if(v1 == v2){
    send_to_char("You're already fighting that one, goofball.\n\r",ch);
    return;
  }
  act("$n stops fighting $N.",TRUE,ch,0,v1,TO_ROOM);
  act("$n begins fighting $N.",TRUE,ch,0,v2,TO_ROOM);
  act("You stop fighting $N.",TRUE,ch,0,v1,TO_CHAR);
  act("You begin fighting $N.",TRUE,ch,0,v2,TO_CHAR);
  stop_fighting(ch);
  hit(ch,v2,TYPE_UNDEFINED);
}

#define BACKINT 60

void do_backstab(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char name[256];
  byte percent;
  int delay,t;

  one_argument(argument, name);

  if (!(victim = get_char_room_vis(ch, name))) {
    send_to_char("Backstab who?\n\r", ch);
    return;
  }
  if (victim == ch) {
    send_to_char("How can you sneak up on yourself?\n\r", ch);
    return;
  }
  if (!ch->equipment[WIELD]) {
    send_to_char("You need to wield a weapon, to make it a succes.\n\r",ch);
    return;
  }
  if (ch->equipment[WIELD]->obj_flags.value[3] != 11) {
    send_to_char("Only piercing weapons can be used for backstabbing.\n\r",ch);
    return;
  }
  if(IS_SET(world[ch->in_room].room_flags,LAWFUL)){
    send_to_char("Not here.\n\r",ch);
    return;
  }
  if (victim->specials.fighting) {
    send_to_char("You can't backstab a fighting person, too alert!\n\r", ch);
    return;
  }
  if((AWAKE(victim)) && IS_SET(victim->specials.act,ACT_SMART) && number(0,2)){
    act("$n alertly sidesteps $N.",TRUE,victim,0,ch,TO_NOTVICT);
    act("$n alertly sidesteps your feeble attack.",TRUE,victim,0,ch,TO_VICT);
    damage(victim,ch,GET_LEVEL(victim)<<2,SKILL_KICK);
    return;
  }
  t=time(0);
  if((GET_POS(victim) > POSITION_SLEEPING) &&
     (t-victim->specials.lastback < BACKINT)){
    victim->specials.lastback=t;
    act("$N was just backstabbed, you think $E's stupid??",
      FALSE,ch,0,victim,TO_CHAR);
    return;
  }
  victim->specials.lastback=t;
  percent=number(1,101);
  if(AWAKE(victim) && 
     (!IS_NPC(ch) && (percent > ch->skills[SKILL_BACKSTAB].learned)) &&
     (number(1,GET_AC(victim)) > (number(1,100) + GET_DEX(ch))))
    damage(ch,victim,0,SKILL_BACKSTAB);
  else
    hit(ch,victim,SKILL_BACKSTAB);
  if(GET_LEVEL(ch) >= 999)
    return;
  delay=1+(GET_LEVEL(victim) > GET_LEVEL(ch));
  WAIT_STATE(ch,delay*PULSE_VIOLENCE);
}
void do_order(struct char_data *ch, char *argument, int cmd)
{
  char name[100], message[256];
  char buf[256];
  bool found = FALSE;
  int i, org_room;
  struct char_data *victim;
  struct obj_data *o;
  struct follow_type *k;

  if(strlen(argument) > 80){
    fprintf(stderr,"LONG %s\n",GET_NAME(ch));
    return;
  }
  half_chop(argument, name, message);
  if (!*name || !*message)
    send_to_char("Order who to do what?\n\r", ch);
  else if (!(victim = get_char_room_vis(ch, name)) &&
           str_cmp("follower", name) && str_cmp("followers", name))
      send_to_char("That person isn't here.\n\r", ch);
  else if (ch == victim)
    send_to_char("You decline to do it.\n\r", ch);
  else {
    if (IS_AFFECTED(ch, AFF_CHARM)) {
      send_to_char("Your superior would not approve.\n\r",ch);
      return;
    }
    if (victim) {
      if(!IS_NPC(victim) && !victim->desc) return;
      sprintf(buf, "$N orders you to '%s'", message);
      act(buf, FALSE, victim, 0, ch, TO_CHAR);
      act("$n gives $N an order.", FALSE, ch, 0, victim, TO_ROOM);
      if ( (victim->master!=ch) || !IS_AFFECTED(victim, AFF_CHARM) )
        act("$n has an indifferent look.", FALSE, victim, 0, 0, TO_ROOM);
      else {
        if(!IS_NPC(ch) && !IS_NPC(victim) && (GET_LEVEL(ch) < IMO)){
          strip(ch);
          return;
        }
        send_to_char("Ok.\n\r", ch);
        command_interpreter(victim, message);
      }
    } else {  /* This is order "followers" */
      sprintf(buf, "$n issues the order '%s'.", message);
      act(buf, FALSE, ch, 0, victim, TO_ROOM);
      org_room = ch->in_room;
      for (k = ch->followers; k; k = k->next) {
        if (org_room == k->follower->in_room)
          if (IS_AFFECTED(k->follower, AFF_CHARM)) {
            found = TRUE;
            if(!IS_NPC(k->follower)){
              strip(ch);
              fprintf(stderr,"STRIP %s\n",GET_NAME(ch));
              return;
            }
            command_interpreter(k->follower, message);
          }
      }
      if (found)
        send_to_char("Ok.\n\r", ch);
      else
        send_to_char("Nobody here are loyal subjects of yours!\n\r", ch);
    }
  }
}

void do_flee(struct char_data *ch, char *argument, int cmd)
{
  int i, attempt, die, l1 ,l2;
  struct char_data *badguy;
  void gain_exp(struct char_data *ch, int gain);
  int special(struct char_data *ch, int cmd, char *arg);

  if(!(badguy=ch->specials.fighting)){
    for(i=0;i<3;i++){
      attempt = number(0, 5);
      if(attempt > 3) attempt = number(0, 5);
      if (CAN_GO(ch, attempt)) {
        act("$n panics, and attempts to flee.", TRUE, ch, 0, 0, TO_ROOM);
        if ((die = do_simple_move(ch, attempt, FALSE))== 1) {
          send_to_char("You flee head over heels.\n\r", ch);
          return;
        } else {
          if (!die)
            act("$n tries to flee, but is too exhausted!",TRUE,ch,0,0,TO_ROOM);
          return;
        }
      }
    }
    send_to_char("PANIC! You couldn't escape!\n\r", ch);
    return;
  }
  if(IS_MOB(badguy) && IS_AFFECTED(badguy,AFF_HOLD) &&
    (GET_LEVEL(ch) <= GET_LEVEL(badguy))){
    send_to_char("PANIC! You couldn't escape!\n\r", ch);
    return;
  }
  for(i=0;i<4;i++){
    attempt = number(0, 5);  /* Select a random direction */
    if (CAN_GO(ch, attempt)) {
      act("$n panics, and attempts to flee.", TRUE, ch, 0, 0, TO_ROOM);
      if ((die = do_simple_move(ch, attempt, FALSE))== 1) { 
        if(!IS_AFFECTED(ch,AFF_FEARLESSNESS)){
          if(!IS_NPC(ch) && (GET_HIT(ch) > (GET_MAX_HIT(ch)>>2))){
            l1=GET_LEVEL(ch);
            if(l1 > 100) l1=100;
            l2=ch->specials.fighting ? GET_LEVEL(ch->specials.fighting) : 1;
            if(l2 > 100) l2=100;
            gain_exp(ch, -XP4LEV(MIN(l1,l2))/100);
          }
        }
        send_to_char("You flee head over heels.\n\r", ch);
        /* Insert later when using huntig system        */
        /* ch->specials.fighting->specials.hunting = ch */
        if (ch->specials.fighting->specials.fighting == ch)
          stop_fighting(ch->specials.fighting);
        stop_fighting(ch);
        return;
      } else {
        if (!die)
          act("$n tries to flee, but is too exhausted!",TRUE,ch,0,0,TO_ROOM);
        return;
      }
    }
  } /* for */
  /* No exits was found */
  send_to_char("PANIC! You couldn't escape!\n\r", ch);
}



void do_bash(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char name[256], buf[256];
  byte percent;

  one_argument(argument, name);

  if (!(victim = get_char_room_vis(ch, name))) {
    if (ch->specials.fighting) {
      victim = ch->specials.fighting;
    } else {
      send_to_char("Bash who?\n\r", ch);
      return;
    }
  }
  if (victim == ch) {
    send_to_char("Aren't we funny today...\n\r", ch);
    return;
  }
  if(IS_AFFECTED(ch, AFF_CHARM) && (ch->master==victim)){
    send_to_char("Bash your master?\n\r",ch);
    return;
  }
  if (!ch->equipment[WIELD]) {
    send_to_char("You need to wield a weapon, to make it a success.\n\r",ch);
    return;
  }
  percent=number(1,101); /* 101% is a complete failure */
  if (percent > ch->skills[SKILL_BASH].learned) {
    damage(ch, victim, 0, SKILL_BASH);
  } else {
    damage(ch, victim, GET_LEVEL(ch), SKILL_BASH);
    GET_POS(victim) = POSITION_SITTING;
    WAIT_STATE(victim, PULSE_VIOLENCE);
  }
  WAIT_STATE(ch, PULSE_VIOLENCE*2);
}

void rescuesub(struct char_data *ch, struct char_data *victim)
{
  struct char_data *tmp_ch;

  tmp_ch=victim->specials.fighting;
  if(!tmp_ch)
    return;
  send_to_char("Banzai! To the rescue...\n\r", ch);
  if(!IS_NPC(victim))
    act("You are rescued by $N, you are confused!",FALSE,victim,0,ch,TO_CHAR);
  act("$n heroically rescues $N.", FALSE, ch, 0, victim, TO_NOTVICT);
  if(victim->specials.fighting == tmp_ch)
    stop_fighting(victim);
  if(tmp_ch->specials.fighting)
    stop_fighting(tmp_ch);
  if(ch->specials.fighting)
    stop_fighting(ch);
  set_fighting(ch, tmp_ch);
  set_fighting(tmp_ch, ch);
  if(IS_NPC(ch)){
    GET_HIT(victim)>>=1;
    GET_HIT(ch)+=GET_HIT(victim);
    return;
  }
  WAIT_STATE(ch, PULSE_VIOLENCE);
  WAIT_STATE(victim, PULSE_VIOLENCE);
}
void do_rescue(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim, *tmp_ch;
  int percent;
  char victim_name[240];
  char buf[240];

  one_argument(argument, victim_name);

  if (!(victim = get_char_room_vis(ch, victim_name))) {
    send_to_char("Who do you want to rescue?\n\r", ch);
    return;
  }
  if (victim == ch) {
    send_to_char("What about fleeing instead?\n\r", ch);
    return;
  }
  if(!IS_NPC(ch) && IS_NPC(victim)){
    send_to_char("But why?\n\r",ch);
    return;
  }
  if (ch->specials.fighting == victim) {
    send_to_char("How can you rescue someone you are trying to kill?\n\r",ch);
    return;
  }
  for (tmp_ch=world[ch->in_room].people; tmp_ch &&
      (tmp_ch->specials.fighting != victim); tmp_ch=tmp_ch->next_in_room)  ;
  if (!tmp_ch) {
    act("But nobody is fighting $M?", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }
  if((GET_LEVEL(victim)-GET_LEVEL(ch)) > 50){
    send_to_char("Try rescuing someone more your size!\n\r",ch);
    return;
  }
  if(!IS_NPC(victim)){
    if(victim->desc)
      percent=number(1,100+((victim->desc->wait)<<2));
    else
      percent=50;
    if (percent > ch->skills[SKILL_RESCUE].learned) {
      send_to_char("You fail the rescue.\n\r", ch);
      return;
    }
  }
  rescuesub(ch,victim);
}

void do_kick(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char name[256], buf[256];
  int percent;

  one_argument(argument, name);

  if((! *name) || !(victim = get_char_room_vis(ch, name))) {
    if (ch->specials.fighting) {
      victim = ch->specials.fighting;
    } else {
      send_to_char("Kick who?\n\r", ch);
      return;
    }
  }
  if (victim == ch) {
    send_to_char("Aren't we funny today...\n\r", ch);
    return;
  }
  if(IS_AFFECTED(ch, AFF_CHARM) && (ch->master==victim)){
    send_to_char("Kick your master?\n\r",ch);
    return;
  }
  if(IS_SET(world[ch->in_room].room_flags,LAWFUL)){
    send_to_char("Not here.\n\r",ch);
    return;
  }
  percent=number(1,100)+(GET_AC(victim)>>1)-GET_HITROLL(ch);
  if (percent < ch->skills[SKILL_KICK].learned) {
    damage(ch,victim,2+GET_LEVEL(ch),SKILL_KICK);
  } else {
    damage(ch,victim,0,SKILL_KICK);
  }
  WAIT_STATE(ch, PULSE_VIOLENCE<<1);
  if(IS_AFFECTED(ch, AFF_CHARM) && ch->master)
    WAIT_STATE(ch->master, PULSE_VIOLENCE<<1);
}
void shoot(struct char_data *ch, struct char_data *victim, int type)
{
  struct obj_data *obj;
  byte percent;
  int dam;

  if(!(obj=ch->equipment[HOLD]))
    return;
  if(victim == ch){
    if(!IS_NPC(ch))
      send_to_char("Shoot yourself? Nah...\n\r", ch);
    return;
  }
  if(GET_ITEM_TYPE(obj)!=ITEM_FIREWEAPON) {
    if(!IS_NPC(ch))
      send_to_char("To shoot, you need to HOLD a firing weapon.\n\r",ch);
    return;
  }
  if(obj->obj_flags.value[0] <= 0){
    if(!IS_NPC(ch))
      send_to_char("Oops.  Nothing to shoot.\n\r",ch);
    act("Hmmm.  $n fires an empty $p.",
      FALSE, ch, obj,0,TO_ROOM);
    return;
  }
  if(IS_AFFECTED(ch,AFF_CHARM) && ch->master)
    victim = ch->master;
  if((GET_LEVEL(ch) < IMO)&&(!IS_NPC(ch)))
    obj->obj_flags.value[0]--;
  if((!IS_NPC(ch)) && (GET_LEVEL(ch)>=(IMO+1)))
    percent=0;
  else
    percent=number(1,101)+GET_LEVEL(victim)-GET_LEVEL(ch);
  if(percent < (IS_NPC(ch) ? 100 : ch->skills[SKILL_SHOOT].learned)){
    act("Thwapp! You shoot $M with $p.",
      FALSE, ch, obj,victim,TO_CHAR);
    act("Thwapp! You are shot by $n with $p.",
      FALSE, ch, obj,victim,TO_VICT);
    act("Thwapp! $n shoots $N with $p.",
      FALSE, ch, obj,victim,TO_NOTVICT);
    dam=obj->obj_flags.value[2];
    damage(ch, victim, dam, TYPE_SHOOT);
  } else {
    act("You try to shoot $M with $p, but miss.",
      FALSE, ch, obj,victim,TO_CHAR);
    act("You are shot at by $n with $p, but missed.",
      FALSE, ch, obj,victim,TO_VICT);
    act("$n tries to shoot $N with $p, but misses.",
      FALSE, ch, obj,victim,TO_NOTVICT);
    if(!victim->specials.fighting){
      act("$n says 'Hey, that wasn't nice!'",TRUE,victim,0,0,TO_ROOM);
      hit(victim,ch,TYPE_UNDEFINED);
    }
  }
  if(IS_NPC(ch))
    return;
  if(GET_LEVEL(ch) < (IMO+1))
    WAIT_STATE(ch, PULSE_VIOLENCE*2);
  WAIT_STATE(victim, PULSE_VIOLENCE*2);
}

void do_attack(struct char_data *ch, struct char_data *victim, int type)
{
  struct char_data *tch, *ttch;
  int percent, x;

  if(IS_NPC(ch))
    return;
  if(IS_SET(world[ch->in_room].room_flags,LAWFUL)){
    send_to_char("Not here.\n\r",ch);
    return;
  }
  x = ch->skills[SKILL_ATTACK].learned + GET_DEX(ch);
  for(tch=world[ch->in_room].people;tch;tch=ttch){
    ttch=tch->next_in_room;
    if(IS_NPC(tch)){
      percent=number(1,100);
      if((GET_DEX(tch)+percent) < x){
        act("$n attacks $N.",FALSE,ch,0,tch,TO_ROOM);
        act("You attack $N.",FALSE,ch,0,tch,TO_CHAR);
        hit(ch,tch,TYPE_UNDEFINED);
      } else {
        act("$N beats $n to the attack.",FALSE,ch,0,tch,TO_ROOM);
        act("$N hits you first.",FALSE,ch,0,tch,TO_CHAR);
        hit(tch,ch,TYPE_UNDEFINED);
      }
    }
    if(GET_HIT(ch) <= 0)
      return;
  }
  if(GET_LEVEL(ch) < IMO)
    WAIT_STATE(ch, PULSE_VIOLENCE);
}

void do_shoot(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char name[256], buf[256];

  one_argument(argument, name);
  if(!(victim = get_char_room_vis(ch, name))) {
    if(ch->specials.fighting){
      victim = ch->specials.fighting;
    } else {
      send_to_char("Shoot whom?\n\r", ch);
      return;
    }
  }
  if((GET_LEVEL(victim) >= IMO)&&(!IS_NPC(victim))){
    if(!IS_NPC(ch))
      send_to_char("Shoot an immortal?  Never.\n\r",ch);
    return;
  }
  shoot(ch,victim,TYPE_SHOOT);
  if(IS_AFFECTED(ch, AFF_CHARM) && ch->master)
    WAIT_STATE(ch->master, PULSE_VIOLENCE*3);
}
void do_assist(struct char_data *ch, char *argument, int cmd)
{
  static char arg[MAX_STRING_LENGTH];
  struct char_data *pal;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("Assist who?\n\r", ch);
  } else {
    if (!(pal = get_char_room_vis(ch, arg)))
       send_to_char("They aren't here.\n\r", ch);
    else if (ch == pal)
       send_to_char("You're already helping yourself.\n\r", ch);
    else if (!pal->specials.fighting)
       send_to_char("Help with what?\n\r",ch);
    else if (IS_NPC(pal))
       send_to_char("Stick to helping players.\n\r",ch);
    else if(!CAN_SEE(ch, pal->specials.fighting))
       send_to_char("Can't see any such thing.\n\r",ch);
    else 
       hit(ch, pal->specials.fighting, TYPE_UNDEFINED);
  }
}
void do_grescue(struct char_data *ch, char *argument, int cmd)
{
  int percent;
  char buf[240];
  struct char_data *vict, *tmp, *k, *leader;
  struct follow_type *f;

  if(IS_NPC(ch))
    return;
  if(!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("You don't belong to a group!\n\r", ch);
    return;
  }
  if(ch->master)
    leader = ch->master;
  else
    leader = ch;
  for(f=leader->followers; f; f=f->next){
    k=f->follower;
    if(k==ch) continue;
    if(k->in_room != ch->in_room) continue;
    if(!IS_AFFECTED(k,AFF_GROUP)) continue;
    vict=k->specials.fighting;
    if(vict){
      if(vict->specials.fighting != k) continue;
      percent=number(1,101);
      if(percent > ch->skills[SKILL_GR_RESCUE].learned){
        sprintf(buf,"You fail on %s.\n\r",GET_NAME(k));
        send_to_char(buf,ch);
        return;
      }
      sprintf(buf,"You rescue %s.\n\r",GET_NAME(k));
      send_to_char(buf,ch);
      act("You are group-rescued by $N, what the heck?",FALSE,k,0,ch,TO_CHAR);
      act("$n group-rescues $N.", FALSE, ch, 0, k, TO_NOTVICT);
      stop_fighting(k);
      stop_fighting(vict);
      tmp=ch->specials.fighting;
      if(tmp!=vict){
        if(tmp)
          stop_fighting(ch);
        set_fighting(ch,vict);
      }
      set_fighting(vict,ch);
      WAIT_STATE(k,2*PULSE_VIOLENCE);
    }
    WAIT_STATE(ch, PULSE_VIOLENCE);
  }
}
void do_detonate(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  char arg1[128],arg2[128];
  struct obj_data *bomb;
  int t;

  argument_interpreter(argument, arg1, arg2);
  if(! *arg1){
    send_to_char("Detonate what?\n\r",ch);
    return;
  }
  bomb=get_obj_in_list_vis(ch, arg1,ch->carrying);
  if(!bomb){
    send_to_char("Can't seen to find that!\n\r",ch);
    return;
  }
  if(GET_ITEM_TYPE(bomb) != ITEM_BOMB){
    send_to_char("You can't detonate that!\n\r",ch);
    return;
  }
  if(!IS_SET(bomb->obj_flags.extra_flags,ITEM_POOF)){
    send_to_char("This bomb is already known to be a DUD!\n\r",ch);
    return;
  }
  if(!*arg2){
    send_to_char("Detonate it in how many ticks?\n\r",ch);
    return;
  }
  if(number(1,101) > ch->skills[SKILL_BOMB].learned){
    act("Fssssssss...POP! $n's $p is a dud.",FALSE,ch,bomb,0,TO_ROOM);
    send_to_char("Drat! A dud.\n\r",ch);
    REMOVE_BIT(bomb->obj_flags.extra_flags,ITEM_POOF);
    return;
  }
  t=atoi(arg2);
  bomb->obj_flags.timer=
    (number(1,101) < ch->skills[SKILL_BOMB].learned) ? t : 0;
  SET_BIT(bomb->obj_flags.extra_flags,ITEM_POOFSOON);
  act("You just ignited $p.",FALSE,ch,bomb,0,TO_CHAR);
  act("$n just ignited $p.",FALSE,ch,bomb,0,TO_ROOM);
}
