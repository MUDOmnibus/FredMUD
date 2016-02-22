/* ************************************************************************
*  file: magic.c , Implementation of spells.              Part of DIKUMUD *
*  Usage : The actual effect of magic.                                    *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************* */

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "limits.h"
#include "db.h"

/* Extern structures */

extern int nokillflag;
extern struct room_data *world;
extern struct obj_data  *object_list;
extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;

/* Extern procedures */

void do_look(struct char_data *ch, char *argument, int cmd);
void damage(struct char_data *ch, struct char_data *victim,
            int damage, int weapontype);
bool saves_spell(struct char_data *ch, sh_int spell);
void weight_change_object(struct obj_data *obj, int weight);
char *strdup(char *source);
int dice(int number, int size);
void hit(struct char_data *ch, struct char_data *victim, int type);
void add_follower(struct char_data *ch, struct char_data *leader);

void spell_remove_poison(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);

void banish(struct char_data *vict)
{
  int location;
  extern int top_of_world;

  SET_BIT(vict->specials.act, PLR_BANISHED);
  for(location = 0; location < top_of_world; location++)
    if(world[location].number == 6999)
      break;
  act("$n disappears in a puff of smoke.",FALSE,vict,0,0,TO_ROOM);
  char_from_room(vict);
  char_to_room(vict,location);
  act("$n appears with an ear-splitting bang.",FALSE,vict,0,0,TO_ROOM);
  send_to_char("You smell fire and brimstone?\n\r", vict);
}

/* Offensive Spells */

void spell_magic_missile(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;

  assert(victim && ch);
  dam = number(1,50);
  if ( saves_spell(victim, SAVING_SPELL) )
    dam >>= 1;
  if((obj) && (GET_ITEM_TYPE(obj) == ITEM_WEAPON))
    damage(ch, victim, dam, SPELL_MAGIC_MISSILE_WPN);
  else
    damage(ch, victim, dam, SPELL_MAGIC_MISSILE);
}

void spell_chill_touch(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;
  int dam;

  assert(victim && ch);
  dam = number(1,100);
  if(nokillflag && !IS_NPC(ch) && !IS_NPC(victim))
    return;
  if(IS_AFFECTED(victim,SPELL_CHILL_TOUCH))
    return;
  if ( !saves_spell(victim, SAVING_SPELL) ) {
    af.type      = SPELL_CHILL_TOUCH;
    af.duration  = 6+level/10;
    af.modifier  = -2 - level/100;
    af.location  = APPLY_STR;
    af.bitvector = 0;
    affect_join(victim, &af, TRUE, FALSE);
  } else {
    dam >>= 2;
  }
  damage(ch, victim, dam, SPELL_CHILL_TOUCH);
}

void spell_burning_hands(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;

  assert(victim && ch);
  dam = number(1,200);
  if(saves_spell(victim, SAVING_SPELL))
    dam >>= 1;
  damage(ch, victim, dam, SPELL_BURNING_HANDS);
}

void spell_shocking_grasp(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;

  assert(victim && ch);
  dam = number(1,400);
  if(saves_spell(victim, SAVING_SPELL))
    dam >>= 1;
  else if(obj)
    spell_burning_hands(level,ch,victim,obj);
  damage(ch, victim, dam, SPELL_SHOCKING_GRASP);
}

void spell_lightning_bolt(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;

  assert(victim && ch);
  dam = number(1,800);
  if ( saves_spell(victim, SAVING_SPELL) )
    dam >>= 1;
  if((obj) && (GET_ITEM_TYPE(obj) == ITEM_WEAPON))
    damage(ch, victim, dam, SPELL_BOLT_WPN);
  else
    damage(ch, victim, dam, SPELL_LIGHTNING_BOLT);
}

void spell_colour_spray(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;

  assert(victim && ch);
  dam = number(1,1600);
  if ( saves_spell(victim, SAVING_SPELL) )
    dam >>= 1;
  if((obj) && (GET_ITEM_TYPE(obj) == ITEM_WEAPON))
    damage(ch, victim, dam, SPELL_COLOUR_SPRAY_WPN);
  else
    damage(ch, victim, dam, SPELL_COLOUR_SPRAY);
}

/* Drain XP, MANA, HP - caster gains HP and MANA */
void spell_energy_drain(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam, xp, mana;
  void gain_exp(struct char_data *ch, int gain);

  assert(victim && ch);

  if(IS_SET(world[ch->in_room].room_flags,LAWFUL))
    return;
  if( !saves_spell(victim, SAVING_SUNBURST) ) {
    if (GET_LEVEL(victim) <= 2) {
      damage(ch,victim, 100, SPELL_ENERGY_DRAIN); /* Kill the sucker */
    } else {
      if((GET_LEVEL(ch) < IMO) && !IS_NPC(ch) && !IS_NPC(victim))
        victim = ch;
      xp = number(level>>1,level)*5000;
      gain_exp(victim, -xp);
      dam = dice(1,10);
      mana = GET_MANA(victim)>>1;
      GET_MOVE(victim) >>= 1;
      GET_MANA(victim) = mana;
      GET_MANA(ch) += mana>>1;
      GET_HIT(ch) += dam;
      send_to_char("Your life energy is drained!\n\r", victim);
      damage(ch, victim, dam, SPELL_ENERGY_DRAIN);
    }
  } else {
    damage(ch, victim, 0, SPELL_ENERGY_DRAIN); /* Miss */
    hit(victim,ch,TYPE_UNDEFINED);
  }
}

void spell_fireball(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam,lev;

  assert(victim && ch);
  lev = GET_LEVEL(ch);
  dam = number(1,100+((lev*lev)>>3));
  if (saves_spell(victim, SAVING_SPELL) )
    dam >>= 1;
  if(GET_LEVEL(victim) > (IMO>>1))
    dam >>= 1;
  if((obj) && (GET_ITEM_TYPE(obj) == ITEM_WEAPON))
    damage(ch, victim, dam, SPELL_FIREBALL_WPN);
  else
    damage(ch, victim, dam, SPELL_FIREBALL);
}

void spell_sunburst(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;

  assert(victim && ch);
  dam = number(1,6400);
  if(saves_spell(victim,SAVING_SPELL))
    dam >>= 1;
  if(GET_LEVEL(victim) > (IMO>>1))
    dam >>= 1;
  if((obj) && (GET_ITEM_TYPE(obj) == ITEM_WEAPON))
    damage(ch, victim, dam, SPELL_SUNBURST_WPN);
  else
    damage(ch, victim, dam, SPELL_SUNBURST);
}
void spell_doppelganger(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;

  assert(victim && ch);
  dam = number(1,102400);
  if(saves_spell(victim,SAVING_SPELL))
    dam >>= 1;
  if(GET_LEVEL(victim) > (IMO>>1))
    dam >>= 1;
  damage(ch, victim, dam, SPELL_DOPPELGANGER);
}
void spell_jingle(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;

  assert(victim && ch);
  dam = number(1,100000);
  if(saves_spell(victim,SAVING_SPELL))
    dam >>= 1;
  if(GET_LEVEL(victim) > (IMO>>1))
    dam >>= 1;
  damage(ch, victim, dam, SPELL_JINGLE);
}

