/* ************************************************************************
*  file: spells2.c , Implementation of magic.             Part of DIKUMUD *
*  Usage : All the non-offensive magic handling routines.                 *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************* */

#include <stdio.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"


/* Global data */

extern struct room_data *world;
extern struct char_data *character_list;
extern struct spell_info_type spell_info[MAX_SPL_LIST];
extern struct obj_data  *object_list;


/* Extern procedures */

void die(struct char_data *ch);
void update_pos( struct char_data *victim );
void damage(struct char_data *ch, struct char_data *victim,
            int damage, int weapontype);
void say_spell( struct char_data *ch, int si );
bool saves_spell(struct char_data *ch, sh_int spell);
void add_follower(struct char_data *ch, struct char_data *victim);
char *strdup(char *str);


void cast_armor( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      if ( affected_by_spell(tar_ch, SPELL_ARMOR) ){
        send_to_char("Nothing seems to happen.\n\r", ch);
        return;
      }
      if (ch != tar_ch)
        act("$N is protected by your deity.", FALSE, ch, 0, tar_ch, TO_CHAR);

      spell_armor(level,ch,tar_ch,0);
      break;
    case SPELL_TYPE_POTION:
      if ( affected_by_spell(ch, SPELL_ARMOR) )
        return;
      spell_armor(level,ch,ch,0);
      break;
    case SPELL_TYPE_SCROLL:
      if (tar_obj) return;
         if (!tar_ch) tar_ch = ch;
      if ( affected_by_spell(tar_ch, SPELL_ARMOR) )
        return;
      spell_armor(level,ch,ch,0);
      break;
    case SPELL_TYPE_WAND:
      if (tar_obj) return;
      if ( affected_by_spell(tar_ch, SPELL_ARMOR) )
        return;
      spell_armor(level,ch,ch,0);
      break;
      default : 
         log("Serious screw-up in armor!");
         break;
  }
}

void cast_clarity( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_WAND:
      if (!tar_ch)
        tar_ch = ch;
      spell_clarity(level, ch, tar_ch, 0);
      break;
    default : 
      log("Serious screw-up in clarity!");
      break;
  }
}


void cast_bless( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  struct affected_type af;

  switch (type) {
    case SPELL_TYPE_SPELL:
      if (tar_obj) {        /* It's an object */
        if ( IS_SET(tar_obj->obj_flags.extra_flags, ITEM_BLESS) ) {
          send_to_char("Nothing seems to happen.\n\r", ch);
          return;
        }
        spell_bless(level,ch,0,tar_obj);
      } else {              /* Then it is a PC | NPC */
        if ( affected_by_spell(tar_ch, SPELL_BLESS) ||
          (GET_POS(tar_ch) == POSITION_FIGHTING)) {
          send_to_char("Nothing seems to happen.\n\r", ch);
          return;
        } 
        spell_bless(level,ch,tar_ch,0);
      }
      break;
   case SPELL_TYPE_POTION:
       if ( affected_by_spell(ch, SPELL_BLESS) ||
        (GET_POS(ch) == POSITION_FIGHTING))
        return;
      spell_bless(level,ch,ch,0);
         break;
    case SPELL_TYPE_SCROLL:
      if (tar_obj) {        /* It's an object */
        if ( IS_SET(tar_obj->obj_flags.extra_flags, ITEM_BLESS) )
          return;
        spell_bless(level,ch,0,tar_obj);
      } else {              /* Then it is a PC | NPC */
        if (!tar_ch) tar_ch = ch;
        if ( affected_by_spell(tar_ch, SPELL_BLESS) ||
          (GET_POS(tar_ch) == POSITION_FIGHTING))
          return;
        spell_bless(level,ch,tar_ch,0);
      }
      break;
    case SPELL_TYPE_WAND:
      if (tar_obj) {        /* It's an object */
        if ( IS_SET(tar_obj->obj_flags.extra_flags, ITEM_BLESS) )
          return;
        spell_bless(level,ch,0,tar_obj);
      } else {              /* Then it is a PC | NPC */
        if ( affected_by_spell(tar_ch, SPELL_BLESS) ||
          (GET_POS(tar_ch) == POSITION_FIGHTING))
          return;
        spell_bless(level,ch,tar_ch,0);
      }
      break;
    default : 
         log("Serious screw-up in bless!");
         break;
  }
}

