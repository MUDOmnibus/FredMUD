/* ************************************************************************
*  file: limits.c , Limit and gain control module.        Part of DIKUMUD *
*  Usage: Procedures controling gain and limit.                           *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************* */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#include "structs.h"
#include "limits.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "db.h"

char *titles[]={
  "the Dog", "the Donkey", "the Cow", "the Pig", "the Chicken",
   "the Horse", "the Squirrel", "the Mouse", "the Goat", "the Turkey",
   "the Mole", "the Rat", "the Goose", "the Kangaroo", "the Snake"
};

#define READ_TITLE(ch) titles[number(0,14)]

extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct room_data *world;
extern struct index_data *mob_index;

/* External procedures */

void update_pos( struct char_data *victim );                 /* in fight.c */
void damage(struct char_data *ch, struct char_data *victim,  /*    do      */
            int damage, int weapontype);
struct time_info_data age(struct char_data *ch);

int mana_limit(struct char_data *ch)
{
  return(ch->points.max_mana + 1);
}
int hit_limit(struct char_data *ch)
{
  return (ch->points.max_hit + 1);
}
int move_limit(struct char_data *ch)
{
  return (ch->points.max_move + 1);
}
int mana_gain(struct char_data *ch)
{
  int n,gain;

  if(IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = (GET_CON(ch)+GET_WIS(ch))/3;
    switch (GET_POS(ch)) {
      case POSITION_SLEEPING:
        gain += gain;
        if(GET_CON(ch) > 100)
          gain+=GET_CON(ch);
        if(IS_AFFECTED(ch,AFF_HYPERREGEN))
          gain<<=1;
        break;
      case POSITION_RESTING:
        gain+= (gain>>1);  /* Divide by 2 */
        break;
      case POSITION_SITTING:
        gain += (gain>>2); /* Divide by 4 */
        break;
    }
    if((GET_COND(ch,FULL)==0)||(GET_COND(ch,THIRST)==0))
      gain >>= 2;
    else if((GET_COND(ch,DRUNK)) >= 8)
      gain <<= 1;
    if(IS_AFFECTED(ch,AFF_POISON))
      gain = GET_MAX_MANA(ch)/(-336);
  }
  return (gain);
}


int hit_gain(struct char_data *ch)
/* Hitpoint gain pr. game hour */
{
  int gain, n;

  if(IS_NPC(ch)) {
    gain = GET_LEVEL(ch);
    if(IS_AFFECTED(ch,AFF_REGEN))
      gain <<= 2;
  } else {
    gain = (GET_STR(ch) + GET_CON(ch))/5;
    switch (GET_POS(ch)) {
      case POSITION_SLEEPING:
        gain += (gain>>1);
        if(GET_CON(ch) > 90)
          gain+=GET_CON(ch)-90;
        if(IS_AFFECTED(ch,AFF_HYPERREGEN))
          gain<<=1;
        break;
      case POSITION_RESTING:
        gain+= (gain>>2);
        break;
      case POSITION_SITTING:
        gain += (gain>>3);
        break;
    }
    if((GET_COND(ch,FULL)==0)||(GET_COND(ch,THIRST)==0))
      gain >>= 2;
    else if((GET_COND(ch,DRUNK)) >= 8)
      gain <<= 1;
    if(IS_AFFECTED(ch,AFF_POISON))
      gain = GET_MAX_HIT(ch)/(-168);
  }
  if(IS_AFFECTED(ch,AFF_REGEN))
    gain <<= 1;
  return (gain);
}

int move_gain(struct char_data *ch)
{
  int gain;

  if(IS_NPC(ch)) {
    return(GET_LEVEL(ch));  
  } else {
    gain = (GET_DEX(ch) + GET_CON(ch))/2;
    switch (GET_POS(ch)) {
      case POSITION_SLEEPING:
        gain += (gain>>1); /* Divide by 2 */
        break;
      case POSITION_RESTING:
        gain+= (gain>>2);  /* Divide by 4 */
        break;
      case POSITION_SITTING:
        gain += (gain>>3); /* Divide by 8 */
        break;
    }
  }

  if (IS_AFFECTED(ch,AFF_POISON))
    gain >>= 2;

  if((GET_COND(ch,FULL)==0)||(GET_COND(ch,THIRST)==0))
    gain >>= 2;

  return (gain);
}