void spell_nova(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam,manaloss,rat,tmp,m;
  struct obj_data *sh;

  assert(victim && ch);
  if(IS_NPC(ch)){
    dam=number(1,GET_HIT(ch));
  } else if(GET_LEVEL(ch) < 999){
    dam=number(1,GET_MANA(ch));
    GET_MANA(ch) -= dam;
    if(GET_MANA(ch) < 100)
      GET_MANA(ch)=100;
  } else {
    dam=2000000000;
  }
  if(saves_spell(victim, SAVING_SUNBURST)){
    act("But $N avoids the worst of the nova...",FALSE,ch,0,victim,TO_NOTVICT);
    act("But you avoid the worst of the nova...",FALSE,ch,0,victim,TO_VICT);
    dam >>= 4;
  }
  if((sh=victim->equipment[WEAR_SHIELD])&&
     (sh->obj_flags.value[1]==SHIELD_NOVA)){
    dam >>= 4;
    act("$N's shield deflects most of the nova.",FALSE,ch,0,victim,TO_NOTVICT);
    act("Your shield deflects most of the nova from $n.",
      FALSE, ch, 0, victim, TO_VICT);
  }
  damage(ch, victim, dam, SPELL_NOVA);
}

void spell_dispel_evil(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;

  assert(ch && victim);

  if (IS_EVIL(ch))
    victim = ch;
  else
    if (IS_GOOD(victim)) {
      act("God protects $N.", FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
  if ((GET_LEVEL(victim) < level) || (victim == ch))
    dam = 50;
  else {
    dam = dice(level,5);
    if ( saves_spell(victim, SAVING_SPELL) )
      dam >>= 1;
  }
  if((obj) && (GET_ITEM_TYPE(obj) == ITEM_WEAPON))
    damage(ch, victim, dam, SPELL_DISPEL_EVIL_WPN);
  else
    damage(ch, victim, dam, SPELL_DISPEL_EVIL);
}

void spell_morphia(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;
  struct char_data *tmp;

  assert(victim && ch);
  dam = number(1,12800);
  if ( saves_spell(victim, SAVING_SPELL) )
    dam >>= 2;
  damage(ch, victim, dam, SPELL_MORPHIA);
}

void spell_slime(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;
  struct char_data *tmp;
  struct obj_data *sh;

  assert(victim && ch);
  dam = number(1,25600);
  if((sh=victim->equipment[WEAR_SHIELD])&&
     (sh->obj_flags.value[1]==SHIELD_SLIME)){
    dam >>= 4;
    act("$N's shield deflects most of the slime.",
      FALSE, ch, 0, victim, TO_NOTVICT);
    act("Your shield deflects most of the slime from $n.",
      FALSE, ch, 0, victim, TO_VICT);
  }
  if ( saves_spell(victim, SAVING_SPELL) )
    dam >>= 2;
  if((obj) && (GET_ITEM_TYPE(obj) == ITEM_WEAPON))
    damage(ch, victim, dam, SPELL_SLIME_WPN);
  else
    damage(ch, victim, dam, SPELL_SLIME);
}
void spell_gas(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;
  struct char_data *tmp;
  struct obj_data *sh;

  assert(victim && ch);
  dam = number(1,51200);
  if((sh=victim->equipment[WEAR_SHIELD])&&
     (sh->obj_flags.value[1]==SHIELD_GAS)){
    dam >>= 4;
    act("$N's shield absorbs most of the gas.",
      FALSE, ch, 0, victim, TO_NOTVICT);
    act("Your shield absorbs most of the gas from $n.",
      FALSE, ch, 0, victim, TO_VICT);
  }
  if(saves_spell(victim, SAVING_SPELL) )
    dam >>= 1;
  if((obj) && (GET_ITEM_TYPE(obj) == ITEM_WEAPON))
    damage(ch, victim, dam, SPELL_GAS_WPN);
  else
    damage(ch, victim, dam, SPELL_GAS);
}

void spell_harm(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;
  struct obj_data *sh;

  assert(victim && ch);
  dam = number(1,GET_MOVE(ch));
  if (dam < 0)
    dam = 0;
  else if(!IS_NPC(ch)){
    if(saves_spell(victim,SAVING_SPELL))
      dam >>= 1;
    if(saves_spell(victim,SAVING_SPELL))
      dam >>= 1;
  }
  if((sh=victim->equipment[WEAR_SHIELD])&&
     (sh->obj_flags.value[1]==SHIELD_HARM)){
    dam >>= 2;
    act("$N's shield reduces the harm.", FALSE, ch, 0, victim, TO_NOTVICT);
    act("Your shield reduces the harm.", FALSE, ch, 0, victim, TO_VICT);
  }
  damage(ch, victim, dam, SPELL_HARM);
  GET_MOVE(ch) -= (dam>>1);
}
void spell_barf(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;

  assert(victim && ch);
  dam = GET_COND(ch,THIRST)+GET_COND(ch,FULL);
  if(IS_AFFECTED(ch,AFF_POISON))
    dam <<= 1;
  GET_COND(ch,THIRST) = GET_COND(ch,FULL) = 0;
  damage(ch, victim, dam, SPELL_BARF);
}

void spell_armor(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  assert(victim);
  if (!affected_by_spell(victim, SPELL_ARMOR)) {
    af.type      = SPELL_ARMOR;
    af.duration  = 24;
    af.modifier  = (number(1,25) + (level/50));
    af.location  = APPLY_AC;
    af.bitvector = 0;
    affect_to_char(victim, &af);
    send_to_char("You feel someone protecting you.\n\r", victim);
  }
}

void spell_relocate(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  char buf[MAX_INPUT_LENGTH];
  int location,i;
  struct char_data *pers, *mnt;

  assert(ch);
  if (CAN_SEE(ch,victim)){
    i=level-GET_LEVEL(victim);
    if(i < 0){
      send_to_char("Your spell is not powerful enough for that.\n\r",ch);
      return;
    } else if(number(0,2) > i){
      send_to_char("You make a valiant effort, but barely fail.\n\r",ch);
      return;
    }
    location = victim->in_room;
  } else {
    send_to_char("No such creature around, what a waste.\n\r", ch);
    return;
  }
  if (IS_SET(world[location].room_flags, PRIVATE) ||
      IS_SET(world[location].room_flags, NORELOCATE) ||
      IS_SET(world[location].room_flags, OFF_LIMITS)) {
    send_to_char( "You fail miserably.\n\r", ch);
    return;
  }
  if(IS_SET(victim->specials.act,PLR_NORELO)){
    act("$N seems to be resisting your efforts.",TRUE,ch,0,victim, TO_CHAR);
    return;
  }
  act("$n disappears in a puff of purple smoke.", FALSE,ch,0,0,TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, location);
  act("$n appears with a modest poof.",FALSE,ch,0,0,TO_ROOM);
  do_look(ch,"",15);
}

void spell_bless(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  assert(ch && (victim || obj));

  if (obj) {
    if(GET_POS(ch) != POSITION_FIGHTING){
      SET_BIT(obj->obj_flags.extra_flags, ITEM_BLESS);
      act("$p briefly glows.",FALSE,ch,obj,0,TO_CHAR);
    }
  } else {
    if ((GET_POS(victim) != POSITION_FIGHTING) &&
        (!affected_by_spell(victim, SPELL_BLESS))) {
      send_to_char("You feel righteous.\n\r", victim);
      af.type      = SPELL_BLESS;
      af.duration  = 6;
      af.modifier  = 1+(level/25);
      af.location  = APPLY_HITROLL;
      af.bitvector = 0;
      affect_to_char(victim, &af);
      af.location = APPLY_SAVING_SPELL;
      af.modifier = -1;                 /* Make better */
      affect_to_char(victim, &af);
      if (!affected_by_spell(victim, SPELL_HOLDALIGN))
        GET_ALIGNMENT(victim) = (1000);
    }
  }
}

static int wps[]=
  {2,4,8,16,32,64,128,256,512,1024,2048,4096,16384,131072,262144};
void spell_transmutation(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  assert(ch && obj);

  if (obj) {
    if(IS_OBJ_STAT(obj,ITEM_MUTABLE)){
      obj->obj_flags.wear_flags = 1 + wps[number(0,14)];
      REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_MUTABLE);
      act("$p briefly flashes gold.",FALSE,ch,obj,0,TO_CHAR);
    } else {
      obj->obj_flags.wear_flags = 1;
      act("$p briefly flashes silver.",FALSE,ch,obj,0,TO_CHAR);
    }
  }
}

void spell_blindness(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct obj_data *o;
  struct affected_type af;

  assert(ch && victim);

  if(level < IMO){
    if(!IS_NPC(ch) && !IS_NPC(victim) && 
       !IS_SET(world[ch->in_room].room_flags,PK_ROOM))
      return;
    if(saves_spell(victim, SAVING_SPELL) ||
      affected_by_spell(victim, SPELL_BLINDNESS))
      return;
    if((o=victim->equipment[WEAR_FACE]) && (o->obj_flags.value[2]==5))
      return;
  } else {
    return;
  }
  act("$n seems to be blinded!", TRUE, victim, 0, 0, TO_ROOM);
  send_to_char("You have been blinded!\n\r", victim);
  af.type      = SPELL_BLINDNESS;
  af.location  = APPLY_HITROLL;
  af.modifier  = -(3+(level>>6));  /* Make hitroll worse */
  af.duration  = 1;
  af.bitvector = AFF_BLIND;
  affect_to_char(victim, &af);
  af.location = APPLY_AC;
  af.modifier = -(5+(level>>3)); /* Make AC Worse! */
  affect_to_char(victim, &af);
}

void spell_create_food(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct obj_data *tmp_obj;

  assert(ch);

  CREATE(tmp_obj, struct obj_data, 1);
  clear_object(tmp_obj);

  tmp_obj->name = strdup("mushroom");
  tmp_obj->short_description = strdup("A Magic Mushroom");
  tmp_obj->description = strdup("A really delicious looking magic mushroom lies here.");

  tmp_obj->obj_flags.type_flag = ITEM_FOOD;
  tmp_obj->obj_flags.wear_flags = ITEM_TAKE | ITEM_HOLD;
  tmp_obj->obj_flags.value[0] = 5+level;
  tmp_obj->obj_flags.weight = 1;
  tmp_obj->obj_flags.cost = 10;

  tmp_obj->next = object_list;
  object_list = tmp_obj;

  obj_to_room(tmp_obj,ch->in_room);

  tmp_obj->item_number = -1;

  act("$p suddenly appears.",TRUE,ch,tmp_obj,0,TO_ROOM);
  act("$p suddenly appears.",TRUE,ch,tmp_obj,0,TO_CHAR);
}



void spell_create_water(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int weight;

  assert(ch && obj);

  if(GET_ITEM_TYPE(obj) == ITEM_DRINKCON) {
     weight = obj->obj_flags.value[0] - obj->obj_flags.value[1];
     obj->obj_flags.value[1] += weight;
     weight_change_object(obj, weight);
     act("$p is filled.", FALSE, ch,obj,0,TO_CHAR);
  }
}



void spell_cure_blind(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  assert(victim);

  if (affected_by_spell(victim, SPELL_BLINDNESS)) {
    affect_from_char(victim, SPELL_BLINDNESS);
    send_to_char("Your vision returns!\n\r", victim);
  }
}



void spell_cure_critic(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int healpoints;

  assert(victim);
  if(IS_NPC(ch))
    healpoints = GET_MAX_HIT(ch) / 100;
  else
    healpoints = 20+((level*level)>>7);
  if ( (healpoints + GET_HIT(victim)) > hit_limit(victim) )
    GET_HIT(victim) = hit_limit(victim);
  else
    GET_HIT(victim) += healpoints;
  send_to_char("You feel better!\n\r", victim);
  update_pos(victim);
}

void spell_cure_light(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int healpoints;

  assert(ch && victim);

  healpoints = dice(1,8)+3;

  if ( (healpoints+GET_HIT(victim)) > hit_limit(victim) )
    GET_HIT(victim) = hit_limit(victim);
  else
    GET_HIT(victim) += healpoints;

  update_pos( victim );

  send_to_char("You feel better!\n\r", victim);
}



void spell_curse(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  assert(victim || obj);

  if (obj) {
    SET_BIT(obj->obj_flags.extra_flags, ITEM_HUM);
    SET_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
    if(obj->obj_flags.type_flag == ITEM_WEAPON){
      if(obj->obj_flags.value[2] > 0)
        obj->obj_flags.value[2]--;
      else
        obj->obj_flags.value[2]=0;
    }
    act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
  } else {
    if ( saves_spell(victim, SAVING_SPELL) ||
       affected_by_spell(victim, SPELL_CURSE))
      return;
    af.type      = SPELL_CURSE;
    af.duration  = 24*7;       /* 7 Days */
    af.modifier  = -(1+(level>>6));
    af.location  = APPLY_HITROLL;
    af.bitvector = AFF_CURSE;
    affect_to_char(victim, &af);
    af.location = APPLY_SAVING_PARA;
    af.modifier = 1; /* Make worse */
    affect_to_char(victim, &af);
    if (!affected_by_spell(victim, SPELL_HOLDALIGN))
      GET_ALIGNMENT(victim) = (-1000);
    act("$n briefly reveals a red aura!", FALSE, victim, 0, 0, TO_ROOM);
    act("You feel very uncomfortable.",FALSE,victim,0,0,TO_CHAR);
  }
}

void spell_detect_invisibility(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  assert(victim);
  if ( affected_by_spell(victim, SPELL_DETECT_INVISIBLE) )
    return;
  af.type      = SPELL_DETECT_INVISIBLE;
  af.duration  = level*5;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_DETECT_INVISIBLE;
  affect_to_char(victim, &af);
  send_to_char("Your eyes tingle.\n\r", victim);
}


void spell_infravision(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  assert(victim);
  if ( affected_by_spell(victim, SPELL_INFRAVISION) )
    return;
  af.type      = SPELL_INFRAVISION;
  af.duration  = level*3;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_INFRAVISION;
  affect_to_char(victim, &af);
  send_to_char("Your eyes glow red.\n\r", victim);
}

static char *colors[]={
  "green", "blue", "red", "pink", "purple", "red", "yellow", "orange"
};
static char *awes[]={
  "amazed", "awestruck", "astounded", "impressed", "inspired"
};
void spell_kerplunk(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
 char buf[MAX_STRING_LENGTH];
 int n;

 if((!victim) || (ch==victim)){
  sprintf(buf,"A huge %s cloud envelops $n. You have witnessed a miracle.",
   colors[number(0,7)]);
  act(buf, TRUE,ch,0,0,TO_ROOM);
  act("You do some really cool magical stuff, impressing everyone in the room.",
   TRUE,ch,0,0,TO_CHAR);
  } else {
   sprintf(buf,"$n creates a large %s cloud, which envelops $N. You are %s.",
    colors[n=number(0,7)],awes[number(0,4)]);
   act(buf,TRUE,ch,0,victim,TO_NOTVICT);
   sprintf(buf,"$n does something magical to you. You feel %s.",
    colors[n]);
   act(buf,TRUE,ch,0,victim,TO_VICT);
   act("You do some really cool magical stuff to $N.",TRUE,ch,0,victim,TO_CHAR);
  }
}

#define N_ENCH_LOCS 11

int enchant_locs[]={1,2,3,4,5,12,13,14,17,18,19};

void spell_enchant(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int i;

  assert(ch && obj);

  if(!IS_SET(obj->obj_flags.extra_flags,ITEM_MAGIC)){
    for (i=0; i < MAX_OBJ_AFFECT; i++){
      if (obj->affected[i].location == APPLY_NONE){
        obj->affected[i].location = enchant_locs[number(0,N_ENCH_LOCS-1)];
        obj->affected[i].modifier = 1+number(0,level/100);
        if(IS_GOOD(ch)) {
          SET_BIT(obj->obj_flags.extra_flags, ITEM_ANTI_EVIL);
          act("$p smells a little funny.",FALSE,ch,obj,0,TO_CHAR);
        } else if (IS_EVIL(ch)) {
          SET_BIT(obj->obj_flags.extra_flags, ITEM_ANTI_GOOD);
          act("$p makes a tinkling sound.",FALSE,ch,obj,0,TO_CHAR);
        } else {
          act("$p flashes brightly.",FALSE,ch,obj,0,TO_CHAR);
        }
        if(number(1,10)==3){
          act("$p emits an acrid smoke.",FALSE,ch,obj,0,TO_CHAR);
          SET_BIT(obj->obj_flags.extra_flags, ITEM_MAGIC);
        }
        return;
      }
    }
  } else {
    act("$p is already magical.",TRUE,ch,obj,0,TO_CHAR);
  }
}
void spell_heal(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int boost;

  assert(victim);
  spell_cure_blind(level, ch, victim, obj);
  if(number(0,99) < 10)
    spell_remove_poison(level, ch, victim, obj);
  if(level<=30)
    boost = 100;
  else if(level <=  70)
    boost = 600;
  else if(level <= 150)
    boost = 1200;
  else if(level <= 225)
    boost = 3600;
  else if(level <= 300)
    boost = 10000;
  else if(level <= 500)
    boost = 50000;
  else
    boost = 90000;
  GET_HIT(victim) += boost;
  if(GET_HIT(victim) >= hit_limit(victim))
    GET_HIT(victim) = hit_limit(victim);
  update_pos( victim );
  if(number(1,10)==7)
    send_to_char("A warm body fills your feeling.\n\r", victim);
  else
    send_to_char("A warm feeling fills your body.\n\r", victim);
}
void spell_groupheal(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct char_data *k;
  struct follow_type *f;

  if(!IS_AFFECTED(ch,AFF_GROUP)){
    send_to_char("You don't belong to a group!\n\r", ch);
    return;
  }
  if(ch->master)
    k = ch->master;
  else
    k = ch;
  if(IS_AFFECTED(k,AFF_GROUP))
    spell_heal(70,ch,k,obj);
  for(f=k->followers; f; f=f->next)
    if(IS_AFFECTED(f->follower,AFF_GROUP))
      spell_heal(70,ch,f->follower,obj);
  send_to_char("Nice work, Doc.\n\r",ch);
}
void spell_transmogrification(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  assert(victim);
  if(GET_MOVE(victim) < 0)
    return;
  GET_HIT(victim) += GET_MOVE(victim);
  GET_MOVE(victim) = 0;
  if (GET_HIT(victim) >= hit_limit(victim))
    GET_HIT(victim) = hit_limit(victim)-dice(1,3);
  update_pos( victim );
  send_to_char("You are transmogrified.\n\r", victim);
}
void spell_oesophagostenosis(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  assert(victim);
  if(GET_MOVE(victim) < 0)
    return;
  GET_MANA(victim) += GET_MOVE(victim);
  GET_MOVE(victim) = 0;
  if (GET_MANA(victim) >= mana_limit(victim))
    GET_MANA(victim) = mana_limit(victim)-dice(1,3);
  send_to_char("You are oesophagostenated.\n\r", victim);
}
void spell_invisibility(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  assert((ch && obj) || victim);

  if (obj) {
    if(obj_index[obj->item_number].virtual == 1000)
      return;
    if( !IS_SET(obj->obj_flags.extra_flags, ITEM_INVISIBLE) ) {
      act("$p turns invisible.",FALSE,ch,obj,0,TO_CHAR);
      act("$p turns invisible.",TRUE,ch,obj,0,TO_ROOM);
      SET_BIT(obj->obj_flags.extra_flags, ITEM_INVISIBLE);
    } else {
      act("$p turns visible.",FALSE,ch,obj,0,TO_CHAR);
      act("$p turns visible.",TRUE,ch,obj,0,TO_ROOM);
      REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_INVISIBLE);
    }
  } else { 
    if (!affected_by_spell(victim, SPELL_INVISIBLE)) {
      act("$n slowly fade out of existence.", TRUE, victim,0,0,TO_ROOM);
      send_to_char("You vanish.\n\r", victim);
      af.type      = SPELL_INVISIBLE;
      af.duration  = 24;
      af.modifier  = 10;
      af.location  = APPLY_AC;
      af.bitvector = AFF_INVISIBLE;
      affect_to_char(victim, &af);
    }
  }
}

