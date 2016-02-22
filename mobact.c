/* ************************************************************************
*  file: mobact.c , Mobile action module.                 Part of DIKUMUD *
*  Usage: Procedures generating 'intelligent' behavior in the mobiles.    *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************* */

#include <stdio.h>

#include "utils.h"
#include "structs.h"
#include "db.h"
#include "comm.h"
#include "spells.h"

extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;

void hit(struct char_data *ch, struct char_data *victim, int type);
char *strdup(char *source);

int check4enemies(struct char_data *ch, int type)
{
  struct char_data *tmp;

  if(!ch->player.title) return(0);
  if(IS_SET(world[ch->in_room].room_flags,LAWFUL))
    return;
  for(tmp=world[ch->in_room].people; tmp; tmp = tmp->next_in_room){
    if(!IS_NPC(tmp) && !strcmp(ch->player.title,GET_NAME(tmp))){
      switch(type){
        case 1:
          act("$n screams 'Ah, it's $N!",FALSE,ch,0,tmp,TO_ROOM);
          hit(ch,tmp,TYPE_UNDEFINED);
          break;
        case 2:
          hit(ch,tmp,SKILL_BACKSTAB);
          break;
      }
      return(1);
    }
  }
  if((GET_LEVEL(ch) > (IMO>>1)) && !IS_SET(ch->specials.act,ACT_AGGRESSIVE)){
    tmp=(struct char_data *)get_char_vis(ch,ch->player.title);
    if(tmp && !IS_NPC(tmp) && (GET_LEVEL(tmp) >= MIN(250,GET_LEVEL(ch))))
      if(!IS_SET(world[tmp->in_room].room_flags,LAWFUL))
        teleport(ch,tmp->in_room);
  }
  return(0);
}