/* Gain maximum in various points */
void advance_level(struct char_data *ch)
{
  int addx;

  addx = (GET_CON(ch)/8);
  addx += ((GET_LEVEL(ch) < 2) ? 2 : number(1,3));
  ch->points.max_hit += MAX(1, addx);
  addx = (GET_INT(ch)/8);
  addx += ((GET_LEVEL(ch) < 2) ? 2 : number(1,3));
  ch->points.max_mana += MAX(1, addx);
  addx = (GET_DEX(ch)/8);
  addx += 8;
  ch->points.max_move += MAX(1, addx);
  ch->specials.spells_to_learn += (9+GET_WIS(ch))/10;
}  

void set_title(struct char_data *ch)
{
  char *s;

  s=READ_TITLE(ch);
  if (GET_TITLE(ch))
    RECREATE(GET_TITLE(ch),char,strlen(s)+1);
  else
    CREATE(GET_TITLE(ch),char,strlen(s)+1);
  strcpy(GET_TITLE(ch), s);
}
void gain_exp(struct char_data *ch, int gain)
{
  int t, confactor;


  if(gain > 0){
   
/* SLUG_CHANGE 11-9-96  */
    ch->specials.metadata[ch->specials.metadata[MAX_META_SAMPLES]] += gain;
    t=GET_EXP(ch);
    if((t > 0)&&((t+gain) < 0)){
      send_to_char("Hey dogbreath, your experience is at the maximum!\n\r",ch);
      GET_EXP(ch) = 2147000000;
      return;
    }
  }
  if(IS_NPC(ch) || ((GET_LEVEL(ch)<IMO) && (GET_LEVEL(ch) > 0))) {
    if(gain > 0){ 
      GET_EXP(ch) += gain;
      if(!IS_NPC(ch) && IS_SET(ch->specials.act,PLR_AUTOCNVRT))
        if(GET_EXP(ch) >= 1000000){
          GET_META(ch) += GET_EXP(ch)/1000000;
          GET_EXP(ch)  %= 1000000;
        }
    } else if (gain < 0) {
      gain = abs(gain);
      if(GET_EXP(ch) > gain)
        GET_EXP(ch) -= gain;
      else
        GET_EXP(ch) = 0;
    }
  }
}
void gain_condition(struct char_data *ch,int condition,int value)
{
  bool intoxicated;

  intoxicated=(GET_COND(ch, DRUNK) > 0);
  GET_COND(ch,condition) += value;
  GET_COND(ch,condition) = MAX(0,GET_COND(ch,condition));
  GET_COND(ch,condition) = MIN(GET_GUT(ch),GET_COND(ch,condition));
  if(GET_COND(ch,condition))
    return;

  switch(condition){
    case FULL :
    {
      send_to_char("You are hungry.\n\r",ch);
      return;
    }
    case THIRST :
    {
      send_to_char("You are thirsty.\n\r",ch);
      return;
    }
    case DRUNK :
    {
      if(intoxicated)
        send_to_char("You are now sober.\n\r",ch);
      return;
    }
    default : break;
  }
}