void spell_stupidity(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int t;
  struct affected_type af;

  assert(victim || obj);

  if (victim) {
    if(IS_AFFECTED(victim,AFF_STUPIDITY))
      return;
    if(!IS_NPC(ch) && !IS_NPC(victim) && (GET_LEVEL(ch) < IMO))
      victim=ch;
    if((level > IMO) || (!saves_spell(victim, SAVING_PARA))){
      af.type = SPELL_STUPIDITY;
      t = (level>>5);
      af.duration = number(2,level);
      af.modifier = (-t);
      af.location = APPLY_INT;
      af.bitvector = AFF_STUPIDITY;
      affect_join(victim, &af, FALSE, FALSE);
      send_to_char("You feel very stupid.\n\r", victim);
    }
  }
}

void spell_poison(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int s,t;
  struct affected_type af;

  assert(victim || obj);

  if (victim) {
    if (IS_AFFECTED(victim, AFF_POISON))
      return;
    if(!IS_NPC(ch) && !IS_NPC(victim) && (GET_LEVEL(ch) < IMO))
      victim=ch;
    s = GET_STR(victim);
    if((level > IMO) || (!saves_spell(victim, SAVING_PARA))){
      af.type = SPELL_POISON;
      t = 2+level/100;
      af.duration = 10+t;
      af.modifier = (s > t) ? (-t) : (-s);
      af.location = APPLY_STR;
      af.bitvector = AFF_POISON;
      affect_join(victim, &af, FALSE, FALSE);
      send_to_char("You feel very sick.\n\r", victim);
    }
  } else { /* Object poison */
    if(obj_index[obj->item_number].virtual == 1000){
      if(GET_LEVEL(ch) < IMO)
        raw_kill(ch);
      return;
    }
    if ((obj->obj_flags.type_flag == ITEM_DRINKCON) ||
        (obj->obj_flags.type_flag == ITEM_FOOD)) {
      obj->obj_flags.value[3] = 1;
    }
  }
}

