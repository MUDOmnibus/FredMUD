/* ************************************************************************
*  file: spell_parser.c , Basic routines and parsing      Part of DIKUMUD *
*  Usage : Interpreter of spells                                          *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************* */

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h" 
#include "spells.h"
#include "handler.h"

#define SPELLO(nr,beat,pos,l0,s0,mana,tar,func) { \
 spell_info[nr].spell_pointer = (func);    \
 spell_info[nr].beats = (beat);            \
 spell_info[nr].minimum_position = (pos);  \
 spell_info[nr].min_usesmana = (mana);     \
 spell_info[nr].min_level = (l0); \
 spell_info[nr].max_skill = (s0); \
 spell_info[nr].targets = (tar);     \
}

#define SPELL_LEVEL(ch,sn) (spell_info[sn].min_level)

#define USE_MANA(ch, sn) (spell_info[sn].min_usesmana)

/* Global data */

extern struct room_data *world;
extern struct char_data *character_list;
extern char *spell_wear_off_msg[];

/* Extern procedures */

extern char *strdup (__const char *__s);

void cast_armor( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_clarity( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_bless( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_blindness( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_burning_hands( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_morphia( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_slime( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_charm_person( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_chill_touch( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_shocking_grasp( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_colour_spray( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_create_food( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_create_water( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_cure_blind( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_cure_critic( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_cure_light( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_curse( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_detect_invisibility(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_holdalign(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_infravision(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_kerplunk( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_drain_mr( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_dispel_evil( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_earthquake( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_enchant( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_energy_drain( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_fireball( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_sunburst( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_harm( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_heal( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_groupheal( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_manaheal( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_moveheal( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_invisibility( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_lightning_bolt( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_magic_missile( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_poison( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_regen( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_farsee( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_protection_from_evil(short level,struct char_data *ch,char *arg,
   int si, struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_remove_curse( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_sanctuary( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_sleep( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_strength( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_summon( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_relocate( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_fearlessness( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_word_of_recall( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_remove_poison( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_sense_life( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_reanimate( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_clone( short level, struct char_data *ch, char *arg, int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_transmogrification(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_gas(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_doppelganger(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_jingle(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_radiation(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_cyclone(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_forgetfulness(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_extraheal(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_barf(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_burlyheal(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_nova(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_invigorate(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_haste(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_hyperregen(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_stupidity(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_dispel_spell(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_extremelyburlyheal(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_oesophagostenosis(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);
void cast_transmutation(short level,struct char_data *ch,char *arg,int si,
   struct char_data *tar_ch, struct obj_data *tar_obj);

struct spell_info_type spell_info[MAX_SPL_LIST];

char *spells[]=
{
   "armor",               /* 1 */
   "clarity",
   "bless",
   "blindness",
   "burning hands",
   "enantiomorphia",
   "charm person",
   "chill touch",
   "reanimate",
   "colour spray",
   "relocate",     /* 11 */
   "create food",
   "create water",
   "cure blind",
   "cure critic",
   "cure light",
   "curse",
   "jingle",
   "detect invisible",
   "kerplunk",
   "drain mr",       /* 21 */
   "dispel evil",
   "earthquake",
   "enchant",
   "energy drain",
   "fireball",
   "harm",
   "heal",
   "invisibility",
   "lightning bolt",
   "doppelganger",      /* 31 */
   "magic missile",
   "poison",
   "protect from evil",
   "remove curse",
   "sanctuary",
   "shocking grasp",
   "sleep",
   "strength",
   "summon",
   "fearlessness",      /* 41 */
   "word of recall",
   "remove poison",
   "sense life",         /* 44 */
   "sunburst",
   "clone",
   "infravision",
   "hold alignment",
   "regeneration",
   "far sight",       /* 50 */
   "groupheal",
   "transmogrification",
   "burlyheal",
   "radiation",
   "nova",
   "forgetfulness",
   "extraheal",
   "barf",
   "invigorate",
   "haste",
   "hyper regeneration",
   "slime",
   "stupidity",
   "dispel spell",
   "extremelyburlyheal",
   "oesophagostenosis",
   "transmutation",
   "gas",
   "cyclone",
   "attack",
   "sneak",        /* 71 */
   "hide",
   "steal",
   "backstab",
   "pick",
   "kick",
   "bash",
   "rescue",
   "grescue",
   "shoot",
   "bomb",
   "\n",
   "mana restoration",
   "movement restoration",
   "\n"
};


void affect_update( void )
{
 void stop_follower(struct char_data *ch);
 void do_wake(struct char_data *ch, char *argument, int cmd);
 void do_stand(struct char_data *ch, char *argument, int cmd);
 static struct affected_type *af, *next_af_dude;
 static struct char_data *i;

 for (i = character_list; i; i = i->next)
  for (af = i->affected; af; af = next_af_dude) {
   next_af_dude = af->next;
   if (af->duration >= 1)
    af->duration--;
   else {
    if ((af->type > 0) && (af->type < MAXSPELL)) /* It must be a spell */
     if (!af->next || (af->next->type != af->type) ||
         (af->next->duration > 0))
      if (*spell_wear_off_msg[af->type]) {
       send_to_char(spell_wear_off_msg[af->type], i);
       send_to_char("\n\r", i);
      }
    if(af->type == SPELL_CHARM_PERSON)
      stop_follower(i);
    else {
      affect_remove(i, af);
      if(af->type == SPELL_SLEEP){
        do_wake(i,"",0);
        do_stand(i,"",0);
      }
    }
   }
  }
}


/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data *ch, struct char_data *victim)
{
 struct char_data *k;

 for(k=victim; k; k=k->master) {
  if (k == ch)
   return(TRUE);
 }
 return(FALSE);
}



/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data *ch)
{
 struct follow_type *j, *k;

 if(!ch->master) return;
 if (IS_AFFECTED(ch, AFF_CHARM)) {
  act("You realize that $N is a jerk!", FALSE, ch, 0, ch->master, TO_CHAR);
  act("$n realizes that $N is a jerk!", FALSE, ch, 0, ch->master, TO_NOTVICT);
  act("$n dislikes you intensely.", FALSE, ch, 0, ch->master, TO_VICT);
  if (affected_by_spell(ch, SPELL_CHARM_PERSON))
   affect_from_char(ch, SPELL_CHARM_PERSON);
 } else {
  act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
  act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
  act("$n stops following you.",TRUE, ch, 0, ch->master, TO_VICT);
 }
 if (ch->master->followers->follower == ch) { /* Head of follower-list? */
  k = ch->master->followers;
  ch->master->followers = k->next;
  free(k);
 } else { /* locate follower who is not head of list */
  for(k = ch->master->followers; k->next->follower!=ch; k=k->next)  ;
  j = k->next;
  k->next = j->next;
  free(j);
 }
 ch->master = 0;
 REMOVE_BIT(ch->specials.affected_by, AFF_CHARM | AFF_GROUP);
}


/* Called when a character that follows/is followed dies */
void die_follower(struct char_data *ch)
{
 struct follow_type *j, *k;

 if (ch->master)
  stop_follower(ch);
 for (k=ch->followers; k; k=j) {
  j = k->next;
  stop_follower(k->follower);
 }
}

/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data *ch, struct char_data *leader)
{
 struct follow_type *k;

 assert(!ch->master);

 ch->master = leader;
 CREATE(k, struct follow_type, 1);
 k->follower = ch;
 k->next = leader->followers;
 leader->followers = k;
 act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
 act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
 act("$n now follows $N.", TRUE, ch, 0, leader, TO_NOTVICT);
}

say_spell( struct char_data *ch, int si )
{
 char buf[MAX_STRING_LENGTH], splwd[MAX_BUF_LENGTH];
 char buf2[MAX_STRING_LENGTH];

 int j, offs;
 struct char_data *temp_char;

 struct syllable {
  char org[10];
  char new[10];
 };

 struct syllable syls[] = {
 { " ", " " },
 { "ar", "abra"   },
 { "au", "kada"    },
 { "bless", "fido" },
  { "blind", "nose" },
  { "bur", "mosa" },
 { "cu", "judi" },
 { "de", "oculo"},
 { "en", "unso" },
 { "light", "dies" },
 { "lo", "hi" },
 { "mor", "zak" },
 { "move", "sido" },
  { "ness", "lacri" },
  { "ning", "illa" },
 { "per", "duda" },
 { "ra", "gru"   },
  { "re", "candus" },
 { "son", "sabru" },
  { "tect", "infra" },
 { "tri", "cula" },
 { "ven", "nofo" },
 {"a", "a"},{"b","b"},{"c","q"},{"d","e"},{"e","z"},{"f","y"},{"g","o"},
 {"h", "p"},{"i","u"},{"j","y"},{"k","t"},{"l","r"},{"m","w"},{"n","i"},
 {"o", "a"},{"p","s"},{"q","d"},{"r","f"},{"s","g"},{"t","h"},{"u","j"},
 {"v", "z"},{"w","x"},{"x","n"},{"y","l"},{"z","k"}, {"",""}
 };



 strcpy(buf, "");
 strcpy(splwd, spells[si-1]);

 offs = 0;

 while(*(splwd+offs)) {
  for(j=0; *(syls[j].org); j++)
   if (strncmp(syls[j].org, splwd+offs, strlen(syls[j].org))==0) {
    strcat(buf, syls[j].new);
    if (strlen(syls[j].org))
     offs+=strlen(syls[j].org);
    else
     ++offs;
   }
 }
 sprintf(buf2,"$n utters the words, '%s'", buf);
 sprintf(buf, "$n utters the words, '%s'", spells[si-1]);
 for(temp_char = world[ch->in_room].people;
  temp_char;
  temp_char = temp_char->next_in_room)
  if(temp_char != ch) {
    act(buf2, FALSE, ch, 0, temp_char, TO_VICT);
  }
}

bool saves_spell(struct char_data *ch, sh_int save_type)
{
 int save;

 if(GET_LEVEL(ch) >= IMO)
   return(TRUE);

 save = ch->specials.apply_saving_throw[save_type]+
        GET_SAVING_THROW(GET_LEVEL(ch));
 return(MAX(1,save) < number(0,IMO));
}
char *skip_spaces(char *string)
{
 for(;*string && (*string)==' ';string++);
 return(string);
}
/* Assumes that *argument does start with first letter of chopped string */
void do_cast(struct char_data *ch, char *argument, int cmd)
{
 struct obj_data *tar_obj;
 struct char_data *tar_char;
 char name[MAX_STRING_LENGTH];
 int qend, spl, i, splev;
 bool target_ok;

 if (IS_SET(world[ch->in_room].room_flags,NO_MAGIC)){
  send_to_char("Your magic is powerless here.\n\r",ch);
  return;
 }
 argument = skip_spaces(argument);
 /* If there is no chars in argument */
 if (!(*argument)) {
  send_to_char("Cast which what where?\n\r", ch);
  return;
 }
 if (*argument != '\'') {
  send_to_char("Magic must always be enclosed by the magic symbols: '\n\r",ch);
  return;
 }

 /* Locate the last quote && lowercase the magic words (if any) */

 for (qend=1; *(argument+qend) && (*(argument+qend) != '\'') ; qend++)
  *(argument+qend) = LOWER(*(argument+qend));

 if (*(argument+qend) != '\'') {
  send_to_char("Magic must always be enclosed by the magic symbols: '\n\r",ch);
  return;
 }

 spl = old_search_block(argument, 1, qend-1,spells, 0);

 if (!spl) {
  send_to_char("Your lips do not move, no magic appears.\n\r",ch);
  return;
 }

#define ISASPELL ((spl > 0) && (spl < MAXSPELL))

 if(ISASPELL && spell_info[spl].spell_pointer) {
  if (GET_POS(ch) < spell_info[spl].minimum_position) {
   switch(GET_POS(ch)) {
    case POSITION_SLEEPING :
     send_to_char("You dream about great magical powers.\n\r", ch);
     break;
    case POSITION_RESTING :
     send_to_char("You can't concentrate enough while resting.\n\r",ch);
     break;
    case POSITION_SITTING :
     send_to_char("You can't do this sitting!\n\r", ch);
     break;
    case POSITION_FIGHTING :
     send_to_char("Impossible! You can't concentrate enough!.\n\r", ch);
     break;
    default:
     send_to_char("It seems like you're in a pretty bad shape!\n\r",ch);
     break;
   } /* Switch */
  } else {
   if(spell_info[spl].min_level > GET_LEVEL(ch)){
    send_to_char("Sorry, you can't do that yet.\n\r", ch);
    return;
   }
   argument+=qend+1; /* Point to the last ' */
   for(;*argument == ' '; argument++);
   target_ok = FALSE;
   tar_char = 0;
   tar_obj = 0;

   if (!IS_SET(spell_info[spl].targets, TAR_IGNORE)) {
    argument = one_argument(argument, name);
    if (*name) {
     if (IS_SET(spell_info[spl].targets, TAR_CHAR_ROOM))
      if (tar_char = get_char_room_vis(ch, name))
       target_ok = TRUE;
     if (!target_ok && IS_SET(spell_info[spl].targets, TAR_CHAR_WORLD))
      if (tar_char = (struct char_data *) get_ranchar_vis(ch, name))
       target_ok = TRUE;
     if (!target_ok && IS_SET(spell_info[spl].targets, TAR_OBJ_INV))
      if (tar_obj = get_obj_in_list_vis(ch, name, ch->carrying))
       target_ok = TRUE;
     if (!target_ok && IS_SET(spell_info[spl].targets, TAR_OBJ_ROOM))
      if (tar_obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents))
       target_ok = TRUE;
     if (!target_ok && IS_SET(spell_info[spl].targets, TAR_OBJ_WORLD))
      if (tar_obj = get_obj_vis(ch, name))
       target_ok = TRUE;
     if (!target_ok && IS_SET(spell_info[spl].targets, TAR_OBJ_EQUIP)) {
      for(i=0; i<MAX_WEAR && !target_ok; i++)
       if (ch->equipment[i] && str_cmp(name, ch->equipment[i]->name) == 0) {
        tar_obj = ch->equipment[i];
        target_ok = TRUE;
       }
     }
     if (!target_ok && IS_SET(spell_info[spl].targets, TAR_SELF_ONLY))
      if (str_cmp(GET_NAME(ch), name) == 0) {
       tar_char = ch;
       target_ok = TRUE;
      }
    } else { /* No argument was typed */
     if (IS_SET(spell_info[spl].targets, TAR_FIGHT_SELF))
      if (ch->specials.fighting) {
       tar_char = ch;
       target_ok = TRUE;
      }
     if (!target_ok && IS_SET(spell_info[spl].targets, TAR_FIGHT_VICT))
      if (ch->specials.fighting) {
       /* WARNING, MAKE INTO POINTER */
       tar_char = ch->specials.fighting;
       target_ok = TRUE;
      }
     if (!target_ok && IS_SET(spell_info[spl].targets, TAR_SELF_ONLY)) {
      tar_char = ch;
      target_ok = TRUE;
     }
    }
   } else {
    target_ok = TRUE; /* No target, is a good target */
   }
   if (!target_ok) {
    if (*name) {
     send_to_char("Say what?\n\r", ch);
    } else { /* Nothing was given as argument */
     if (spell_info[spl].targets < TAR_OBJ_INV){
      send_to_char("Who should the spell be cast upon?\n\r", ch);
      GET_MANA(ch) -= MIN(USE_MANA(ch, spl),GET_MANA(ch));
     } else
      send_to_char("What should the spell be cast upon?\n\r", ch);
    }
    return;
   } else if(GET_LEVEL(ch) < (IMO+1)){
    if((tar_char == ch) && IS_SET(spell_info[spl].targets, TAR_SELF_NONO)) {
     send_to_char("You can not cast this spell upon yourself.\n\r", ch);
     return;
    }
    else if ((tar_char!=ch)&&IS_SET(spell_info[spl].targets,TAR_SELF_ONLY)) {
     send_to_char("You can only cast this spell upon yourself.\n\r", ch);
     return;
    } else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tar_char)) {
     send_to_char("You are afraid that it could harm your master.\n\r", ch);
     return;
    }
   }
   if (GET_LEVEL(ch) < IMO) {
    if (GET_MANA(ch) < USE_MANA(ch, spl)) {
     send_to_char("You can't summon enough energy to cast the spell.\n\r", ch);
     return;
    }
   }
   say_spell(ch, spl);
   WAIT_STATE(ch, spell_info[spl].beats-(GET_DEX(ch)/100));
   if ((spell_info[spl].spell_pointer == 0) && spl>0)
    send_to_char("Sorry, this magic has not yet been implemented :(\n\r", ch);
   else {
    splev = GET_SPELL_LEVEL(ch);
    if(!IS_NPC(ch)){
      if(number(1,101) > ch->skills[spl].learned){
       send_to_char("You lost your concentration!\n\r", ch);
       GET_MANA(ch) -= (USE_MANA(ch, spl)>>1);
       return;
      }
      if(ch->skills[spl].used < 100)
        ch->skills[spl].used++;
      send_to_char("Ok.\n\r",ch);
      ((*spell_info[spl].spell_pointer) (splev,ch,argument,
         SPELL_TYPE_SPELL, tar_char, tar_obj));
      GET_MANA(ch) -= (USE_MANA(ch, spl));
    }
   }
  } /* if GET_POS < min_pos */
  return;
 }
 send_to_char("Eh?\n\r", ch);
}
void assign_spell_pointers(void)
{
 int i;

 for(i=0; i<MAX_SPL_LIST; i++)
  spell_info[i].spell_pointer = 0;

 /* From spells1.c */

 SPELLO(32,12,POSITION_FIGHTING, 2,95,25, 
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_magic_missile);
 SPELLO(16,12,POSITION_FIGHTING, 3,90,20,
  TAR_CHAR_ROOM, cast_cure_light);
 SPELLO( 1,12,POSITION_STANDING, 4,90,10,
  TAR_CHAR_ROOM, cast_armor);
 SPELLO(12,12,POSITION_STANDING, 5,50,10,
  TAR_IGNORE, cast_create_food);
 SPELLO(13,12,POSITION_STANDING, 6,50,50,
  TAR_OBJ_INV | TAR_OBJ_EQUIP, cast_create_water);
 SPELLO(20,12,POSITION_STANDING, 7,90,10,
  TAR_CHAR_ROOM, cast_kerplunk);
 SPELLO( 8,12,POSITION_FIGHTING, 8,95,25,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_chill_touch);
 SPELLO(39,12,POSITION_STANDING,10,90,25,
  TAR_CHAR_ROOM | TAR_SELF_ONLY, cast_strength);
 SPELLO( 4,24,POSITION_STANDING,12,90,10,
  TAR_CHAR_ROOM, cast_blindness);
 SPELLO(21,12,POSITION_FIGHTING,14,80,25000,
  TAR_FIGHT_VICT, cast_drain_mr);
 SPELLO( 5,12,POSITION_FIGHTING,16,95,25,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_burning_hands);
 SPELLO( 9,12,POSITION_STANDING,18,90,15,
  TAR_OBJ_ROOM,cast_reanimate);
 SPELLO(37,12,POSITION_FIGHTING,32,95,50,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_shocking_grasp);
 SPELLO(14,12,POSITION_STANDING,22,75,25,
  TAR_CHAR_ROOM, cast_cure_blind);
 SPELLO(15,12,POSITION_FIGHTING,24,90,20,
  TAR_CHAR_ROOM, cast_cure_critic);
 SPELLO(30,12,POSITION_FIGHTING,48,95,75,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_lightning_bolt);
 SPELLO(17,12,POSITION_STANDING,28,90,25,
  TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP, cast_curse);
 SPELLO(19,12,POSITION_STANDING,30,90,10,
  TAR_CHAR_ROOM | TAR_SELF_ONLY, cast_detect_invisibility);
 SPELLO(29,12,POSITION_STANDING,36,90,10,
  TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM|TAR_OBJ_EQUIP,cast_invisibility);
 SPELLO(43,12,POSITION_STANDING,34,90,10,
  TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, cast_remove_poison);
 SPELLO(10,10,POSITION_FIGHTING,64,95,75,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_colour_spray);
 SPELLO(25,15,POSITION_FIGHTING,38,95,25,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_energy_drain);
 SPELLO(23,15,POSITION_FIGHTING,40,95,25,
  TAR_IGNORE, cast_earthquake);
 SPELLO(24,12,POSITION_STANDING,42,90,100,
  TAR_OBJ_INV | TAR_OBJ_EQUIP, cast_enchant);
 SPELLO( 3,12,POSITION_STANDING,44,90,10,
  TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_CHAR_ROOM, cast_bless);
 SPELLO(28, 6,POSITION_FIGHTING,46,90,50,
  TAR_CHAR_ROOM, cast_heal);
 SPELLO(26,15,POSITION_FIGHTING,96,95,120,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_fireball);
 SPELLO(46,12,POSITION_STANDING,50,90,30,
  TAR_CHAR_ROOM | TAR_OBJ_INV, cast_clone);
 SPELLO(22,12,POSITION_FIGHTING,54,95,25,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_dispel_evil);
 SPELLO( 2,12,POSITION_FIGHTING,56,80,25,
  TAR_SELF_ONLY, cast_clarity);
 SPELLO( 7,12,POSITION_STANDING,58,90,10,
  TAR_CHAR_ROOM | TAR_SELF_NONO, cast_charm_person);
 SPELLO(11,12,POSITION_STANDING,60,75,50,
  TAR_CHAR_WORLD,cast_relocate);
 SPELLO(18,12,POSITION_FIGHTING,350,96,3500,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_jingle);
 SPELLO(34,12,POSITION_STANDING,68,70,10,
  TAR_CHAR_ROOM | TAR_SELF_ONLY, cast_protection_from_evil);
 SPELLO(33,12,POSITION_STANDING,66,90,25,
  TAR_CHAR_ROOM | TAR_SELF_NONO | TAR_OBJ_INV | TAR_OBJ_EQUIP, cast_poison);
 SPELLO(45,15,POSITION_FIGHTING,128,95,200,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_sunburst);
 SPELLO(35,12,POSITION_STANDING,70,95,10,
  TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_EQUIP|TAR_OBJ_ROOM, cast_remove_curse);
 SPELLO(27,15,POSITION_FIGHTING,72,95,40,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_harm);
 SPELLO(36,12,POSITION_STANDING,74,95,75,
  TAR_CHAR_ROOM, cast_sanctuary);
 SPELLO(40,12,POSITION_STANDING,76,95,50,
  TAR_CHAR_WORLD, cast_summon);
 SPELLO(41,12,POSITION_FIGHTING,78,95,10,
  TAR_SELF_ONLY, cast_fearlessness);
 SPELLO(47,12,POSITION_STANDING,80,95,10,
  TAR_CHAR_ROOM, cast_infravision);
 SPELLO(50,12,POSITION_FIGHTING,82,95,25,
  TAR_SELF_ONLY, cast_farsee);
 SPELLO(49,12,POSITION_FIGHTING,84,95,50,
  TAR_CHAR_ROOM, cast_regen);
 SPELLO(42,12,POSITION_STANDING,86,95, 5,
  TAR_CHAR_ROOM | TAR_SELF_ONLY, cast_word_of_recall);
 SPELLO(52, 6,POSITION_STANDING,88,95,50,
  TAR_SELF_ONLY, cast_transmogrification);
 SPELLO(54,20,POSITION_FIGHTING,90,95,30,
  TAR_IGNORE,cast_radiation);
 SPELLO(38,12,POSITION_STANDING,92,95,25,
  TAR_CHAR_ROOM, cast_sleep);
 SPELLO(44,12,POSITION_STANDING,94,95,10,
   TAR_CHAR_ROOM | TAR_SELF_ONLY,cast_sense_life);
 SPELLO(57, 6,POSITION_FIGHTING,98,95,70,
  TAR_CHAR_ROOM, cast_extraheal);
 SPELLO( 6,18,POSITION_FIGHTING,155,95,350,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_morphia);
 SPELLO(59,12,POSITION_STANDING,100,95,150,
  TAR_CHAR_ROOM,cast_invigorate);
 SPELLO(58, 6,POSITION_FIGHTING,100,95,90,
  TAR_CHAR_ROOM, cast_barf);
 SPELLO(48,12,POSITION_STANDING,120,75,1000,
  TAR_CHAR_ROOM, cast_holdalign);
 SPELLO(56,12,POSITION_STANDING,130,90,50,
  TAR_CHAR_ROOM,cast_forgetfulness);
 SPELLO(60,12,POSITION_STANDING,135,95,250,
  TAR_SELF_ONLY,cast_haste);
 SPELLO(67, 9,POSITION_STANDING,150,50,5000,
  TAR_OBJ_INV,cast_transmutation);
 SPELLO(53, 6,POSITION_FIGHTING,160,95,250,
  TAR_CHAR_ROOM, cast_burlyheal);
 SPELLO(51,12,POSITION_FIGHTING,175,95,350,
  TAR_IGNORE, cast_groupheal);
 SPELLO(66, 9,POSITION_STANDING,180,90,350,
  TAR_SELF_ONLY,cast_oesophagostenosis);
 SPELLO(55,60,POSITION_FIGHTING,205,95,100,
  TAR_CHAR_ROOM,cast_nova);
 SPELLO(64,12,POSITION_STANDING,235,95,750,
  TAR_SELF_ONLY,cast_dispel_spell);
 SPELLO(65,18,POSITION_FIGHTING,245,95,2250,
  TAR_CHAR_ROOM,cast_extremelyburlyheal);
 SPELLO(61,12,POSITION_STANDING,265,95,500,
  TAR_SELF_ONLY, cast_hyperregen);
 SPELLO(62,18,POSITION_FIGHTING,200,95,600,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT,cast_slime);
 SPELLO(63,12,POSITION_STANDING,295,95,250,
  TAR_CHAR_ROOM,cast_stupidity);
 SPELLO(68,18,POSITION_FIGHTING,250,95,1100,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT,cast_gas);
 SPELLO(31,12,POSITION_FIGHTING,300,95,2000,
  TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_doppelganger);
 SPELLO(69,18,POSITION_FIGHTING,140,95,35,
  TAR_IGNORE,cast_cyclone);
 SPELLO(83,12,POSITION_STANDING,1000,0,100,TAR_SELF_ONLY,cast_manaheal);
 SPELLO(84,12,POSITION_STANDING,1000,0,100,TAR_SELF_ONLY,cast_moveheal);
 SPELLO(70,1,POSITION_STANDING,1,90,100,TAR_IGNORE,0);
 SPELLO(71,1,POSITION_STANDING,1,90,100,TAR_IGNORE,0);
 SPELLO(72,1,POSITION_STANDING,1,90,100,TAR_IGNORE,0);
 SPELLO(73,1,POSITION_STANDING,1,90,100,TAR_IGNORE,0);
 SPELLO(74,1,POSITION_STANDING,1,90,100,TAR_IGNORE,0);
 SPELLO(75,1,POSITION_STANDING,1,90,100,TAR_IGNORE,0);
 SPELLO(76,1,POSITION_STANDING,1,90,100,TAR_IGNORE,0);
 SPELLO(77,1,POSITION_STANDING,1,90,100,TAR_IGNORE,0);
 SPELLO(78,1,POSITION_STANDING,1,90,100,TAR_IGNORE,0);
 SPELLO(79,1,POSITION_STANDING,1,90,100,TAR_IGNORE,0);
 SPELLO(80,1,POSITION_STANDING,1,90,100,TAR_IGNORE,0);
 SPELLO(81,1,POSITION_STANDING,1,90,100,TAR_IGNORE,0);
 SPELLO(82,1,POSITION_STANDING,1,90,100,TAR_IGNORE,0);

}

void spell_magic_missile(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_chill_touch(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_burning_hands(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_shocking_grasp(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_burning_hands(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_lightning_bolt(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_colour_spray(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_energy_drain(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_fireball(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_sunburst(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_nova(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_dispel_evil(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_morphia(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_slime(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_gas(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_drain_mr(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_blindness(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_cure_critic(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_poison(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_earthquake(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_radiation(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);
void spell_cyclone(short level, struct char_data *ch,
   struct char_data *v, struct obj_data *o);

struct weapon_spell_list wsplist[MAX_WEAPON_SPELL+1];

#define WSPELLO(nr,lowis,hiwis,baselev,levshift,spellname,fun) { \
 wsplist[nr].lo = (lowis); \
 wsplist[nr].hi = (hiwis); \
 wsplist[nr].lev = (baselev); \
 wsplist[nr].shift = (levshift); \
 wsplist[nr].name = (spellname); \
 wsplist[nr].spellfun = (fun); \
}

void assign_weapon_spells()
{
  int i;

  for(i=0;i<=MAX_WEAPON_SPELL;i++)
    wsplist[i].spellfun = 0;

  WSPELLO( 0, 6,  30,  7, 0,"magic_missile", spell_magic_missile);
  WSPELLO( 1,30, 500, 30, 1,"shocking_grasp", spell_shocking_grasp);
  WSPELLO( 2,10, 500, 50, 1,"fireball", spell_fireball);
  WSPELLO( 3, 0, 100, 12, 0,"cure", spell_cure_critic);
  WSPELLO( 4,50, 500, 25, 0,"dispel", spell_dispel_evil);
  WSPELLO( 5, 0, 500,125, 0,"blindness", spell_blindness);
  WSPELLO( 7,30, 120, 50, 0,"lightning", spell_lightning_bolt);
  WSPELLO( 8,20, 120, 40, 1,"chill", spell_chill_touch);
  WSPELLO( 9,25, 450, 80, 2,"sunburst", spell_sunburst);
  WSPELLO(10,20, 400, 80, 1,"colour_spray", spell_colour_spray);
  WSPELLO(11,75, 375,100, 2,"slime", spell_slime);
  WSPELLO(12,25, 300, 50, 0,"poison", spell_poison);
  WSPELLO(13, 5, 500,  1, 1,"gas", spell_gas);
  WSPELLO(14,10,1000,100, 1,"drain_mr", spell_drain_mr);

  log("Done assigning weapon spells.");
}