void check_idling(struct char_data *ch)
{
  if (++(ch->specials.timer) > 8)
    if (ch->specials.was_in_room == NOWHERE && ch->in_room != NOWHERE) {
      ch->specials.was_in_room = ch->in_room;
      if (ch->specials.fighting) {
        stop_fighting(ch->specials.fighting);
        stop_fighting(ch);
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You have been idle, and are pulled into a void.\n\r", ch);
      char_from_room(ch);
      char_to_room(ch, 1);  /* Into room number 0 */
    } else if (ch->specials.timer > 99) {
      if (ch->desc)
        close_socket(ch->desc);
      do_rent(ch,0,0);
    }
}

/* Update both PC's & NPC's and objects*/
void point_update( void )
{  
  void update_char_objects( struct char_data *ch ); /* handler.c */
  void extract_obj(struct obj_data *obj); /* handler.c */
  struct char_data *i, *ni, *next_dude, *vict;
  struct obj_data *j, *next_thing, *jj, *next_thing2, *jjj;
  char *adp;
  int d1,d2,d3;
  int bdam,door,newroom;

  /* characters */
  for (i = character_list; i; i = next_dude) {
    next_dude = i->next;
    if (GET_POS(i) > POSITION_STUNNED) {
      d1=hit_limit(i)-GET_HIT(i);
      if(d1 > 0){
        d2=hit_gain(i);
        d3=MIN(d1,d2);
        GET_HIT(i) += d3;
      }
      d1=mana_limit(i)-GET_MANA(i);
      if(d1 > 0){
        d2=mana_gain(i);
        d3=MIN(d1,d2);
        GET_MANA(i) += d3;
        GET_COND(i,FULL) = MAX(0,GET_COND(i,FULL)-d3/100);
      }
      d1=move_limit(i)-GET_MOVE(i);
      if(d1 > 0){
        d2=move_gain(i);
        d3=MIN(d1,d2);
        GET_MOVE(i) += d3;
        GET_COND(i,THIRST) = MAX(0,GET_COND(i,THIRST)-d3/100);
      }
    } else if (GET_POS(i) == POSITION_STUNNED) {
      GET_HIT(i)  = MIN(GET_HIT(i)  + hit_gain(i),  hit_limit(i));
      GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), mana_limit(i));
      GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), move_limit(i));
    } else if  (GET_POS(i) == POSITION_INCAP) {
      damage(i, i, 1, TYPE_SUFFERING);
    } else if (!IS_NPC(i) && (GET_POS(i) == POSITION_MORTALLYW))
      damage(i, i, 2, TYPE_SUFFERING);
    if (!IS_NPC(i)) {
      update_char_objects(i);
      if (GET_LEVEL(i) < (IMO+1)){
        update_pos(i);
        check_idling(i);
      }
    }
    gain_condition(i,FULL,-1);
    gain_condition(i,DRUNK,-1);
    gain_condition(i,THIRST,-1);
    update_pos(i);
    if(GET_POS(i) == POSITION_DEAD) die(i);
 /* SLUG_CHANGE 11-9-96 Increment front of queue index */   
    i->specials.metadata[MAX_META_SAMPLES]++;
    i->specials.metadata[MAX_META_SAMPLES] %= MAX_META_SAMPLES; 
    i->specials.metadata[i->specials.metadata[MAX_META_SAMPLES]] = 0;