void cast_transmutation(short level,struct char_data *ch,char *arg,int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  struct affected_type af;

  switch (type) {
    case SPELL_TYPE_SPELL:
      if (tar_obj)
        spell_transmutation(level,ch,0,tar_obj);
      break;
    case SPELL_TYPE_WAND:
      if (tar_obj) 
        spell_transmutation(level,ch,0,tar_obj);
      break;
    default : 
         log("Serious screw-up in transmutation!");
         break;
  }
}
void cast_blindness( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  struct affected_type af;

  switch (type) {
    case SPELL_TYPE_SPELL:
      if ( IS_AFFECTED(tar_ch, AFF_BLIND) ){
        send_to_char("Nothing seems to happen.\n\r", ch);
        return;
      }
      spell_blindness(level,ch,tar_ch,0);
      break;
    case SPELL_TYPE_POTION:
      if ( IS_AFFECTED(ch, AFF_BLIND) )
        return;
      spell_blindness(level,ch,ch,0);
      break;
    case SPELL_TYPE_SCROLL:
      if (tar_obj) return;
      if (!tar_ch) tar_ch = ch;
      if ( IS_AFFECTED(ch, AFF_BLIND) )
        return;
      spell_blindness(level,ch,ch,0);
      break;
    case SPELL_TYPE_WAND:
      if (tar_obj) return;
      if (!tar_ch) tar_ch = ch;
      if ( IS_AFFECTED(ch, AFF_BLIND) )
        return;
      spell_blindness(level,ch,tar_ch,0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
               if (!(IS_AFFECTED(tar_ch, AFF_BLIND)))
                  spell_blindness(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in blindness!");
         break;
  }
}

void cast_create_food( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{

  switch (type) {
    case SPELL_TYPE_SPELL:
      act("$n magically creates a mushroom.",FALSE, ch, 0, 0, TO_ROOM);
         spell_create_food(level,ch,0,0);
      break;
    case SPELL_TYPE_SCROLL:
         if(tar_obj) return;
         if(tar_ch) return;
         spell_create_food(level,ch,0,0);
      break;
    default : 
         log("Serious screw-up in create food!");
         break;
  }
}



void cast_create_water( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      if (tar_obj->obj_flags.type_flag != ITEM_DRINKCON) {
        send_to_char("It is unable to hold water.\n\r", ch);
        return;
      }
      spell_create_water(level,ch,0,tar_obj);
      break;
      default : 
         log("Serious screw-up in create water!");
         break;
  }
}



void cast_cure_blind( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_cure_blind(level,ch,tar_ch,0);
      break;
    case SPELL_TYPE_POTION:
      spell_cure_blind(level,ch,ch,0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
               spell_cure_blind(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in cure blind!");
         break;
  }
}



void cast_cure_critic( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_cure_critic(level,ch,tar_ch,0);
      break;
    case SPELL_TYPE_POTION:
      spell_cure_critic(level,ch,ch,0);
      break;
    case SPELL_TYPE_WAND:
      spell_cure_critic(level,ch,tar_ch,0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
               spell_cure_critic(level,ch,tar_ch,0);
         break;
      default : 
         log("Serious screw-up in cure critic!");
         break;

  }
}



void cast_cure_light( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_cure_light(level,ch,tar_ch,0);
      break;
    case SPELL_TYPE_POTION:
      spell_cure_light(level,ch,ch,0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
               spell_cure_light(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in cure light!");
         break;
  }
}


void cast_curse( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      if (tar_obj)   /* It is an object */ 
        spell_curse(level,ch,0,tar_obj);
      else {              /* Then it is a PC | NPC */
        spell_curse(level,ch,tar_ch,0);
      }
      break;
    case SPELL_TYPE_POTION:
      spell_curse(level,ch,ch,0);
      break;
    case SPELL_TYPE_SCROLL:
      if (tar_obj)   /* It is an object */ 
        spell_curse(level,ch,0,tar_obj);
      else {              /* Then it is a PC | NPC */
        if (!tar_ch) tar_ch = ch;
        spell_curse(level,ch,tar_ch,0);
      }
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
               spell_curse(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in curse!");
         break;
  }
}

void cast_detect_invisibility( short level, struct char_data *ch,
 char *arg, int type, struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_WAND:
      if ( affected_by_spell(tar_ch, SPELL_DETECT_INVISIBLE) ){
        send_to_char("Nothing seems to happen.\n\r", tar_ch);
        return;
      }
      spell_detect_invisibility(level,ch,tar_ch,0);
      break;
    case SPELL_TYPE_POTION:
      if ( affected_by_spell(ch, SPELL_DETECT_INVISIBLE) )
        return;
      spell_detect_invisibility(level,ch,ch,0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
               if(!(IS_AFFECTED(tar_ch, SPELL_DETECT_INVISIBLE)))
                  spell_detect_invisibility(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in detect invisibility!");
         break;
  }
}

void cast_infravision( short level, struct char_data *ch,
 char *arg, int type, struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      if ( affected_by_spell(tar_ch, SPELL_INFRAVISION) ){
        send_to_char("Nothing seems to happen.\n\r", tar_ch);
        return;
      }
      spell_infravision(level,ch,tar_ch,0);
      break;
    case SPELL_TYPE_POTION:
      if ( affected_by_spell(ch, SPELL_INFRAVISION) )
        return;
      spell_infravision(level,ch,ch,0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
               if(!(IS_AFFECTED(tar_ch, SPELL_INFRAVISION)))
                  spell_infravision(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in infravision!");
         break;
  }
}

void cast_holdalign( short level, struct char_data *ch,
 char *arg, int type, struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      if ( affected_by_spell(tar_ch, SPELL_HOLDALIGN) ){
        send_to_char("Nothing seems to happen.\n\r", tar_ch);
        return;
      }
      spell_holdalign(level,ch,tar_ch,0);
      break;
    case SPELL_TYPE_POTION:
      if ( affected_by_spell(ch, SPELL_HOLDALIGN) )
        return;
      spell_holdalign(level,ch,ch,0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
               if(!(IS_AFFECTED(tar_ch, SPELL_HOLDALIGN)))
                  spell_holdalign(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in hold align!");
         break;
  }
}

void cast_kerplunk( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_kerplunk(level,ch,tar_ch,0);
      break;
    default : 
         log("Serious screw-up in kerplunk!");
         break;
  }
}

void cast_dispel_spell( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_dispel_spell(level, ch, tar_ch,0);
      break;
  }
}

void cast_dispel_evil( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_dispel_evil(level, ch, tar_ch,0);
      break;
    case SPELL_TYPE_POTION:
      spell_dispel_evil(level,ch,ch,0);
      break;
    case SPELL_TYPE_SCROLL:
      if (tar_obj) return;
      if (!tar_ch) tar_ch = ch;
      spell_dispel_evil(level, ch, tar_ch,0);
      break;
    case SPELL_TYPE_WAND:
      if (tar_obj) return;
      spell_dispel_evil(level, ch, tar_ch,0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
              spell_dispel_evil(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in dispel evil!");
         break;
  }
}

void cast_enchant( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_enchant(level, ch, 0,tar_obj);
      break;
    case SPELL_TYPE_SCROLL:
      if(!tar_obj) return;
      spell_enchant(level, ch, 0,tar_obj);
      break;
    default : 
      log("Serious screw-up in enchant!");
      break;
  }
}

void cast_groupheal( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj)
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_groupheal(70, ch, tar_ch, 0);
      break;
  }
}
void cast_transmogrification(short level,struct char_data *ch,
char *arg,int type,
struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      act("$n transmogrifies $N.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
      act("You transmogrify $N.", FALSE, ch, 0, tar_ch, TO_CHAR);
      spell_transmogrification(level, ch, tar_ch, 0);
      break;
    default : 
         log("Serious screw-up in transmogrify!");
         break;
  }
}
void cast_oesophagostenosis(short level,struct char_data *ch,
char *arg,int type,
struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      act("$n oesophagenates $N.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
      act("You oesophagenate $N.", FALSE, ch, 0, tar_ch, TO_CHAR);
      spell_oesophagostenosis(level, ch, tar_ch, 0);
      break;
    default : 
         log("Serious screw-up in oesophagostenosis");
         break;
  }
}
void cast_invisibility( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      if (tar_obj) {
        spell_invisibility(level, ch, 0, tar_obj);
      } else { /* tar_ch */
        if ( IS_AFFECTED(tar_ch, AFF_INVISIBLE) )
          send_to_char("Nothing new seems to happen.\n\r", ch);
        else
          spell_invisibility(level, ch, tar_ch, 0);
      }
      break;
    case SPELL_TYPE_POTION:
         if (!IS_AFFECTED(ch, AFF_INVISIBLE) )
            spell_invisibility(level, ch, ch, 0);
         break;
    case SPELL_TYPE_SCROLL:
      if (tar_obj) {
        if (!(IS_SET(tar_obj->obj_flags.extra_flags, ITEM_INVISIBLE)) )
          spell_invisibility(level, ch, 0, tar_obj);
      } else { /* tar_ch */
            if (!tar_ch) tar_ch = ch;

        if (!( IS_AFFECTED(tar_ch, AFF_INVISIBLE)) )
          spell_invisibility(level, ch, tar_ch, 0);
      }
      break;
    case SPELL_TYPE_WAND:
      if (tar_obj) {
        spell_invisibility(level, ch, 0, tar_obj);
      } else { /* tar_ch */
        if (!( IS_AFFECTED(tar_ch, AFF_INVISIBLE)) )
          spell_invisibility(level, ch, tar_ch, 0);
      }
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
               if (!( IS_AFFECTED(tar_ch, AFF_INVISIBLE)) )
                  spell_invisibility(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in invisibility!");
         break;
  }
}
void cast_drain_mr( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_drain_mr(level, ch, tar_ch, 0);
      break;
      default : 
         log("Serious screw-up in drain mr!");
         break;
  }
}

void cast_poison( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_poison(level, ch, tar_ch, tar_obj);
      break;
    case SPELL_TYPE_POTION:
      spell_poison(level, ch, ch, 0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
                  spell_poison(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in poison!");
         break;
  }
}

void cast_regen( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_WAND:
      if(tar_obj) return;
      spell_regen(level, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_SPELL:
      spell_regen(level, ch, tar_ch, tar_obj);
      break;
    case SPELL_TYPE_POTION:
      spell_regen(level, ch, ch, 0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if(tar_ch != ch)
              spell_regen(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in regen!");
         break;
  }
}

void cast_hyperregen( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_hyperregen(level, ch, tar_ch, tar_obj);
      break;
    case SPELL_TYPE_POTION:
      spell_hyperregen(level, ch, ch, 0);
      break;
    default : 
         log("Serious screw-up in hyperregen!");
         break;
  }
}

void cast_farsee( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_farsee(level, ch, tar_ch, tar_obj);
      break;
    default : 
      log("Serious screw-up in regen!");
      break;
  }
}

void cast_protection_from_evil( short level, struct char_data *ch,
  char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_protection_from_evil(level, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_POTION:
         spell_protection_from_evil(level, ch, ch, 0);
         break;
    case SPELL_TYPE_SCROLL:
         if(tar_obj) return;
         if(!tar_ch) tar_ch = ch;
      spell_protection_from_evil(level, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
                  spell_protection_from_evil(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in protection from evil!");
         break;
  }
}


void cast_remove_curse( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_remove_curse(level, ch, tar_ch, tar_obj);
      break;
    case SPELL_TYPE_POTION:
         spell_remove_curse(level, ch, ch, 0);
         break;
    case SPELL_TYPE_SCROLL:
         if(tar_obj) {
        spell_remove_curse(level, ch, 0, tar_obj);
         return;
      }
         if(!tar_ch) tar_ch = ch;
      spell_remove_curse(level, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
                  spell_remove_curse(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in remove curse!");
         break;
  }
}



void cast_remove_poison( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_remove_poison(level, ch, tar_ch, tar_obj);
      break;
    case SPELL_TYPE_POTION:
         spell_remove_poison(level, ch, ch, 0);
         break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
                  spell_remove_poison(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in remove poison!");
         break;
  }
}



void cast_sanctuary( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_sanctuary(level, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_POTION:
         spell_sanctuary(level, ch, ch, 0);
         break;
    case SPELL_TYPE_WAND:
         if(!tar_ch) tar_ch = ch;
      spell_sanctuary(level, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_SCROLL:
         if(tar_obj)
         return;
         if(!tar_ch) tar_ch = ch;
      spell_sanctuary(level, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
                  spell_sanctuary(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in sanctuary!");
         break;
  }
}


void cast_sleep( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_sleep(level, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_POTION:
      spell_sleep(level, ch, ch, 0);
      break;
    case SPELL_TYPE_SCROLL:
         if(tar_obj) return;
         if (!tar_ch) tar_ch = ch;
         spell_sleep(level, ch, tar_ch, 0);
         break;
    case SPELL_TYPE_WAND:
         if(tar_obj) return;
         spell_sleep(level, ch, tar_ch, 0);
         break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if(tar_ch != ch)
               spell_sleep(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in sleep!");
         break;
  }
}


void cast_strength( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_strength(level, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_POTION:
      spell_strength(level, ch, ch, 0);
      break;
    case SPELL_TYPE_SCROLL:
         if(tar_obj) return;
         if (!tar_ch) tar_ch = ch;
         spell_strength(level, ch, tar_ch, 0);
         break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
                  spell_strength(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in strength!");
         break;
  }
}

void cast_fearlessness( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch(type){
    case SPELL_TYPE_SPELL:
      spell_fearlessness(level, ch, ch, 0);
      break;
    case SPELL_TYPE_POTION:
      spell_fearlessness(level, ch, ch, 0);
      break;
    default:
      break;
  }
}

void cast_word_of_recall( short level, struct char_data *ch, char *arg,
  int type, struct char_data *tar_ch, struct obj_data *tar_obj )
{
  struct char_data *tmp,*next_tmp;

  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_word_of_recall(level, ch, ch, 0);
      break;
    case SPELL_TYPE_POTION:
      spell_word_of_recall(level, ch, ch, 0);
      break;
    case SPELL_TYPE_SCROLL:
      if(tar_obj) return;
      if (!tar_ch) tar_ch = ch;
      spell_word_of_recall(level, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_STAFF:
      for(tmp=world[ch->in_room].people;tmp;tmp=next_tmp){
         next_tmp=tmp->next_in_room;
         if(IS_NPC(tmp)) continue;
         spell_word_of_recall(level,ch,tmp,0);
      }
      break;
    default : 
         log("Serious screw-up in word of recall!");
         break;
  }
}

void cast_summon( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_summon(level, ch, tar_ch, 0);
      break;
      default : 
         log("Serious screw-up in summon!");
         break;
  }
}

void cast_relocate( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_relocate(level, ch, tar_ch, 0);
      break;
      default : 
         log("Serious screw-up in relocate!");
         break;
  }
}

void cast_charm_person( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_charm_person(level, ch, tar_ch, 0);
      break;
      case SPELL_TYPE_SCROLL:
         if(!tar_ch) return;
         spell_charm_person(level, ch, tar_ch, 0);
         break;
      case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
                  spell_charm_person(level,ch,tar_ch,0);
         break;
      default : 
         log("Serious screw-up in charm person!");
         break;
  }
}



void cast_sense_life( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_sense_life(level, ch, ch, 0);
      break;
      case SPELL_TYPE_POTION:
         spell_sense_life(level, ch, ch, 0);
         break;
      case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
                  spell_sense_life(level,ch,tar_ch,0);
         break;
      default : 
         log("Serious screw-up in sense life!");
         break;
  }
}

void cast_fire_breath( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_fire_breath(level, ch, tar_ch, 0);
      break;   /* It's a spell.. But people can'c cast it! */
      default : 
         log("Serious screw-up in firebreath!");
         break;
  }
}

void cast_frost_breath( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_frost_breath(level, ch, tar_ch, 0);
      break;   /* It's a spell.. But people can'c cast it! */
      default : 
         log("Serious screw-up in frostbreath!");
         break;
  }
}

void cast_gas_breath( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
    for (tar_ch = world[ch->in_room].people ; 
        tar_ch ; tar_ch = tar_ch->next_in_room)
      if ((tar_ch != ch)&&(GET_LEVEL(tar_ch) < IMO)&&(!IS_NPC(tar_ch)))
        spell_gas_breath(level,ch,tar_ch,0);
         break;
      default : 
         log("Serious screw-up in gasbreath!");
         break;
  }
}

void cast_lightning_breath( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_lightning_breath(level, ch, tar_ch, 0);
      break;   /* It's a spell.. But people can'c cast it! */
      default : 
         log("Serious screw-up in lightningbreath!");
         break;
  }
}

void cast_reanimate( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  spell_reanimate(level, ch, tar_ch, tar_obj);
}

void cast_forgetfulness( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_WAND:
      spell_forgetfulness(level, ch, tar_ch, 0);
      break;
    default : 
      log("Serious screw-up in forgetfulness!");
      break;
  }
}

void cast_invigorate( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      if(ch != tar_ch){
        act("$n invigorates $N.",FALSE,ch,0,tar_ch,TO_NOTVICT);
        act("You invigorate $N.", FALSE, ch, 0, tar_ch, TO_CHAR);
      } else {
        act("$n invigorates $mself.",FALSE,ch,0,tar_ch,TO_NOTVICT);
        act("You invigorate yourself.",FALSE,ch,0,tar_ch,TO_CHAR);
      }
      spell_invigorate(level, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_POTION:
         spell_invigorate(level, ch, tar_ch, tar_obj);
         break;
    default : 
         log("Serious screw-up in invigorate!");
         break;
  }
}
void cast_haste( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      if(ch != tar_ch){
        act("$n hastes $N.",FALSE,ch,0,tar_ch,TO_NOTVICT);
        act("You haste $N.", FALSE, ch, 0, tar_ch, TO_CHAR);
      } else {
        act("$n hastes $mself.",FALSE,ch,0,tar_ch,TO_NOTVICT);
        act("You haste yourself.",FALSE,ch,0,tar_ch,TO_CHAR);
      }
      spell_haste(level, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_POTION:
         spell_haste(level, ch, ch, 0);
         break;
    default : 
         log("Serious screw-up in haste!");
         break;
  }
}

/*
   Spells that cannot be casted by anyone!
*/

void cast_manaheal( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_POTION:
         spell_manaheal(level, ch, ch, 0);
         break;
    case SPELL_TYPE_WAND:
      if (tar_obj) return;
      spell_manaheal(level, ch, tar_ch,0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
              spell_manaheal(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in manaheal!");
         break;
  }
}

void cast_moveheal( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_POTION:
         spell_moveheal(level, ch, ch, 0);
         break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
              spell_moveheal(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in moveheal!");
         break;
  }
}

void cast_stupidity( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_stupidity(level, ch, tar_ch, tar_obj);
      break;
    case SPELL_TYPE_POTION:
      spell_stupidity(level, ch, ch, 0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
                  spell_stupidity(level,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in stupidity!");
         break;
  }
}
void cast_heal( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      act("$n heals $N.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
      act("You heal $N.", FALSE, ch, 0, tar_ch, TO_CHAR);
      spell_heal(30, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_POTION:
         spell_heal(30, ch, ch, 0);
         break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
              spell_heal(30,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in heal!");
         break;
  }
}
void cast_extraheal( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      act("$n really heals $N.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
      act("You really heal $N.", FALSE, ch, 0, tar_ch, TO_CHAR);
      spell_heal(50, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_POTION:
         spell_heal(60, ch, ch, 0);
         break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
              spell_heal(60,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in extraheal!");
         break;
  }
}
void cast_burlyheal( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      act("$n lays $s hands upon $N and hums.",FALSE,ch,0,tar_ch,TO_NOTVICT);
      act("You touch $N and hum a little tune.",FALSE,ch,0,tar_ch,TO_CHAR);
      spell_heal(150, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_POTION:
         spell_heal(150, ch, ch, 0);
         break;
    case SPELL_TYPE_SCROLL:
      spell_heal(150, ch, ch, 0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
              spell_heal(150,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in burlyheal!");
         break;
  }
}
void cast_veryburlyheal( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      act("$n gives $N a soothing massage.",FALSE,ch,0,tar_ch,TO_NOTVICT);
      act("You massage $N and whistle a little tune.",
        FALSE,ch,0,tar_ch,TO_CHAR);
      spell_heal(200, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_POTION:
         spell_heal(200, ch, ch, 0);
         break;
    case SPELL_TYPE_SCROLL:
      spell_heal(200, ch, ch, 0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
              spell_heal(200,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in veryburlyheal!");
         break;
  }
}

void cast_extremelyburlyheal( short level, struct char_data *ch,
  char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      act("$n gives $N a soothing massage.",FALSE,ch,0,tar_ch,TO_NOTVICT);
      act("You massage $N and whistle a little tune.",
        FALSE,ch,0,tar_ch,TO_CHAR);
      spell_heal(275, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_POTION:
         spell_heal(275, ch, ch, 0);
         break;
    case SPELL_TYPE_SCROLL:
      spell_heal(275, ch, ch, 0);
      break;
    case SPELL_TYPE_STAFF:
         for (tar_ch = world[ch->in_room].people ; 
              tar_ch ; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch) 
              spell_heal(275,ch,tar_ch,0);
         break;
    default : 
         log("Serious screw-up in burlyheal!");
         break;
  }
}
