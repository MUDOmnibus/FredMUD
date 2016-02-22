/* ************************************************************************
*  file: spells1.c , handling of magic.                   Part of DIKUMUD *
*  Usage : Procedures handling all offensive magic.                       *
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

/* Extern functions */

void spell_burning_hands(short level, struct char_data *ch, 
  struct char_data *victim, struct obj_data *obj);
void spell_morphia(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_chill_touch(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_shocking_grasp(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_colour_spray(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_earthquake(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_energy_drain(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_fireball(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_harm(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_barf(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_lightning_bolt(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_magic_missile(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_sunburst(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_radiation(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_cyclone(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_nova(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_slime(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void spell_gas(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);

void cast_burning_hands( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_burning_hands(level, ch, victim, 0); 
      break;
    default : 
      log("Serious screw-up in burning hands!");
      break;
  }
}


void cast_morphia(short level,struct char_data *ch,char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{

  switch (type) {
    case SPELL_TYPE_SPELL:
        spell_morphia(level, ch, victim, 0);
        break;
      default : 
        log("Serious screw-up in morphia!");
        break;
  }
}


void cast_slime(short level,struct char_data *ch,char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{

  switch (type) {
    case SPELL_TYPE_SPELL:
        spell_slime(level, ch, victim, 0);
        break;
    case SPELL_TYPE_WAND:
         if(victim) 
            spell_slime(level, ch, victim, 0);
         break;
      default : 
        log("Serious screw-up in slime!");
        break;
  }
}

void cast_gas(short level,struct char_data *ch,char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{

  switch (type) {
    case SPELL_TYPE_SPELL:
        spell_gas(level, ch, victim, 0);
        break;
    case SPELL_TYPE_WAND:
         if(victim) 
            spell_gas(level, ch, victim, 0);
         break;
      default : 
        log("Serious screw-up in gas!");
        break;
  }
}


void cast_chill_touch( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_chill_touch(level, ch, victim, 0);
      break;
      default : 
         log("Serious screw-up in chill touch!");
         break;
  }
}


void cast_shocking_grasp( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_shocking_grasp(level, ch, victim, 0);
      break;
      default : 
         log("Serious screw-up in shocking grasp!");
         break;
  }
}


void cast_colour_spray( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_colour_spray(level, ch, victim, 0);
         break; 
    case SPELL_TYPE_SCROLL:
         if(victim) 
            spell_colour_spray(level, ch, victim, 0);
         else if (!tar_obj)
        spell_colour_spray(level, ch, ch, 0);
         break;
    case SPELL_TYPE_WAND:
         if(victim) 
            spell_colour_spray(level, ch, victim, 0);
         break;
    default : 
         log("Serious screw-up in colour spray!");
         break;
  }
}


void cast_earthquake( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_STAFF:
      spell_earthquake(level, ch, 0, 0);
        break;
    default : 
         log("Serious screw-up in earthquake!");
         break;
  }
}


void cast_energy_drain( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_energy_drain(level, ch, victim, 0);
      break;
    case SPELL_TYPE_POTION:
         spell_energy_drain(level, ch, ch, 0);
         break;
    case SPELL_TYPE_SCROLL:
         if(victim)
        spell_energy_drain(level, ch, victim, 0);
         else if(!tar_obj)
            spell_energy_drain(level, ch, ch, 0);
         break;
    case SPELL_TYPE_WAND:
         if(victim)
        spell_energy_drain(level, ch, victim, 0);
         break;
    case SPELL_TYPE_STAFF:
         for (victim = world[ch->in_room].people ;
              victim ; victim = victim->next_in_room )
            if(victim != ch)
               spell_energy_drain(level, ch, victim, 0);
         break;
    default : 
         log("Serious screw-up in energy drain!");
         break;
  }
}

void cast_sunburst( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch(type) {
    case SPELL_TYPE_SPELL:
      spell_sunburst(level,ch,victim,0);
      break;
    case SPELL_TYPE_STAFF:
         for (victim = world[ch->in_room].people ;
              victim ; victim = victim->next_in_room )
            if(ch == victim->specials.fighting)
               spell_sunburst(level, ch, victim, 0);
         break;
    default:
         log("Serious screw-up in sunburst!");
         break;
  }
}
void cast_doppelganger( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch(type) {
    case SPELL_TYPE_SPELL:
      spell_doppelganger(level,ch,victim,0);
      break;
    case SPELL_TYPE_STAFF:
         for (victim = world[ch->in_room].people ;
              victim ; victim = victim->next_in_room )
            if(ch == victim->specials.fighting)
               spell_doppelganger(level, ch, victim, 0);
         break;
    default:
         log("Serious screw-up in doppelganger!");
         break;
  }
}
void cast_jingle( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch(type) {
    case SPELL_TYPE_SPELL:
      if(!ch->specials.fighting)
        spell_jingle(level,ch,victim,0);
      else
        for(victim = world[ch->in_room].people ;
          victim ; victim = victim->next_in_room )
            if(ch == victim->specials.fighting)
              spell_jingle(level, ch, victim, 0);
      break;
    default:
      log("Serious screw-up in doppelganger!");
      break;
  }
}

void cast_fireball( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_fireball(level, ch, victim, 0);
      break;
    case SPELL_TYPE_SCROLL:
         if(victim)
        spell_fireball(level, ch, victim, 0);
         else if(!tar_obj)
            spell_fireball(level, ch, ch, 0);
         break;
    case SPELL_TYPE_WAND:
         if(victim)
        spell_fireball(level, ch, victim, 0);
         break;
    default : 
         log("Serious screw-up in fireball!");
         break;

  }
}

void cast_harm( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
         spell_harm(level, ch, victim, 0);
         break;
    case SPELL_TYPE_POTION:
         spell_harm(level, ch, ch, 0);
         break;
    case SPELL_TYPE_STAFF:
         for (victim = world[ch->in_room].people ;
              victim ; victim = victim->next_in_room )
            if(victim != ch)
               spell_harm(level, ch, victim, 0);
         break;
    default : 
         log("Serious screw-up in harm!");
         break;
  }
}
void cast_barf( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
         spell_barf(level, ch, victim, 0);
         break;
    default : 
         log("Serious screw-up in barf!");
         break;
  }
}

void cast_lightning_bolt( short level, struct char_data *ch, char *arg,
 int type, struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
         spell_lightning_bolt(level, ch, victim, 0);
         break;
    case SPELL_TYPE_SCROLL:
         if(victim)
        spell_lightning_bolt(level, ch, victim, 0);
         else if(!tar_obj)
            spell_lightning_bolt(level, ch, ch, 0);
         break;
    case SPELL_TYPE_WAND:
         if(victim)
        spell_lightning_bolt(level, ch, victim, 0);
         break;
    default : 
         log("Serious screw-up in lightning bolt!");
         break;

  }
}


void cast_magic_missile( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_magic_missile(level, ch, victim, 0);
      break;
    case SPELL_TYPE_SCROLL:
         if(victim)
        spell_magic_missile(level, ch, victim, 0);
         else if(!tar_obj)
            spell_magic_missile(level, ch, ch, 0);
         break;
    case SPELL_TYPE_WAND:
         if(victim)
        spell_magic_missile(level, ch, victim, 0);
         break;
    default : 
         log("Serious screw-up in magic missile!");
         break;

  }
}

void cast_clone( short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj )
{
  spell_clone(level, ch, tar_ch, tar_obj);
}

void cast_cyclone( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_STAFF:
      spell_cyclone(level, ch, 0, 0);
      break;
    default : 
         log("Serious screw-up in cyclone!");
         break;
  }
}
void cast_radiation( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_WAND:
      spell_radiation(level, ch, 0, 0);
      break;
    default : 
         log("Serious screw-up in radiation!");
         break;
  }
}
void cast_nova( short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      spell_nova(level, ch, victim, 0);
      break;
    default : 
         log("Serious screw-up in nova!");
         break;
  }
}