void spell_regen(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  if(IS_NPC(victim)) return;
  if (!affected_by_spell(victim, SPELL_REGEN) ) {
    assert(victim);
    af.type = SPELL_REGEN;
    af.duration = level*2;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_REGEN;
    affect_join(victim, &af, FALSE, FALSE);
    send_to_char("You feel energized.\n\r", victim);
  }
}

void spell_hyperregen(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  if(IS_NPC(victim)) return;
  if (!affected_by_spell(victim, SPELL_HYPERREGEN) ) {
    assert(victim);
    af.type = SPELL_HYPERREGEN;
    af.duration = level;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_HYPERREGEN;
    affect_join(victim, &af, FALSE, FALSE);
    send_to_char("You feel damn good!\n\r", victim);
  }
}

void spell_farsee(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  assert(victim);
  af.type = SPELL_FARSEE;
  af.duration = level*2;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_FARSEE;
  affect_join(victim, &af, FALSE, FALSE);
  send_to_char("You are less myopic.\n\r", victim);
}

void spell_protection_from_evil(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  assert(victim);

  if (!affected_by_spell(victim, SPELL_PROTECT_FROM_EVIL) ) {
    af.type      = SPELL_PROTECT_FROM_EVIL;
    af.duration  = 12;
    af.modifier  = 1;
    af.location  = APPLY_CON;
    af.bitvector = AFF_PROTECT_EVIL;
    affect_to_char(victim, &af);
    send_to_char("You have a righteous feeling!\n\r", victim);
  }
}