/* SLUG_CHANGE 11-13-96 increment char tic counter */
    i->specials.connect_tics++;
  } /* for */
  /* objects */
  for(j = object_list; j ; j = next_thing){
    next_thing = j->next; /* Next in object list */
    if(IS_SET(j->obj_flags.extra_flags,ITEM_POOFSOON)){
      if(j->obj_flags.timer)
       --j->obj_flags.timer;
      if(j->obj_flags.timer <= 0){
        if(GET_ITEM_TYPE(j) == ITEM_BOMB) {
          if(i=j->carried_by){
            act("A bomb in your inventory explodes, OUCH!!",
              FALSE,i,j,0,TO_CHAR);
            act("A bomb carried by $n EXPLODES, causing bodily harm!!",
              FALSE,i,j,0,TO_NOTVICT);
            if(!IS_SET(world[i->in_room].room_flags,LAWFUL))
            if(IS_NPC(i) || (GET_LEVEL(i) < IMO)){
              GET_HIT(i) -= MIN(j->obj_flags.value[0],GET_HIT(i)-1);
              update_pos(i);
            }
            extract_obj(j);
          } else if (j->in_room != NOWHERE) {
            if(!IS_SET(world[j->in_room].room_flags,LAWFUL)){
              for(i=world[j->in_room].people;i;i=ni){
                ni=i->next_in_room;
                if(IS_NPC(i)){
                  if(IS_SET(i->specials.act,ACT_SMART)){
                    if(GET_POS(i) < POSITION_STANDING)
                      do_stand(i,0,0);
                    if(j->obj_flags.value[0] > (GET_HIT(i)>>4)){
                      if(number(0,4))
                        defuser(i,0,0);
                      else
                        do_flee(i,0,0);
                    }
                  }
                }
              }
              for(i=world[j->in_room].people;i;i=ni){
                ni=i->next_in_room;
                if(IS_NPC(i) || (GET_LEVEL(i) < IMO)){
                  if(!IS_NPC(i))
                    send_to_char("You are hurt!\n\r",i);
                  bdam = MIN((j->obj_flags.value[0])>>1,GET_HIT(i)-1);
                  if((jjj=i->equipment[WEAR_SHIELD])&&
                     (jjj->obj_flags.value[1]==SHIELD_BOMB))
                    bdam>>=2;
                  GET_HIT(i) -= bdam;
                  update_pos(i);
                }
              }
              send_to_room("An explosion rocks the room!\n\r",j->in_room);
              for(door=0;door<=5;door++) 
                if((world[j->in_room].dir_option[door])&&
                   ((newroom=world[j->in_room].dir_option[door]->to_room) > 0))
                  send_to_room("You hear a nearby explosion.\n\r",newroom);
            } else {
              send_to_room("Your hear a loud POP!\n\r",j->in_room);
            }
            extract_obj(j);
          }
        } else if (j->carried_by){
          act("It seems that $p has vanished!",
            FALSE,j->carried_by,j,0,TO_CHAR);
          extract_obj(j);
        } else if ((j->in_room != NOWHERE) && (world[j->in_room].people)){
          act("Miraculously, $p vanishes.",
            TRUE, world[j->in_room].people, j, 0, TO_ROOM);
          extract_obj(j);
        }
      }
    } else if((GET_ITEM_TYPE(j)==ITEM_CONTAINER)&&(j->obj_flags.value[3])){
      /* timer count down */
      if (j->obj_flags.timer > 0) j->obj_flags.timer--;
      if (!j->obj_flags.timer) {
        if (j->carried_by)
          act("The $p decays in your hands.",FALSE,j->carried_by,j,0,TO_CHAR);
        else if ((j->in_room != NOWHERE) && (world[j->in_room].people)){
          act("The $p dries up and blows away.",
            TRUE, world[j->in_room].people, j, 0, TO_ROOM);
          act("The $p dries up and blows away.",
            TRUE, world[j->in_room].people, j, 0, TO_CHAR);
        }
        for(jj = j->contains; jj; jj = next_thing2) {
          next_thing2 = jj->next_content; /* Next in inventory */
          obj_from_obj(jj);
          if (j->in_obj)
            obj_to_obj(jj,j->in_obj);
          else if (j->carried_by)
            obj_to_char(jj,j->carried_by);
          else if (j->in_room != NOWHERE)
            obj_to_room(jj,j->in_room);
          else
            assert(FALSE);
        }
        extract_obj(j);
      }
    }
  }
}
void recover_in_rent(struct char_data *ch)
{
  int t,dhp,dma,dmo;
  char buf[256];

  t=(ch->specials.xxx)/15;
  ch->specials.xxx=0;
  sprintf(buf,"You were gone %d ticks.\n\r",t);
  send_to_char(buf,ch);
  if(t < 120) return;
  t<<=2;
  dhp = hit_gain(ch);  if(dhp < 0) dhp=0;
  dma = mana_gain(ch); if(dma < 0) dma=0;
  dmo = move_gain(ch); if(dmo < 0) dmo=0;
  GET_HIT(ch)  = MIN(GET_HIT(ch)  + t*dhp,  hit_limit(ch));
  GET_MANA(ch) = MIN(GET_MANA(ch) + t*dma, mana_limit(ch));
  GET_MOVE(ch) = MIN(GET_MOVE(ch) + t*dmo, move_limit(ch));
  send_to_char("You feel refreshed from your long nap.\n\r",ch);
}