void mobile_activity(void)
{
 register struct char_data *ch;
 struct descriptor_data *d;
 struct char_data *tmp_ch,*cho_ch,*vict, *p, *nextchar;
 struct obj_data *obj, *best_obj, *worst_obj;
 int t,door, found, max, min, tarlev;
 int minac,minhp;
 char buf[128];
 void do_move(struct char_data *ch, char *argument, int cmd);
 void do_get(struct char_data *ch, char *argument, int cmd);

 for (ch = character_list; ch; ch = nextchar){
  nextchar=ch->next;
  if(IS_MOB(ch)){
   if (IS_SET(ch->specials.act, ACT_SPEC)) {
    if (!mob_index[ch->nr].func) {
     sprintf(buf,"Attempting to call missing MOB func for %s.",GET_NAME(ch));
     log(buf);
     REMOVE_BIT(ch->specials.act, ACT_SPEC);
    } else {
      if((*mob_index[ch->nr].func) (ch, 0, ""))
        continue;
    }
   }
   if(IS_SET(ch->specials.act, ACT_HEALER)){
     if(IS_AFFECTED(ch, AFF_POISON)){
       spell_remove_poison(GET_LEVEL(ch),ch,ch,0);
       spell_heal(GET_LEVEL(ch),ch,ch,0);
     } else if(IS_AFFECTED(ch, AFF_BLIND)){
       spell_heal(GET_LEVEL(ch),ch,ch,0);
     } else if(ch->specials.fighting) {
       spell_cure_critic(GET_LEVEL(ch),ch,ch,0);
     } else if((GET_LEVEL(ch) >= (IMO/2))&&(GET_HIT(ch) < GET_MAX_HIT(ch))) {
       spell_heal(GET_LEVEL(ch),ch,ch,0);
     }
   }
   if(vict=ch->specials.fighting){
     if(IS_SET(ch->specials.act,ACT_GUARD))
       found=cityguard(ch,0,0);
     if(IS_SET(ch->specials.act,ACT_CASTER))
       found=magic_user(ch,0,0);
     if(IS_SET(ch->specials.act,ACT_KICKER))
       found=kickbasher(ch,0,0);
     if(IS_SET(ch->specials.act, ACT_HUNTER)){
       if(!IS_NPC(vict)){
         if(!ch->player.title){
           ch->player.title = strdup(GET_NAME(vict));
         } else if(str_cmp(ch->player.title,GET_NAME(vict))){
           free(ch->player.title);
           ch->player.title = strdup(GET_NAME(vict));
         }
       }
     }
     if((vict=ch->specials.fighting) && IS_NPC(vict)){
       if(IS_AFFECTED(vict,AFF_CHARM) && (vict->master)){
         if(ch->in_room == vict->master->in_room){
           if(number(1,3)==2){
             stop_fighting(ch);
             hit(ch,vict->master,TYPE_UNDEFINED);
           }
         } else {
           stop_fighting(ch);
           stop_fighting(vict);
           act("$n and $N shake hands warmly.",TRUE,ch,0,vict,TO_ROOM);
         }
       }
     }
     if(IS_SET(ch->specials.act, ACT_SMART)){
       if((vict=ch->specials.fighting)&&IS_NPC(vict)){
         if(GET_HIT(vict) > GET_HIT(ch))
           do_flee(ch);
       } else {
         vict=0;
         minhp=0;  /* minhp is actually max level now */
         for(tmp_ch=world[ch->in_room].people;tmp_ch;
           tmp_ch=tmp_ch->next_in_room)
           if(!IS_NPC(tmp_ch))
             if((tmp_ch->specials.fighting==ch)&&((t=GET_LEVEL(tmp_ch))>minhp)){
               minhp=t; vict=tmp_ch;
             }
         if(vict && (vict != ch->specials.fighting)&&
           (GET_POS(vict) > POSITION_DEAD)){
           act("$n decides that $N is a juicier target.",
             FALSE,ch,0,vict,TO_ROOM);
           stop_fighting(ch);
           hit(ch,vict,TYPE_UNDEFINED);
         }
       }
     }
   } else if(AWAKE(ch) && !(ch->specials.fighting)) {
    if(IS_SET(ch->specials.act, ACT_DUMPER)){
     t=number(1,200);
     if(t < 5){
      obj = read_object(1298,VIRTUAL);
      if(obj){
        obj_to_room(obj,ch->in_room);
        act("$n takes a dump.",TRUE,ch,0,0,TO_ROOM);
        obj->obj_flags.value[0]=t;
        if(GET_LEVEL(ch) < IMO)
          teleport(ch,0);
      }
     }
    }
    if(IS_SET(ch->specials.act, ACT_HUNTER)){
     if(ch->player.title)
      check4enemies(ch,1);
    }
    if(IS_SET(ch->specials.act, ACT_SCAVENGER)) {
     if(world[ch->in_room].contents && !number(0,10)) {
      for(max = 1, best_obj = 0, obj = world[ch->in_room].contents;
            obj; obj = obj->next_content) {
       if(CAN_GET_OBJ(ch, obj)) {
        if(obj->obj_flags.cost > max) {
         best_obj = obj;
         max = obj->obj_flags.cost;
        }
       }
      } /* for */
      if (best_obj) {
       obj_from_room(best_obj);
       obj_to_char(best_obj, ch);
       act("$n gets $p.",FALSE,ch,best_obj,0,TO_ROOM);
       sprintf(buf,"%s",best_obj->name);
       if(GET_ITEM_TYPE(best_obj) == ITEM_WEAPON)
         do_wield(ch,buf,0);
       else if(best_obj->obj_flags.cost > 0)
         do_wear(ch,buf,0);
      }
     }
    } /* Scavenger */
    if (!IS_SET(ch->specials.act, ACT_SENTINEL) && 
     (GET_POS(ch) == POSITION_STANDING) &&
     ((door = number(0, 47)) <= 5) && CAN_GO(ch,door) &&
     !IS_SET(world[EXIT(ch, door)->to_room].room_flags, NO_MOB)) {
     if (ch->specials.last_direction == door) {
      ch->specials.last_direction = -1;
     } else {
      if (!IS_SET(ch->specials.act, ACT_STAY_ZONE)) {
       ch->specials.last_direction = door;
       do_move(ch, "", ++door);
      } else {
       if (world[EXIT(ch, door)->to_room].zone == world[ch->in_room].zone) {
        ch->specials.last_direction = door;
        do_move(ch, "", ++door);
       }
      }
     }
    } /* if SENTINEL */
    if (IS_AFFECTED(ch,AFF_CHARM) && (ch->master)){
     if(ch->master->specials.fighting && !ch->specials.fighting){
      if(ch->in_room == ch->master->in_room){
       act("$n comes to the aid of $N.",1,ch,0,ch->master,TO_ROOM);
       hit(ch,ch->master->specials.fighting,0);
      }
     }
    } else if (IS_SET(ch->specials.act,ACT_AGGRESSIVE)) {
     found = FALSE;
     for(tmp_ch = world[ch->in_room].people; tmp_ch;
          tmp_ch = tmp_ch->next_in_room) {
      if(IS_AFFECTED(tmp_ch,AFF_PROTECT_EVIL) &&
         (GET_ALIGNMENT(ch) < -350) && number(0,16))
        continue;
      if(!IS_NPC(tmp_ch) && CAN_SEE(ch, tmp_ch) && (GET_LEVEL(tmp_ch)<IMO)) {
       if (!IS_SET(ch->specials.act, ACT_WIMPY) || !AWAKE(tmp_ch)) {
        if(!found){
         cho_ch=tmp_ch;
         found = TRUE;
        } else {
         if(number(1,7) <= 3)
           cho_ch=tmp_ch;
        } /* else */
       } /* if IS_SET */
      } /* if IS_NPC */
     } /* for */
     if(found)
       hit(ch, cho_ch, 0);
    } else if (IS_SET(ch->specials.act,ACT_SEMIAGGR)) {
     found = FALSE;
     tarlev = GET_LEVEL(ch);
     for(tmp_ch=world[ch->in_room].people;tmp_ch;tmp_ch=tmp_ch->next_in_room){
      if(GET_LEVEL(tmp_ch) >= tarlev){
       if(!IS_NPC(tmp_ch) && CAN_SEE(ch, tmp_ch) && (GET_LEVEL(tmp_ch)<IMO)) {
        if(!found){
         cho_ch=tmp_ch;
         found = TRUE;
        } else {
         if(number(1,7) <= 4)
           cho_ch=tmp_ch;
        }
       }
      }
     }
     if(found)
       hit(ch, cho_ch, 0);
    } else if (IS_SET(ch->specials.act,ACT_HELPER)) {
     for(tmp_ch=world[ch->in_room].people;tmp_ch;tmp_ch=tmp_ch->next_in_room){
      if(vict=tmp_ch->specials.fighting){
       if(IS_NPC(tmp_ch) && (GET_LEVEL(vict) >= MIN(250,GET_LEVEL(ch))) &&
         CAN_SEE(ch,tmp_ch) && CAN_SEE(ch,vict) && (!IS_NPC(vict))){
           if((number(1,7)==3)&&
              (!IS_SET(world[ch->in_room].room_flags,LAWFUL))){
             act("$n decides to assist $N.",1,ch,0,tmp_ch,TO_ROOM);
             hit(ch,vict,0);
             break;
           }
       }
      }
     } /* for */
    } /* is HELPER */
   } /* If AWAKE(ch)   */
  }   /* If IS_MOB(ch)  */
 }
}