void spell_remove_curse(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  assert(ch && (victim || obj));

  if (obj) {
    if(IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP)){
      act("$p briefly glows blue.", TRUE, ch, obj, 0, TO_CHAR);
      REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
    }
  } else {      /* Then it is a PC | NPC */
    if (affected_by_spell(victim, SPELL_CURSE) ) {
      act("$n briefly glows red, then blue.",FALSE,victim,0,0,TO_ROOM);
      act("You feel better.",FALSE,victim,0,0,TO_CHAR);
      affect_from_char(victim, SPELL_CURSE);
    }
  }
}

void spell_remove_poison(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type *hjp;
  int rpvalue;

  assert(ch && (victim || obj));
  if (victim) {
    if(affected_by_spell(victim,SPELL_POISON)) {
      rpvalue=(level>>3);
      for(hjp = victim->affected; hjp; hjp = hjp->next)
        if (hjp->type == SPELL_POISON){
          if(hjp->duration > rpvalue){
            hjp->duration-=rpvalue;
            act("You feel somewhat better.",FALSE,victim,0,0,TO_CHAR);
            act("$N looks somewhat better.",FALSE,ch,0,victim,TO_ROOM);
            return;
          }
          break;
        }
      affect_from_char(victim,SPELL_POISON);
      act("A warm feeling runs through your body.",FALSE,victim,0,0,TO_CHAR);
      act("$N looks better.",FALSE,ch,0,victim,TO_ROOM);
    }
  } else {
    if ((obj->obj_flags.type_flag == ITEM_DRINKCON) ||
        (obj->obj_flags.type_flag == ITEM_FOOD)) {
      obj->obj_flags.value[3] = 0;
      act("The $p steams briefly.",FALSE,ch,obj,0,TO_CHAR);
    }
  }
}

void spell_forgetfulness(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{

  assert(ch && victim && !obj);
  if(IS_SET(world[ch->in_room].room_flags,LAWFUL))
    return;
  if(!IS_NPC(victim))
    return;
  if(saves_spell(victim, SAVING_PARA) && number(0,6)){
    act("$N resists your spell.",FALSE,ch,0,victim,TO_CHAR);
    hit(victim,ch,TYPE_UNDEFINED);
    return;
  }
  if(victim->player.title){
    free(victim->player.title);
    victim->player.title = 0;
  }
  victim->specials.lastback = 0;
  act("$N looks a little goofy.",FALSE,ch,0,victim,TO_ROOM);
}

void spell_sanctuary(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  if(IS_NPC(victim)) return;
  if (!affected_by_spell(victim, SPELL_SANCTUARY) ) {
    act("$n is surrounded by a white aura.",TRUE,victim,0,0,TO_ROOM);
    act("You start glowing.",TRUE,victim,0,0,TO_CHAR);
    af.type      = SPELL_SANCTUARY;
    af.duration  = (level < 30) ? 3 : 3+(level-30);
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_SANCTUARY;
    affect_to_char(victim, &af);
  }
}
void spell_holdalign(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  if(IS_NPC(victim)) return;
  if (!affected_by_spell(victim, SPELL_HOLDALIGN) ) {
    act("$n looks like a complete idiot.",TRUE,victim,0,0,TO_ROOM);
    act("You feel like an idiot.",TRUE,victim,0,0,TO_CHAR);
    af.type      = SPELL_HOLDALIGN;
    af.duration  = 24;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_HOLDALIGN;
    affect_to_char(victim, &af);
  }
}
void spell_clarity(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  if(IS_NPC(victim)) return;
  if (!affected_by_spell(victim, SPELL_CLARITY) ) {
    act("$n seems more purposeful.",TRUE,victim,0,0,TO_ROOM);
    act("You feel focused.",TRUE,victim,0,0,TO_CHAR);
    af.type      = SPELL_CLARITY;
    af.duration  = (level < 100) ? 3 : 168;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_CLARITY;
    affect_to_char(victim, &af);
  }
}

void spell_sleep(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  assert(victim);
  if(IS_NPC(victim))
    if(number(1,level) <= GET_LEVEL(victim))
      return;
  if((GET_LEVEL(ch) < IMO) && (ch != victim) && (!IS_NPC(ch)) &&
     (!IS_NPC(victim))){
    return;
  }
  if((GET_LEVEL(ch) < IMO)&&(GET_LEVEL(victim) > IMO)){
    victim = ch;
    send_to_char("Oh oh...\n\n",ch);
  }
  if((level > IMO) || (!saves_spell(victim, SAVING_PARA))) {
    af.type      = SPELL_SLEEP;
    af.duration  = number(1,4);
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_SLEEP;
    affect_join(victim, &af, FALSE, FALSE);
    if (GET_POS(victim)>POSITION_SLEEPING) {
      act("You feel very sleepy ..... zzzzzz",FALSE,victim,0,0,TO_CHAR);
      act("$n goes to sleep.",TRUE,victim,0,0,TO_ROOM);
      GET_POS(victim)=POSITION_SLEEPING;
    }
    return;
  } else if (IS_NPC(victim) && !IS_NPC(ch)) {
    act("$n is very angry!",TRUE,victim,0,0,TO_ROOM);
    hit(victim,ch,TYPE_UNDEFINED);
  }
}

void spell_strength(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af, *afp;
  int m,flag;
  
  assert(victim);
  if(IS_AFFECTED(victim, AFF_POISON)){
    send_to_char("You lose your concentration!\n\r",ch);
    return;
  }
  flag=0;
  if(victim->affected) {
    for(afp = victim->affected; afp; afp = afp->next)
      if(afp->type==SPELL_STRENGTH){
        flag=1; break;
      }
  }
  if(flag)
    m=number(-1,1);
  else
    m=1+(GET_INT(ch)+level)/10;
  act("You feel stronger.",FALSE,victim,0,0,TO_CHAR);
  af.type      = SPELL_STRENGTH;
  af.duration  = level;
  af.modifier  = (m > 127) ? 127 : m;
  af.location  = APPLY_STR;
  af.bitvector = 0;
  affect_join(victim, &af, TRUE, FALSE);
}

void spell_fearlessness(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  if(IS_NPC(victim)) return;
  if (!affected_by_spell(victim, SPELL_FEARLESSNESS) ) {
    act("$n looks rather heroic!",TRUE,victim,0,0,TO_ROOM);
    act("You feel heroic.",TRUE,victim,0,0,TO_CHAR);
    af.type      = SPELL_FEARLESSNESS;
    af.duration  = 2;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_FEARLESSNESS;
    affect_to_char(victim, &af);
  }
}

void spell_word_of_recall(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct char_data *mnt;
  extern int top_of_world;
  int loc_nr,tmp_loc;
  static int location=0;
  bool found = FALSE;

  assert(victim);
  if(ch != victim){
    if(IS_NPC(victim)) return;
    if(IS_SET(victim->specials.act,PLR_NOSUMMON)) return;
  }
  if(((GET_LEVEL(ch) < IMO) && (GET_LEVEL(victim) >= IMO)) ||
    IS_SET(world[victim->in_room].room_flags,NO_MAGIC))
    return;
  if((ch!=victim) && (GET_POS(victim)==POSITION_FIGHTING) &&
     (victim->desc) && (victim->desc->wait))
    return;
  if((level < IMO) && victim->specials.fighting &&
      IS_AFFECTED(victim,AFF_HOLD)){
    act("The power of $N negates the recall spell!",
      FALSE,victim,0,victim->specials.fighting,TO_CHAR);
    return;
  }
  if(victim->specials.recall_room){
    tmp_loc=victim->specials.recall_room;
    if(ch->in_room == tmp_loc){
      send_to_char("You're already there!\n\r",ch);
      WAIT_STATE(ch,PULSE_VIOLENCE);
      return;
    }
  } else {
    if(!location){
      loc_nr=3001;
      for(location=0;location<=top_of_world;location++)
        if(world[location].number == loc_nr){
          found=TRUE;
          break;
        }
      if ((location == top_of_world) || !found) {
        send_to_char("You are completely lost.\n\r", victim);
        location=0;
        return;
      }
    }
    tmp_loc=location;
  }
  /* a location has been found. */
  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, tmp_loc);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  do_look(victim, "",15);
  victim->points.move=MAX((victim->points.move - 50) , 0 );
}
void spell_summon(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  sh_int target;
  struct char_data *cons;
  assert(ch && victim);

  if(IS_NPC(victim)){
    if(IS_AFFECTED(victim,AFF_NOSUMMON)){
      send_to_char("You are a pathetic fool.\n\r",ch);
      return;
    }
    if((2*GET_LEVEL(victim)) >= GET_LEVEL(ch)){
      send_to_char("You failed.\n\r",ch);
      return;
    }
    if(IS_SET(victim->specials.act,ACT_SPEC)){
      send_to_char("You stink!\n\r",ch);
      return;
    }
    if(GET_POS(victim)==POSITION_FIGHTING){
      act("$n is busy now, sorry.",TRUE,victim,0,ch,TO_VICT);
      return;
    }
    if(IS_AFFECTED(victim,AFF_CHARM)&&(victim->master != ch)){
      act("$n is a loyal follower.",TRUE,victim,0,ch,TO_VICT);
      return;
    }
  } else {
    if(!victim->desc){
      send_to_char("Sorry, your target is in limbo.\n\r",ch);
      return;
    }
    if(level < GET_LEVEL(victim)){
      send_to_char("You are too humble a soul.\n\r",ch);
      return;
    }
    if((level < (IMO+1)) && IS_SET(victim->specials.act,PLR_NOSUMMON)){
      send_to_char("Sorry.\n\r",ch);
      return;
    }
  }
  if(GET_LEVEL(ch) < (IMO+1)){
    if(IS_SET(world[victim->in_room].room_flags,NOSUMMON)){
      send_to_char("You nearly succeed, but not quite.\n\r",ch);
      return;
    }
    if(IS_SET(world[ch->in_room].room_flags,NOSUMMON)){
      send_to_char("Alas.  No luck.\n\r",ch);
      return;
    }
  }
  if(!IS_NPC(ch)){
    for(cons=world[ch->in_room].people; cons; cons = cons->next_in_room )
      if(IS_NPC(cons) && IS_SET(cons->specials.act,ACT_AGGRESSIVE)){
        send_to_char("You failed.\n\r", ch);
        return;
      }
  }
  if (IS_NPC(victim) && saves_spell(victim, SAVING_SPELL) ) {
    send_to_char("You failed.\n\r", ch);
    return;
  }
  if((!IS_NPC(victim))&&(GET_POS(victim)==POSITION_FIGHTING)&&number(0,2)){
    send_to_char("You lost your concentration!\n\r",ch);
    return;
  }
  if(IS_SET(victim->specials.act,ACT_AGGRESSIVE))
    REMOVE_BIT(victim->specials.act,ACT_AGGRESSIVE);
  act("$n disappears suddenly.",TRUE,victim,0,0,TO_ROOM);
  target = ch->in_room;
  char_from_room(victim);
  char_to_room(victim,target);

  act("$n arrives suddenly.",TRUE,victim,0,0,TO_ROOM);
  act("$n has summoned you!",FALSE,ch,0,victim,TO_VICT);
  do_look(victim,"",15);
}


void spell_charm_person(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct follow_type *j,*jt;
  int k; 
  struct char_data *tmp;
  struct affected_type af;
  int maxcharmlevel;
  void add_follower(struct char_data *ch, struct char_data *leader);
  bool circle_follow(struct char_data *ch, struct char_data *victim);
  void stop_follower(struct char_data *ch);

  assert(ch && victim);

  if((GET_LEVEL(ch) < IMO) && (ch != victim) && (!IS_NPC(ch)) &&
      (!IS_NPC(victim)) && (GET_LEVEL(victim) < IMO)){
    return;
  }
  if (victim == ch) {
    send_to_char("You like yourself even better!\n\r", ch);
    return;
  }
  if (!IS_AFFECTED(victim, AFF_CHARM) && !IS_AFFECTED(ch, AFF_CHARM)){
    if (circle_follow(victim, ch)) {
      send_to_char("Sorry, following in circles can not be allowed.\n\r", ch);
      return;
    }
    if(level > IMO)
      maxcharmlevel = GET_LEVEL(ch)-1;
    else
      maxcharmlevel=(level < 55) ? 20 : 20+(level-50)/5;
    if(GET_LEVEL(victim) > maxcharmlevel){
      send_to_char("You fail miserably.\n\r",ch);
      return;
    }
    if (victim->master)
      stop_follower(victim);
    if(level <= IMO)
      if (saves_spell(victim, SAVING_PARA)){
        if (IS_NPC(victim) && !IS_NPC(ch)) {
          act("$n is very angry!",TRUE,victim,0,0,TO_ROOM);
          hit(victim,ch,TYPE_UNDEFINED);
        }
        return;
      }
    if(IS_NPC(victim) && IS_SET(victim->specials.act,ACT_AGGRESSIVE)){
      REMOVE_BIT(victim->specials.act,ACT_AGGRESSIVE);
      act("$n seems calmer now.",FALSE,victim,0,0,TO_ROOM);
      return;
    }
    k=0;
    for(j=ch->followers;j;j=jt){
      jt=j->next;
      k++;
    }
    if(number(1,4) < k){
      send_to_char("You fail MISERABLY.\n\r",ch);
      return;
    }
    if(victim->specials.act & ACT_SPEC)
      REMOVE_BIT(victim->specials.act, ACT_SPEC);
    add_follower(victim, ch);
    af.type = SPELL_CHARM_PERSON;
    af.duration  = 25-GET_LEVEL(victim)/2;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(victim, &af);
    act("Isn't $n just such a nice fellow?",FALSE,ch,0,victim,TO_VICT);
  }
}



void spell_sense_life(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  assert(victim);

  if (!affected_by_spell(victim, SPELL_SENSE_LIFE)) {
    send_to_char("Your feel your awareness improve.\n\r", ch);

    af.type      = SPELL_SENSE_LIFE;
    af.duration  = 5*level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_SENSE_LIFE;
    affect_to_char(victim, &af);
  }
}
#define REAL 0
#define VIRTUAL 1
void spell_reanimate(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
 struct char_data *mob;
 struct follow_type *j,*jt;
 int k; 
 struct char_data *read_mobile(int nr, int type);

 if(!obj)
   return;
 if(obj->obj_flags.value[3] != 1){
   send_to_char("There do not appear to be any corpses hereabouts?\n\r",ch);
   return;
 }
 if(dice(9,9) > ch->skills[SPELL_REANIMATE].learned){
   send_to_char("The spell fails miserably.\n\r",ch);
   return;
 }
 k=0;
 for(j=ch->followers;j;j=jt){
   jt=j->next;
   k++;
 }
 if(number(1,8) < k){
   send_to_char("The spell fails MISERABLY.\n\r",ch);
   return;
 }
 extract_obj(obj);
 mob=read_mobile(2, VIRTUAL);
 char_to_room(mob,ch->in_room);
 act("$n has created a zombie!",TRUE,ch,0,0,TO_ROOM);
 send_to_char("You have created a zombie.\n\r",ch);
 add_follower(mob,ch);
 mob->points.max_hit+=GET_LEVEL(ch);
 mob->points.hit=mob->points.max_hit;
 mob->player.title=(char *)strdup(GET_NAME(ch));
}
void spell_clone(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
 struct char_data *mob,*tmp; 
 struct char_data *read_mobile(int nr, int type);
 char buf[256];
 int f,n;

 if(obj){
   if(GET_LEVEL(ch) > (IMO-100)){
     if(GET_ITEM_TYPE(obj)==0){
       obj_from_char(obj);
       extract_obj(obj);
       obj=read_object(17024,VIRTUAL);
       obj_to_char(obj,ch);
       act("$n has created $o!",TRUE,ch,obj,0,TO_ROOM);
       act("You have created $o!",TRUE,ch,obj,0,TO_CHAR);
     } else {
       act("$n tried to clone $o.  What a fool!",TRUE,ch,obj,0,TO_ROOM);
       act("You try to clone $o, you fool.",TRUE,ch,obj,0,TO_CHAR);
     }
     return;
   }
   send_to_char("Cloning objects may not YET be possible??\n\r",ch);
   return;
 }
 if(!victim){
   send_to_char("Clone who?\n\r",ch);
   return;
 }
 if(dice(5,30) > ch->skills[SPELL_CLONE].learned){
   send_to_char("You fail, but not by much.\n\r",ch);
   return;
 }
 if(IS_SET(victim->specials.act,ACT_CLONE)){
   send_to_char("You are a knave and a fool.\n\r",ch);
   return;
 }
 if(IS_NPC(victim) && (GET_LEVEL(victim) <= (level/4))){
   mob=read_mobile(victim->nr, REAL);
   char_to_room(mob,ch->in_room);
   f = (mob->specials.act & 1);
   GET_EXP(mob) = GET_EXP(victim);
   sprintf(buf,"%s has been cloned!\n\r",
     victim->player.short_descr);
   send_to_room(buf,ch->in_room);
   victim->specials.act |= ACT_CLONE;
   mob->specials.act = ACT_ISNPC | ACT_CLONE;;
   GET_GOLD(mob) = 0;
 } else {
   send_to_char("You may not clone THAT!\n\r",ch);
 }
}

/* ***************************************************************************
 *                     NPC spells..                                          *
 * ************************************************************************* */

void spell_fire_breath(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;
  int hpch;
  struct obj_data *burn;

  assert(victim && ch);
  dam = (level*level*level)>>3;
  if(saves_spell(victim, SAVING_BREATH) )
    dam >>= 1;
  damage(ch, victim, dam, SPELL_FIRE_BREATH);

  /* And now for the damage on inventory */

  if(!saves_spell(victim,SAVING_BREATH)) {
    for(burn=victim->carrying ; burn ; burn=burn->next_content){
      if(number(1,7) == 3){
        if((burn->obj_flags.type_flag==ITEM_CONTAINER)&&
           (IS_SET(burn->obj_flags.extra_flags,ITEM_SFX)))
          continue;
        else {
          act("$o burns",0,victim,burn,0,TO_CHAR);
          obj_from_char(burn);
          extract_obj(burn);
          return;
        }
      }
    }
  }
}


void spell_frost_breath(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;
  int hpch;
  struct obj_data *frozen;

  assert(victim && ch);

  dam = level*level;
  if ( saves_spell(victim, SAVING_BREATH) )
    dam >>= 1;
  damage(ch, victim, dam, SPELL_FROST_BREATH);

  if (!saves_spell(victim, SAVING_BREATH)){
    for(frozen=victim->carrying ; 
      frozen && (frozen->obj_flags.type_flag!=ITEM_DRINKCON) && 
      (frozen->obj_flags.type_flag!=ITEM_PILL) &&
      (frozen->obj_flags.type_flag!=ITEM_POTION);
       frozen=frozen->next_content); 
    if(frozen) {
      act("$o breaks.",0,victim,frozen,0,TO_CHAR);
      extract_obj(frozen);
    }
    if(IS_SET(world[victim->in_room].room_flags,DARK))
    if(frozen=victim->equipment[WEAR_LIGHT]){
      if(frozen->obj_flags.value[2]){
        world[victim->in_room].light--;
        frozen->obj_flags.value[2]=0;
        act("$o is extinguished.",0,victim,frozen,0,TO_CHAR);
      }
    }
  }
}

void spell_gas_breath(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;
  int hpch;

  assert(victim && ch);

  dam = level*level;
  if ( saves_spell(victim, SAVING_BREATH) )
    dam >>= 1;
  damage(ch, victim, dam, SPELL_GAS_BREATH);

}

void spell_lightning_breath(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam;
  int hpch;

  assert(victim && ch);

  dam = level*level;

  if ( saves_spell(victim, SAVING_BREATH) )
    dam >>= 1;
  damage(ch, victim, dam, SPELL_LIGHTNING_BREATH);
}
void spell_super_heal(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  assert(victim);
  spell_cure_blind(level, ch, victim, obj);
  spell_remove_poison(level, ch, victim, obj);
  GET_HIT(victim) = hit_limit(victim);
  update_pos( victim );
  send_to_char("You feel a prickly sensation.\n\r", victim);
}

static int maxmanaheal[]={100,500,2500,12500,62500};
static char *breezename[]={"slight","cool","warm","refreshing","brisk"};

void spell_manaheal(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int hv;
  char buf[256];

  assert(victim);
  if((level < 1)||(level > 5))
    return;
  hv=maxmanaheal[level-1];
  if(GET_MANA(victim) < (GET_MAX_MANA(victim)-hv))
    GET_MANA(victim) += hv;
  else
    GET_MANA(victim) = GET_MAX_MANA(victim);
  sprintf(buf,"You feel a %s breeze.\n\r",breezename[level-1]);
  send_to_char(buf,victim);
}

void spell_moveheal(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  assert(victim);
  GET_MOVE(victim) = GET_MAX_MOVE(victim);
  send_to_char("You feel light on your feet.\n\r", victim);
}

void spell_greenman(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int num;
  struct char_data *tmp;

  assert(victim);
  num = real_mobile(2450);  /* 2450 = Little Green Man */
  if(num > 0){
    tmp=read_mobile(num,REAL);
    char_to_room(tmp,victim->in_room);
    act("You think you see $n.",TRUE,tmp,0,victim,TO_VICT);
    spell_charm_person(IMO+1,tmp,victim,0);
  }
}
void spell_earthquake(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int dam, halfdam;
  struct char_data *tmp, *tmp2;

  assert(ch);
  dam = dice(1,3)*level;
  halfdam = dam>>1;
  send_to_char("The earth trembles beneath your feet!\n\r", ch);
  act("$n makes the earth tremble and shiver\n\rYou fall, and hit yourself!",
    FALSE, ch, 0, 0, TO_ROOM);
  for(tmp = character_list; tmp; tmp = tmp2) {
    tmp2 = tmp->next;
    if(!IS_NPC(tmp)){
      if(GET_LEVEL(tmp) > IMO) continue;
      if(GET_LEVEL(tmp) < (level>>2)) continue;
    }
    if((GET_LEVEL(tmp) >= IMO)||(IS_NPC(tmp)==IS_NPC(ch))) continue;
    if ( (ch->in_room == tmp->in_room) && (ch != tmp) ) {
      damage(ch,tmp,GET_LEVEL(tmp) < 250 ? dam : halfdam,SPELL_EARTHQUAKE);
    } else {
      if (world[ch->in_room].zone == world[tmp->in_room].zone)
        send_to_char("The earth trembles and shivers.\n\r", tmp);
    }
  }
}
void spell_radiation(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int idam,dam,n=0;
  struct char_data *vict, *temp;
  struct obj_data *sh;

  assert(ch);
  if(IS_NPC(ch) && IS_AFFECTED(ch,AFF_CHARM))
    return;
  idam = dice((GET_WIS(ch)+GET_INT(ch))/25,level>>1);
  act("You fill the room with radiation.",
    FALSE, ch, 0, 0, TO_CHAR);
  act("$n fills the room with deadly radiation.",
    FALSE, ch, 0, 0, TO_ROOM);
  for(vict=world[ch->in_room].people;vict;vict=temp){
    temp = vict->next_in_room;
    if(!IS_NPC(vict) && !(vict->specials.fighting))
      continue;
    if((GET_LEVEL(vict) >= IMO)||(IS_NPC(vict)==IS_NPC(ch))) continue;
    dam = idam;
    send_to_char("Sizzle..\n\r",vict);
    if(ch != vict){
      if(saves_spell(vict,SAVING_ROD))
        dam>>=3;
      if((sh=vict->equipment[WEAR_SHIELD])&&
         (sh->obj_flags.value[1]==SHIELD_RAD)){
        dam >>= 2;
        act("$N's shield deflects most of the radiation.",
          FALSE, ch, 0, vict, TO_NOTVICT);
        act("Your shield deflects most of the radiation from $n.",
          FALSE, ch, 0, vict, TO_VICT);
      } else {
        switch(number(0,4)){
          case 0: spell_blindness(level,ch,vict,0); break;
          case 1: spell_poison(level,ch,vict,0); break;
        }
      }
      damage(ch,vict,dam,SPELL_RADIATION);
      n++;
      if(n==2){
        n=0;
        idam>>=1;
      }
    }
  }
}
void spell_cyclone(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int idam,dam,n=0;
  struct char_data *vict, *temp;
  struct obj_data *sh;

  assert(ch);
  if(IS_NPC(ch) && IS_AFFECTED(ch,AFF_CHARM))
    return;
  idam = dice((GET_WIS(ch)+GET_INT(ch))/25,level);
  act("You move your hands in a big circle!",
    FALSE, ch, 0, 0, TO_CHAR);
  act("$n makes a swirling motion.",
    FALSE, ch, 0, 0, TO_ROOM);
  for(vict=world[ch->in_room].people;vict;vict=temp){
    temp = vict->next_in_room;
    if(!IS_NPC(vict) && !(vict->specials.fighting))
      continue;
    if((GET_LEVEL(vict) >= IMO)||(IS_NPC(vict)==IS_NPC(ch))) continue;
    dam = idam;
    send_to_char("Whoosh..\n\r",vict);
    if(ch != vict){
      if(saves_spell(vict,SAVING_ROD))
        dam>>=2;
      if((sh=vict->equipment[WEAR_SHIELD])&&
         (sh->obj_flags.value[1]==SHIELD_CYCLONE)){
        dam >>= 2;
        act("$N's shield reduces the wind velocity.",
          FALSE, ch, 0, vict, TO_NOTVICT);
        act("Your shield reduces the wind velocity.",
          FALSE, ch, 0, vict, TO_VICT);
      }
      damage(ch,vict,dam,SPELL_CYCLONE);
      n++;
      if(n==5){
        idam/=3; n=0;
      }
    }
  }
}
void spell_invigorate(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type *aff;
  void teleport(struct char_data *ch, int to_room);
  int delta;

  if(IS_NPC(victim))
    return;
  delta = (level>>4);
  if(GET_WIS(ch) > 80)
    delta += GET_WIS(ch)-80;
  for(aff = victim->affected; aff; aff = aff->next) {
    if(!obj && (number(1,101) > ch->skills[SPELL_INVIGORATE].learned)){
      send_to_char("Grr..\n\r",ch);
      if((ch!=victim) && (number(1,7) < 2))
        teleport(ch,0);
      return;
    } else {
      (aff->duration)+=delta;
      send_to_char("Ah!\n\r",ch);
    }
  }
}
void spell_haste(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type af;

  if(IS_NPC(victim)) return;
  if (!affected_by_spell(victim, SPELL_HASTE) ) {
    act("$n starts quivering.",TRUE,victim,0,0,TO_ROOM);
    act("You start quivering.",TRUE,victim,0,0,TO_CHAR);
    af.type      = SPELL_HASTE;
    af.duration  = 2;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_HASTE;
    affect_to_char(victim, &af);
  }
}
void spell_dispel_spell(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  struct affected_type *aff;

  if(IS_NPC(victim))
    return;
  aff = victim->affected;
  if(!aff)
    return;
  aff->duration=0;
  send_to_char("Patience!\n\r",ch);
}
void spell_drain_mr(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj)
{
  int hploss,mr;
  struct obj_data *sh;

  assert(ch);
  assert(victim);
  if(IS_SET(world[ch->in_room].room_flags,LAWFUL))
    return;
  if(IS_NPC(ch) || !IS_NPC(victim))
    return;
  if(victim != ch->specials.fighting){
    act("You aren't fighting $N.",FALSE,ch,0,victim,TO_CHAR);
    return;
  }
  if(number(1,GET_LEVEL(ch)+GET_WIS(ch)) < GET_LEVEL(victim)){
    act("$N avoids any damage.",
      FALSE, ch, 0, victim, TO_ROOM);
    return; 
  }
  --(victim->specials.magres);
  act("$N's resistance seems to be a little lower.",
      FALSE, ch, 0, victim, TO_ROOM);
  act("Your spell seems to have had some effect.",
      FALSE, ch, 0, victim, TO_CHAR);
}


