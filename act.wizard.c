/* ************************************************************************
*  file: actwiz.c , Implementation of commands.           Part of DIKUMUD *
*  Usage : Wizard Commands.                                               *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************* */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <malloc.h>
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "limits.h"

/*   external vars  */

extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern int nokillflag,nostealflag,nonewplayers,noshoutflag,nonewconnect;
extern int maxpokerbet,hitchance,mischance,defaulthelper;
extern int top_of_world,mobdeaths,pcdeaths,boottime,momsreadflag;
extern int shutdownwarn,shutdowngame,droptotal,newsflashflag;
extern int bobflag,jokecount,grand_total,reboottime,melsnumber;
extern char *bobname;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern dig_room, build_room, dig_vnum, build_vnum;

/* external functs */

void do_wiznet(struct char_data *ch, char *argument, int cmd);
void do_look(struct char_data *ch, char *argument, int cmd);
void set_title(struct char_data *ch);
int str_cmp(char *arg1, char *arg2);
struct time_info_data age(struct char_data *ch);
void sprinttype(int type, char *names[], char *result);
void sprintbit(long vektor, char *names[], char *result);
int mana_limit(struct char_data *ch);
int hit_limit(struct char_data *ch);
int move_limit(struct char_data *ch);
int mana_gain(struct char_data *ch);
int hit_gain(struct char_data *ch);
int move_gain(struct char_data *ch);
void to_long_long(char *s, long long n);

#ifdef NEEDS_STRDUP
char *strdup(char *s);
#endif
do_emote(struct char_data *ch, char *argument, int cmd)
{
  int i;
  static char buf[MAX_STRING_LENGTH];

  for (i = 0; *(argument + i) == ' '; i++);
  if (!*(argument + i))
    send_to_char("Yes.. But what?\n\r", ch);
  else {
    sprintf(buf,"$n %s", argument + i);
    act(buf,FALSE,ch,0,0,TO_ROOM);
 if(!IS_NPC(ch) && IS_SET(ch->specials.act,PLR_REPEAT)) {
        sprintf(buf,"%s %s\n\r",GET_NAME(ch),argument + i);
        send_to_char(buf,ch);
    }
    else
    send_to_char("Ok.\n\r", ch);
  }
}
void do_echo(struct char_data *ch, char *argument, int cmd)
{
  int i;
  static char buf[MAX_STRING_LENGTH];
  
  if(GET_LEVEL(ch) < IMO){
    if(!IS_SET(ch->specials.act,PLR_ECHO)){
      send_to_char("Huh?\n\r",ch);
      return;
    }
    REMOVE_BIT(ch->specials.act,PLR_ECHO);
  }
  for (i = 0; *(argument + i) == ' '; i++);
  if (!*(argument + i))
    send_to_char("That must be a mistake...\n\r", ch);
  else
  {
    sprintf(buf,"%s\n\r", argument + i);
    send_to_room(buf, ch->in_room);
/*  send_to_room_except(buf, ch->in_room, ch);  */
    send_to_char("Ok.\n\r", ch);
  }
}
void do_trans(struct char_data *ch, char *argument, int cmd)
{
  struct descriptor_data *i;
  struct char_data *victim,*cons;
  char buf[100];
  sh_int target;

  if (IS_NPC(ch))
    return;

  one_argument(argument,buf);
  if (!*buf)
    send_to_char("Who do you wich to transfer?\n\r",ch);
  else if (str_cmp("all", buf)) {
    if (!(victim = get_char_vis(ch,buf)))
      send_to_char("No-one by that name around.\n\r",ch);
    else {
      if(GET_LEVEL(victim) > GET_LEVEL(ch)){
        send_to_char("That might not be appreciated.\n\r",ch);
        return;
      }
      act("$n disappears in a mushroom cloud.",FALSE,victim,0,0,TO_ROOM);
      target = ch->in_room;
      char_from_room(victim);
      char_to_room(victim,target);
      act("$n arrives from a puff of smoke.",FALSE,victim,0,0,TO_ROOM);
      act("$n has transferred you!",FALSE,ch,0,victim,TO_VICT);
      do_look(victim,"",15);
      send_to_char("Ok.\n\r",ch);
    }
  } else { /* Trans All */
    if(ch->player.level >= IMO+999)
    for (i = descriptor_list; i; i = i->next)
      if (i->character != ch && !i->connected) {
        target = ch->in_room;
        victim = i->character;
        if(GET_LEVEL(victim) >= GET_LEVEL(ch)) continue;
        char_from_room(victim);
        char_to_room(victim,target);
        act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
        act("$n has transferred you!",FALSE,ch,0,victim,TO_VICT);
        do_look(victim,"",15);
      }
    send_to_char("Ok.\n\r",ch);
  }
}

void do_at(struct char_data *ch, char *argument, int cmd)
{
  char command[MAX_INPUT_LENGTH], loc_str[MAX_INPUT_LENGTH];
  int loc_nr, location, original_loc;
  struct char_data *target_mob;
  struct obj_data *target_obj;
  
  if (IS_NPC(ch))
    return;

  half_chop(argument, loc_str, command);
  if (!*loc_str) {
    send_to_char("You must supply a room number or a name.\n\r", ch);
    return;
  }
  if (isdigit(*loc_str)) {
    loc_nr = atoi(loc_str);
    for (location = 0; location <= top_of_world; location++)
      if (world[location].number == loc_nr)
        break;
      else if (location == top_of_world) {
        send_to_char("No room exists with that number.\n\r", ch);
        return;
      }
  } else if (target_mob = get_char_vis(ch, loc_str))
    location = target_mob->in_room;
  else if (target_obj = get_obj_vis(ch, loc_str))
    if (target_obj->in_room != NOWHERE)
      location = target_obj->in_room;
    else {
      send_to_char("The object is not available.\n\r", ch);
      return;
    } else {
    send_to_char("No such creature or object around.\n\r", ch);
    return;
  }

  /* a location has been found. */

  original_loc = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the guy's still there */
  for (target_mob = world[location].people; target_mob; target_mob =
    target_mob->next_in_room)
    if (ch == target_mob)
    {
      char_from_room(ch);
      char_to_room(ch, original_loc);
    }
}

int room_virtual_to_real(int vn)
{
  int lo,hi,mid,n;

  lo=0;
  hi=top_of_world;
  while(lo<=hi){
    mid=(lo+hi)>>1;
    n=world[mid].number;
    if(vn == n)
      return mid;
    else if(vn < n)
      hi=mid-1;
    else
      lo=mid+1;
  }
  return -1;
}
void do_goto(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_INPUT_LENGTH];
  int loc_nr, location,i,flag;
  struct char_data *target_mob, *pers;
  struct obj_data *target_obj;

  if(IS_NPC(ch) && (GET_LEVEL(ch) < IMO))
    return;
  one_argument(argument, buf);
  if (!*buf) {
    send_to_char("You must supply a room number or a name.\n\r", ch);
    return;
  }
  if (isdigit(*buf)) {
    loc_nr = atoi(buf);
    location = room_virtual_to_real(loc_nr);
    if(location < 0){
        send_to_char("No room exists with that number.\n\r", ch);
        return;
    }
  } else if (target_mob = get_char_vis(ch, buf))
    location = target_mob->in_room;
  else if (target_obj = get_obj_vis(ch, buf))
    if (target_obj->in_room != NOWHERE)
      location = target_obj->in_room;
    else {
      send_to_char("The object is not available.\n\r", ch);
      return;
    } else {
    send_to_char("No such creature or object around.\n\r", ch);
    return;
  }

  /* a location has been found. */

  flag=((GET_LEVEL(ch)>IMO)&&
       IS_SET(ch->specials.act,PLR_WIZINVIS));
  if(!flag){
    if(GET_LEVEL(ch) > 9000)
      act("Your world is poorer, because $n has left!",FALSE,ch,0,0,TO_ROOM);
    else
      act("$n disappears in a puff of smoke.",FALSE,ch,0,0,TO_ROOM);
  }
  char_from_room(ch);
  char_to_room(ch, location);
  if(!flag){
    if(GET_LEVEL(ch) > 9000)
      act("Yippee! $n has appeared out of thin air!", FALSE,ch,0,0,TO_ROOM);
    else
      act("$n appears with an ear-splitting bang.", FALSE, ch, 0,0,TO_ROOM);
  }
  GET_POS(ch) = POSITION_STANDING;
  do_look(ch,"",15);
}
void do_stat(struct char_data *ch, char *argument, int cmd)
{
  extern char *spells[];
  struct affected_type *aff;
  char arg1[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  struct char_data *victim=0, *mnt;
  struct room_data *rm=0;
  struct char_data *k=0;
  struct obj_data  *j=0;
  struct obj_data  *j2=0;
  struct extra_descr_data *desc;
  struct follow_type *fol;
  int i,i2,t,virtual;
  bool found;
  char bufshort[32];

  /* for objects */
  extern char *item_types[];
  extern char *wear_bits[];
  extern char *extra_bits[];
  extern char *drinks[];

  /* for rooms */
  extern char *dirs[];
  extern char *room_bits[];
  extern char *exit_bits[];
  extern char *sector_types[];

  /* for chars */
  extern char *equipment_types[];
  extern char *affected_bits[];
  extern char *apply_types[];
  extern char *action_bits[];
  extern char *player_bits[];
  extern char *position_types[];
  extern char *connected_types[];
  struct char_data *get_specific_vis(struct char_data *ch,char *name,int type);

  if (IS_NPC(ch))
    return;
  argument = one_argument(argument, arg1);
  if (!*arg1) {
    send_to_char("Stats on who or what?\n\r",ch);
    return;
  } else {
    /* stats on room */
    if((cmd != 232) && (cmd != 233) && (!str_cmp("room", arg1))) {
      rm = &world[ch->in_room];
      sprintf(buf,"Room: %s, Zone: %d. V-Num: %d, R-num: %d, Light: %d.\n\r",
              rm->name, rm->zone, rm->number, ch->in_room, rm->light);
      send_to_char(buf, ch);
      sprinttype(rm->sector_type,sector_types,buf2);
      sprintf(buf, "Sector type : %s, ", buf2);
      send_to_char(buf, ch);
      strcpy(buf,"Special procedure : ");
      strcat(buf,(rm->funct) ? "Exists\n\r" : "No\n\r");
      send_to_char(buf,ch);
      sprintf(buf,"Room flags (%d): ",(long) rm->room_flags);
      send_to_char(buf,ch);
      sprintbit((long) rm->room_flags,room_bits,buf);
      strcat(buf,"\n\r");
      send_to_char(buf,ch);
      send_to_char("Description:\n\r", ch);
      send_to_char(rm->description, ch);
#ifdef VERBOSE_STAT
      strcpy(buf, "Extra description keywords(s): ");
      if(rm->ex_description) {
        strcat(buf, "\n\r");
        for (desc = rm->ex_description; desc; desc = desc->next) {
          strcat(buf, desc->keyword);
          strcat(buf, "\n\r");
        }
        strcat(buf, "\n\r");
        send_to_char(buf, ch);
      } else {
        strcat(buf, "None\n\r");
        send_to_char(buf, ch);
      }
#endif
      strcpy(buf, "Chars present:\n\r");
      for (k = rm->people; k; k = k->next_in_room) {
        if(!CAN_SEE(ch,k)) continue;
        strcat(buf, GET_NAME(k));
        strcat(buf,
         (!IS_NPC(k) ? "(PC)\n\r" : (!IS_MOB(k) ? "(NPC)\n\r" : "(MOB)\n\r")));
      }
      strcat(buf, "\n\r");
      send_to_char(buf, ch);
      strcpy(buf, "Contents:\n\r");
      for (j = rm->contents; j; j = j->next_content) {
        strcat(buf, j->name);
        strcat(buf, "\n\r");
      }
      strcat(buf, "\n\r");
      send_to_char(buf, ch);
      return;
    }
    if((cmd != 232) && (cmd != 233) && (j = get_obj_vis(ch, arg1))) {
      virtual = (j->item_number >= 0) ? obj_index[j->item_number].virtual : 0;
      sprintf(buf,"Object name: [%s], R-num: [%d], V-number: [%d] Item type: ",
         j->name, j->item_number, virtual);
      sprinttype(GET_ITEM_TYPE(j),item_types,buf2);
      strcat(buf,buf2); strcat(buf,"\n\r");
      send_to_char(buf, ch);
      sprintf(buf, "Short desc: %s\n\rLong desc:\n\r%s\n\r",
              ((j->short_description) ? j->short_description : "None"),
              ((j->description) ? j->description : "None") );
      send_to_char(buf, ch);
      if(j->ex_description){
        strcpy(buf, "Extra desc keyword(s):\n\r");
        for (desc = j->ex_description; desc; desc = desc->next) {
          strcat(buf, desc->keyword);
          strcat(buf, "\n\r");
        }
        send_to_char(buf, ch);
      }
      sprintf(buf,"Worn (%d): ",j->obj_flags.wear_flags);
      send_to_char(buf, ch);
      sprintbit(j->obj_flags.wear_flags,wear_bits,buf);
      strcat(buf,"\n\r");
      send_to_char(buf, ch);
      sprintf(buf,"Extra(%d): ",j->obj_flags.extra_flags);
      send_to_char(buf, ch);
      sprintbit(j->obj_flags.extra_flags,extra_bits,buf);
      strcat(buf,"\n\r");
      send_to_char(buf,ch);
      sprintf(buf,"Wgt: %d, Val: %d, Rent: %d, Timer: %d, ID: %d\n\r",
        j->obj_flags.weight,j->obj_flags.cost,j->obj_flags.rentlevel,
        j->obj_flags.timer,j->oid);
      send_to_char(buf, ch);
      sprintf(buf,"Values 0-3 : [%d] [%d] [%d] [%d]\n\r",
            j->obj_flags.value[0],
            j->obj_flags.value[1],
            j->obj_flags.value[2],
            j->obj_flags.value[3]);
      send_to_char(buf, ch);
      for (i=0;i<MAX_OBJ_AFFECT;++i) {
        sprinttype(j->affected[i].location,apply_types,buf2);
        sprintf(buf,"Affects: %s by %d\n\r",buf2,j->affected[i].modifier);
        send_to_char(buf, ch);      
      }
      return;
    }
    if(cmd==233) k=get_specific_vis(ch,arg1,0);
    else if(cmd==232) k=get_specific_vis(ch,arg1,1);
    else k=get_char_vis(ch,arg1);
    if(k){
      sprintf(buf,"Sex: %d,",k->player.sex);
      sprintf(buf2, " %s, Name: %s\n\r",
         (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
          GET_NAME(k));
      strcat(buf, buf2);
      send_to_char(buf, ch);
      if(IS_NPC(k)) {
        sprintf(buf, "V-Number [%d]\n\r", mob_index[k->nr].virtual);
        send_to_char(buf, ch);
        strcpy(buf,"Short desc: ");
        strcat(buf,(k->player.short_descr ? k->player.short_descr : "None"));
        strcat(buf,"\n\r");
        send_to_char(buf,ch);
      }
      if(k->player.title){
        strcpy(buf,"Title: ");
        strcat(buf, (k->player.title ? k->player.title : "None"));
        strcat(buf,"\n\r");
        send_to_char(buf,ch);
      }
      sprintf(buf,"Level = %d, Alignment = %d\n\r",k->player.level,
                    k->specials.alignment);
      send_to_char(buf, ch);
      strcpy(buf,"Birth: ");
      t=k->player.time.birth; 
      strncat(buf,(char *)ctime((time_t *)&t),24);
      strcat(buf,", Logon: ");
      t=k->player.time.logon; 
      strncat(buf,(char *)ctime((time_t *)&t),24);
     /* SLUG_CHANGE 11-17-96 add the logon time so that it updates */ 
      t=k->player.time.played+time(0)-k->player.time.logon;
      i=t%86400;
      sprintf(buf2,"\n\rPlayed: %d days, %d:%02d:%02d\n\r",
        t/86400,i/3600,(i%3600)/60,(i)%60);
      strcat(buf,buf2);
      send_to_char(buf, ch);
      sprintf(buf,"Age: %d Y, %d M, %d D, %d H.\n\r",
              age(k).year, age(k).month, age(k).day, age(k).hours);
      send_to_char(buf,ch);
      sprintf(buf,"Str:[%d]  Int:[%d]  Wis:[%d]  Dex:[%d]  Con:[%d]\n\r",
        GET_STR(k), GET_INT(k),
        GET_WIS(k), GET_DEX(k), GET_CON(k) );
      send_to_char(buf,ch);
      sprintf(buf,"Mana: %d/%d+%d, Hits: %d/%d+%d, Moves: %d/%d+%d\n\r",
        GET_MANA(k),mana_limit(k),mana_gain(k),
        GET_HIT(k),hit_limit(k),hit_gain(k),
        GET_MOVE(k),move_limit(k),move_gain(k) );
      send_to_char(buf,ch);
      sprintf(buf,"MAG_RES = %d, Saving throws: ",k->specials.magres);
      for(i=0;i<5;++i){
        i2=k->specials.apply_saving_throw[i]+
           GET_SAVING_THROW(GET_LEVEL(k));
        sprintf(buf2,"%4d",i2);
        strcat(buf,buf2);
      }
      strcat(buf,"\n\r");
      send_to_char(buf,ch);
      to_long_long(bufshort,k->points.bank);
      sprintf(buf,
        "AC=%d/10, Gold=%d, Xp=%d, Meta=%d, Hit=%d, Dam=%d, Bank=%s\n\r",
        GET_AC(k), GET_GOLD(k), GET_EXP(k), GET_META(k),
        k->points.hitroll, k->points.damroll, bufshort);
      send_to_char(buf,ch);
      if (IS_NPC(k)) {
        strcpy(buf,"Special: ");
        strcat(buf,(mob_index[k->nr].func ? "Exists\n\r" : "None\n\r"));
        send_to_char(buf, ch);
        sprintf(buf, "NPC Bare Hand Damage %dd%d.\n\r",
          k->specials.damnodice, k->specials.damsizedice);
        send_to_char(buf, ch);
      } else {
        sprintf(buf,"Kills: %d, Deaths: %d\n\r",
          k->points.kills,k->points.deaths);
        send_to_char(buf,ch);
      }
#ifdef VERBOSE_STAT
      sprintf(buf,"Carried weight: %d   Carried items: %d\n\r",
        IS_CARRYING_W(k),
        IS_CARRYING_N(k) );
      send_to_char(buf,ch);
      for(i=0,j=k->carrying;j;j=j->next_content,i++);
      sprintf(buf,"Items in inv: %d, ",i);
      for(i=0,i2=0;i<MAX_WEAR;i++)
        if (k->equipment[i]) i2++;
      sprintf(buf2,"Items in equ: %d\n\r", i2);
      strcat(buf,buf2);
      send_to_char(buf, ch);
#endif
      if(k->desc){
        sprinttype(k->desc->connected,connected_types,buf2);
        strcpy(buf,"Connected: ");
        strcat(buf,buf2);
        sprintf(buf2," %s (%d)\n\r",k->desc->host,k->desc->descriptor);
        strcat(buf,buf2);
        send_to_char(buf,ch);
      }
      if(IS_NPC(k)) {
        sprintf(buf,"NPC flags (%d): ",k->specials.act);
        sprintbit(k->specials.act,action_bits,buf2);
      } else {
        sprintf(buf,"PC flags (%d): ",k->specials.act);
        sprintbit(k->specials.act,player_bits,buf2);
      }
      strcat(buf,buf2);
      strcat(buf,"\n\r");
      send_to_char(buf, ch);

      if(!IS_NPC(k)){
        sprintf(buf, "Thirst: %d, Hunger: %d, Drunk: %d\n\r",
              k->specials.conditions[THIRST],
              k->specials.conditions[FULL],
              k->specials.conditions[DRUNK]);
        sprintf(buf2,"Practices: %d\n\r",k->specials.spells_to_learn);
        strcat(buf,buf2);
        send_to_char(buf, ch);
      }
      sprintf(buf,"Master is '%s'\n\r",
        ((k->master) ? GET_NAME(k->master) : "NOBODY"));
      send_to_char(buf, ch);
      send_to_char("Followers are:\n\r", ch);
      for(fol=k->followers; fol; fol = fol->next)
        act("    $N", FALSE, ch, 0, fol->follower, TO_CHAR);
      /* Showing the bitvector */
      sprintbit(k->specials.affected_by,affected_bits,buf);
      send_to_char("Affected by: ", ch);
      strcat(buf,"\n\r");
      send_to_char(buf, ch);
      if (k->affected) {
        send_to_char("Affecting Spells:\n\r", ch);
        buf[0]=0;
        for(aff = k->affected; aff; aff = aff->next) {
          sprintf(buf2,"%s - %d  ",spells[aff->type-1],aff->duration);
          strcat(buf,buf2);
          if(strlen(buf) > 60){
            strcat(buf,"\n\r");
            send_to_char(buf,ch);
            buf[0]=0;
          }
        }
        if(buf[0]){
          strcat(buf,"\n\r");
          send_to_char(buf, ch);
        }
      }
      return;
    } else {
      send_to_char("No mobile or object by that name in the world\n\r", ch);
    }
  }
}

void do_shutdow(struct char_data *ch, char *argument, int cmd)
{
  send_to_char("If you want to shut something down - say so!\n\r", ch);
}

void do_shutdown(struct char_data *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  char buf[128];

  if (IS_NPC(ch))
    return;
  one_argument(argument, arg);
  if (!*arg) {
    send_to_all("Shutting down immediately.\n\r");
    shutdowngame = 1;
  } else {
    if(isdigit(*arg)){
      shutdownwarn = atoi(arg);
      sprintf(buf,"Game will shutdown in %d tics.\n\r",shutdownwarn);
      send_to_char(buf,ch);
    } else {
      send_to_char("Go shut down someone your own size.\n\r", ch);
    }
  }
}
void do_snoop(struct char_data *ch, char *argument, int cmd)
{
  static char arg[MAX_STRING_LENGTH];
  struct char_data *victim;
  int diff;

  if (!ch->desc)
    return;
  if (IS_NPC(ch))
    return;
  one_argument(argument, arg);
  if(!*arg) {
    send_to_char("Snoop who ?\n\r",ch);
    return;
  }
  if(!(victim=get_char_vis(ch, arg))) {
    send_to_char("No such person around.\n\r",ch);
    return;
  }
  if(!victim->desc) {
    send_to_char("There's no link.. nothing to snoop.\n\r",ch);
    return;
  }
  if(victim == ch) {
    send_to_char("Ok, you just snoop yourself.\n\r",ch);
    if(ch->desc->snoop.snooping) {
        ch->desc->snoop.snooping->desc->snoop.snoop_by = 0;
        ch->desc->snoop.snooping = 0;
    }
    return;
  }
  if(victim->desc->snoop.snoop_by){
    send_to_char("Busy already. \n\r",ch);
    return;
  }
  diff=GET_LEVEL(victim)-GET_LEVEL(ch);
  if(diff >= 0){
    send_to_char("You failed.\n\r",ch);
    return;
  }
  send_to_char("Ok. \n\r",ch);
  if(ch->desc->snoop.snooping)
    ch->desc->snoop.snooping->desc->snoop.snoop_by = 0;
  ch->desc->snoop.snooping = victim;
  victim->desc->snoop.snoop_by = ch;
  return;
}
void do_possess(struct char_data *ch, char *argument, int cmd)
{
  static char arg[MAX_STRING_LENGTH];
  char buf[70];
  struct char_data *victim;

  if (IS_NPC(ch))
    return;
  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("Switch with who?\n\r", ch);
  } else {
    if (!(victim = get_char(arg)))
       send_to_char("They aren't here.\n\r", ch);
    else {
      if (ch == victim) {
        send_to_char("He he he... We are jolly funny today, eh?\n\r", ch);
        return;
      }
      if (!ch->desc||ch->desc->snoop.snoop_by || ch->desc->snoop.snooping) {
        send_to_char(
           "You can't do that, the body is already in use.\n\r",ch);
        return;
      }
      if(victim->desc || (!IS_NPC(victim))) {
        if(GET_LEVEL(victim) > GET_LEVEL(ch))
          send_to_char("They aren't here.\n\r", ch);
        else
          send_to_char(
             "You can't do that, the body is already in use!\n\r",ch);
      } else {
        send_to_char("Ok.\n\r", ch);
        ch->desc->character = victim;
        ch->desc->original = ch;
        victim->desc = ch->desc;
        ch->desc = 0;
      }
    }
  }
}

void do_return(struct char_data *ch, char *argument, int cmd)
{
  static char arg[MAX_STRING_LENGTH];
  char buf[70];

  if(!ch->desc)
    return;

  if(!ch->desc->original)
   { 
    send_to_char("Eh?\n\r", ch);
    return;
  }
  else
  {
    send_to_char("You return to your original body.\n\r",ch);
    ch->desc->character = ch->desc->original;
    ch->desc->original = 0;
    ch->desc->character->desc = ch->desc; 
    ch->desc = 0;
  }
}

void do_force(struct char_data *ch, char *argument, int cmd)
{
  struct descriptor_data *i;
  struct char_data *vict,*next_vict;
  char name[100], to_force[100],buf[100]; 
  int diff;

  if(IS_NPC(ch))
    return;

  half_chop(argument, name, to_force);

  if (!*name || !*to_force)
     send_to_char("Who do you wish to force to do what?\n\r", ch);
  else if(!strcmp("room",name)){
    if(GET_LEVEL(ch) < 9999)
      return;
    for(vict=world[ch->in_room].people; vict; vict = next_vict){
      next_vict=vict->next_in_room;
      if((vict != ch)&&(GET_LEVEL(vict) < GET_LEVEL(ch)))
        command_interpreter(vict,to_force);
    }
    send_to_char("Ok.\n\r", ch);
  } else if (str_cmp("all", name)) {
    if (!(vict = get_char_vis(ch, name)))
      send_to_char("No-one by that name here..\n\r", ch);
    else {
      diff=GET_LEVEL(ch)-GET_LEVEL(vict);
      if ((!IS_NPC(vict)) && (diff <= 0)){
        send_to_char("Oh no you don't!!\n\r", ch);
      } else {
        if(GET_LEVEL(ch) < (IMO+1))
          sprintf(buf, "$n has forced you to '%s'.", to_force);
        else
          buf[0]=0;
        act(buf, FALSE, ch, 0, vict, TO_VICT);
        send_to_char("Ok.\n\r", ch);
        command_interpreter(vict, to_force);
        if(!IS_NPC(vict)){
          sprintf(buf,"FORCE: %s %s %s.",GET_NAME(ch),GET_NAME(vict),to_force);
          log(buf);
        }
      }
    }
  } else { /* force all */
    if(GET_LEVEL(ch) < (IMO+2)){
      send_to_char("Force all's are a bad idea these days.\n\r",ch);
      return;
    }
    for (i = descriptor_list; i; i = i->next)
      if (i->character != ch && !i->connected) {
        vict = i->character;
        if(GET_LEVEL(vict) >= GET_LEVEL(ch)) continue;
        command_interpreter(vict, to_force);
      }
    send_to_char("Ok.\n\r", ch);
  }
}

void do_load(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *mob;
  struct obj_data *obj;
  char type[100], num[100], buf[100];
  int number, r_num;
  extern int top_of_mobt;
  extern int top_of_objt;

  if(IS_NPC(ch) && (GET_LEVEL(ch) < 999))
    return;
  if(GET_LEVEL(ch) <= (IMO+1000)){
    sprintf(buf," %s just tried LOAD %s",GET_NAME(ch),argument);
    do_wiznet(ch,buf,0);
  }
  argument_interpreter(argument, type, num);
  if (!*type || !*num || !isdigit(*num)) {
    send_to_char("Syntax:\n\rload <'char' | 'obj'> <number>.\n\r", ch);
    return;
  }
  if ((number = atoi(num)) < 0) {
    send_to_char("A NEGATIVE number??\n\r", ch);
    return;
  }
  if (is_abbrev(type, "char")) {
    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("There is no monster with that number.\n\r", ch);
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, ch->in_room);
    act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
      0, 0, TO_ROOM);
    act("$n has created $N!", TRUE, ch, 0, mob, TO_ROOM);
    send_to_char("Done.\n\r", ch);
#ifdef LOG_LOADS
    sprintf(buf,"%s loaded char %d",ch->player.name,number);
    log(buf);
#endif
  }
  else if (is_abbrev(type, "obj")) {
    if((GET_LEVEL(ch) < 999)&&(number>17023)&&(number<17100)){
      send_to_char("You may not load that object.\n\r", ch);
      return;
    }
    if ((r_num = real_object(number)) < 0) {
      send_to_char("There is no object with that number.\n\r", ch);
      return;
    }
    obj = read_object(r_num, REAL);
    if(GET_LEVEL(ch) < (IMO+1000))
    if(IS_SET(obj->obj_flags.extra_flags,ITEM_PRIME)){
      send_to_char("That item is not loadable.\n\r",ch);
      extract_obj(obj);
      sprintf(buf,"%s tried to load %d",ch->player.name,number);
      log(buf);
      return;
    }
    if(GET_LEVEL(ch) < (IMO+1000))
    if(IS_SET(obj->obj_flags.extra_flags,ITEM_POOF)){
      send_to_char("Do not load items that vanish.\n\r",ch);
      extract_obj(obj);
      sprintf(buf,"%s tried to load %d",ch->player.name,number);
      log(buf);
      return;
    }
    act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $p!", TRUE, ch, obj, 0, TO_ROOM);
    if(IS_SET(obj->obj_flags.wear_flags,ITEM_TAKE)){
      obj_to_char(obj,ch);
      act("$n snares $p!\n\r",TRUE,ch,obj,0,TO_ROOM);
    } else
      obj_to_room(obj, ch->in_room);
    send_to_char("Ok.\n\r", ch);
#ifdef LOG_LOADS
    sprintf(buf,"%s loaded object %d",ch->player.name,number);
    log(buf);
#endif
  } else
    send_to_char("That'll have to be either 'char' or 'obj'.\n\r", ch);
}
purge_rent(struct char_data *vict)
{
  int i;

  stash_char(vict,0);
  for(i=0;i<MAX_WEAR;i++)
    if(vict->equipment[i]){
      extract_obj(unequip_char(vict,i));
      vict->equipment[i]=0;
    }
  wipe_obj(vict->carrying);
  vict->carrying=0;
  if(vict->desc)
    close_socket(vict->desc);
  extract_char(vict);
}
/* clean a room of all mobiles and objects */
void do_purge(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict, *next_v;
  struct obj_data *obj, *next_o;
  int i;
  char name[100], buf[100];

  if (IS_NPC(ch))
    return;
  one_argument(argument, name);
  if((GET_LEVEL(ch) == 999) && (!strcmp(name,"all"))){
    for(vict=world[ch->in_room].people;vict;vict=next_v){
      next_v = vict->next_in_room;
      if(!IS_NPC(vict) && (vict!=ch)){
        purge_rent(vict);
      }
    }
    send_to_char("As you wish.\n\r",ch);
    return;
  }
  if (*name) {
    if (vict = get_char_room_vis(ch, name)) {
      if(!IS_NPC(vict)){
        if(GET_LEVEL(ch) < GET_LEVEL(vict)){
          sprintf(buf,"%s tried to purge you.\n\r",ch->player.name);
          send_to_char(buf,vict);
          return;
        }
        purge_rent(vict);
        return;
      }
      act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);
      if (IS_NPC(vict)) {
        extract_char(vict);
      }
    } else if(obj=get_obj_in_list_vis(ch,name,world[ch->in_room].contents)) {
      act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else {
      send_to_char("I don't know anyone or anything by that name.\n\r", ch);
      return;
    }
    send_to_char("Ok.\n\r", ch);
  }
  else { /* no argument. clean out the room */
    if (IS_NPC(ch)) {
      send_to_char("Don't... You would only kill yourself..\n\r", ch);
      return;
    }
    act("$n gestures... You are surrounded by scorching flames!", 
      FALSE, ch, 0, 0, TO_ROOM);
    send_to_room("The world seems a little cleaner.\n\r", ch->in_room);
    for (vict = world[ch->in_room].people; vict; vict = next_v) {
      next_v = vict->next_in_room;
      if (IS_NPC(vict))
        extract_char(vict);
    }
    for (obj = world[ch->in_room].contents; obj; obj = next_o) {
      next_o = obj->next_content;
      extract_obj(obj);
    }
  }
}

void roll_abilities(struct char_data *ch)
{
  GET_INT(ch) = GET_WIS(ch) = GET_DEX(ch) = GET_STR(ch) = GET_CON(ch) = 7;
  ch->abilities = ch->tmpabilities;
}

void do_start(struct char_data *ch)
{
  int i, j;
  byte table[5];
  void advance_level(struct char_data *ch);

  send_to_char("Welcome to MUD!\n\r", ch);
  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;
  set_title(ch);
  roll_abilities(ch);
  ch->points.max_hit  = 10;
  ch->points.max_mana = 10;
  ch->points.max_move = 50;
  ch->specials.spells_to_learn = 4;
  advance_level(ch);
  GET_HIT(ch) = hit_limit(ch);
  GET_MANA(ch) = mana_limit(ch);
  GET_MOVE(ch) = move_limit(ch);
  GET_COND(ch,THIRST) = -1;
  GET_COND(ch,FULL) = -1;
  GET_COND(ch,DRUNK) = 0;
  ch->player.time.played = 0;
  ch->player.time.logon = time(0);
  ch->specials.connect_time = time(0); /* SLUG_CHANGE 11-17-96 */
}
void do_restore(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char buf[256];
  int i;
  void update_pos( struct char_data *victim );

  if(IS_NPC(ch) && (GET_LEVEL(ch) < IMO))
    return;
  one_argument(argument,buf);
  if (!*buf)
    send_to_char("Who do you wish to restore?\n\r",ch);
  else
    if(!(victim = get_char_vis(ch,buf)))
      send_to_char("No-one by that name in the world.\n\r",ch);
    else {
      GET_MANA(victim) = GET_MAX_MANA(victim);
      GET_HIT(victim) = GET_MAX_HIT(victim);
      GET_MOVE(victim) = GET_MAX_MOVE(victim);
      if (GET_LEVEL(victim) >= IMO) {
        for (i = 0; i < MAX_SKILLS; i++) {
          victim->skills[i].learned = 100;
          victim->skills[i].used = 0;
        }
        if(GET_LEVEL(victim) > (IMO+IMO)) {
          victim->abilities.intel = 9999;
          victim->abilities.wis =   9999;
          victim->abilities.dex =   9999;
          victim->abilities.str =   9999;
          victim->abilities.con =   9999;
        }
        victim->tmpabilities = victim->abilities;
      }
      update_pos( victim );
      send_to_char("Done.\n\r", ch);
      act("You have been fully healed by $N!", FALSE, victim, 0, ch, TO_CHAR);
      sprintf(buf,"%s was restored by %s",GET_NAME(victim),GET_NAME(ch));
      log(buf);
    }
}

do_noshout(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict;
  struct obj_data *dummy;
  char buf[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;
  one_argument(argument, buf);
  if (!*buf) {
    if (IS_SET(ch->specials.act, PLR_EARMUFFS)) {
      send_to_char("You can now hear shouts again.\n\r", ch);
      REMOVE_BIT(ch->specials.act, PLR_EARMUFFS);
    } else {
      send_to_char("From now on, you won't hear shouts.\n\r", ch);
      SET_BIT(ch->specials.act, PLR_EARMUFFS);
    }
    return;
  }
  if(GET_LEVEL(ch) < IMO)
    return;
  if (!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
    send_to_char("Couldn't find any such creature.\n\r", ch);
  else if (IS_NPC(vict))
    send_to_char("Can't do that to a beast.\n\r", ch);
  else if (GET_LEVEL(vict) > GET_LEVEL(ch))
    act("$E might object to that.. better not.", 0, ch, 0, vict, TO_CHAR);
  else if (IS_SET(vict->specials.act, PLR_NOSHOUT)) {
    send_to_char("You can shout again.\n\r", vict);
    send_to_char("NOSHOUT removed.\n\r", ch);
    REMOVE_BIT(vict->specials.act, PLR_NOSHOUT);
  } else {
    send_to_char("The gods take away your ability to shout!\n\r", vict);
    send_to_char("NOSHOUT set.\n\r", ch);
    SET_BIT(vict->specials.act, PLR_NOSHOUT);
  }
}
void do_wiznet(struct char_data *ch, char *argument, int cmd)
{
  struct descriptor_data *i;
  struct char_data *victim;
  char buf1[MAX_STRING_LENGTH],buf2[MAX_STRING_LENGTH];
  int f,j,k,n;

  if(IS_NPC(ch))
    return;
  sprintf(buf1,"%s: %s\n\r",ch->player.name,argument);
  sprintf(buf2,"Someone: %s\n\r",argument);
  for (i = descriptor_list; i; i = i->next)
    if (!i->connected) {
      if(i->original) continue;
      victim = i->character;
      if(GET_LEVEL(victim) >= IMO){
        if(CAN_SEE(victim,ch))
          send_to_char(buf1,victim);
        else
          send_to_char(buf2,victim);
      }
    }
  send_to_char("Ok.\n\r",ch);
}
void do_noaffect(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict;
  struct obj_data *dummy;
  struct affected_type *hjp;
  char buf[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;
  one_argument(argument, buf);
  if (!*buf){
    send_to_char("Remove affects from whom?\n\r", ch);
    return;
  } else {
    if (!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
      send_to_char("Couldn't find any such creature.\n\r", ch);
    else if (IS_NPC(vict))
      send_to_char("Can't do that to a beast.\n\r", ch);
    else if (GET_LEVEL(vict) > GET_LEVEL(ch))
      act("$E might object to that.. better not.",0,ch,0,vict,TO_CHAR);
    else{
      send_to_char("You are normal again.\n\r", vict);
      for(hjp = vict->affected; hjp; hjp = hjp->next)
        affect_remove( vict, hjp );
    }
  }
  send_to_char("Ok.\n\r",ch);
}
void do_wall(struct char_data *ch, char *argument, int cmd)
{
  struct descriptor_data *i;
  struct char_data *victim;
  char buf[MAX_STRING_LENGTH];

  if(IS_NPC(ch)||(! *argument))
    return;
  if(GET_LEVEL(ch) < (IMO+3))
    sprintf(buf,"%s> %s\n\r",GET_NAME(ch),argument+1);
  else
    sprintf(buf,"%s\n\r",argument+1);
  send_to_all(buf);
  send_to_char("Ok.\n\r",ch);
}

void do_set(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char mess[1024],buf[256],buf2[32],buf3[32],buf4[32];
  int k;
  extern struct command_info cmd_info[];
  extern int freq_ct[300];
  extern char *command[];

  if (IS_NPC(ch))
    return;
  if((GET_LEVEL(ch) < 9000)&&(newsflashflag)){
    if(*argument == ' ')
      argument++;
    sprintf(buf,"News Flash: %s just did SET %s.\n",GET_NAME(ch),argument);
    send_to_all(buf);
  }
  half_chop(argument,buf,buf2);
  if(!*buf){
    sprintf(mess,             "   nokill= %d\n\r",nokillflag);
    sprintf(mess+strlen(mess),"  nosteal= %d\n\r",nostealflag);
    sprintf(mess+strlen(mess),"new chars= %d\n\r",nonewplayers);
    sprintf(mess+strlen(mess),"new conns= %d\n\r",nonewconnect);
    sprintf(mess+strlen(mess),"  noshout= %d\n\r",noshoutflag);
    sprintf(mess+strlen(mess),"hitchance= %d\n\r",hitchance);
    sprintf(mess+strlen(mess),"mischance= %d\n\r",mischance);
    sprintf(mess+strlen(mess),"jokecount= %d\n\r",jokecount);
    sprintf(mess+strlen(mess),"poker max= %d\n\r",maxpokerbet);
    sprintf(mess+strlen(mess),"   helper= %d\n\r",defaulthelper);
    sprintf(mess+strlen(mess),"  bobflag= %d\n\r",bobflag);
    sprintf(mess+strlen(mess),"  bobname= %s\n\r",bobname);
    send_to_char(mess,ch);
    return;
  } else {
    if(strcmp(buf,"momsreadflag")==0){
      one_argument(buf2,buf3);
      if(*buf3)
        momsreadflag=atoi(buf3);
      sprintf(mess,"Mom's read flag is %d.\n\r",momsreadflag);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"melsnumber")==0){
      one_argument(buf2,buf3);
      if(*buf3)
        melsnumber=atoi(buf3);
      sprintf(mess,"Mel's Number is %d.\n\r",melsnumber);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"newsflashflag")==0){
      one_argument(buf2,buf3);
      if(*buf3)
        newsflashflag=atoi(buf3);
      sprintf(mess,"News Flash flag is %d.\n\r",newsflashflag);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"jokecount")==0){
      one_argument(buf2,buf3);
      if(*buf3)
        jokecount=atoi(buf3);
      sprintf(mess,"Jokecount is %d.\n\r",jokecount);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"bobname")==0){
      one_argument(buf2,buf3);
      if(*buf3){
        free(bobname);
        bobname=strdup(buf3);
        if(islower(bobname[0])) bobname[0]=toupper(bobname[0]);
      }
      sprintf(mess,"Bobname is %s.\n\r",bobname);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"bobflag")==0){
      if(GET_LEVEL(ch) < (IMO+5)) return;
      one_argument(buf2,buf3);
      if(*buf3)
        bobflag=atoi(buf3);
      sprintf(mess,"Bobflag is %d.\n\r",bobflag);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"nokill")==0){
      if(GET_LEVEL(ch) < (IMO+5)) return;
      one_argument(buf2,buf3);
      if(*buf3)
        nokillflag=atoi(buf3);
      sprintf(mess,"No-kill flag is %d.\n\r",nokillflag);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"nosteal")==0){
      if(GET_LEVEL(ch) < (IMO+5)) return;
      one_argument(buf2,buf3);
      if(*buf3)
        nostealflag=atoi(buf3);
      sprintf(mess,"No-steal flag is %d.\n\r",nostealflag);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"freeze")==0){
      one_argument(buf2,buf3);
      if(*buf3)
        nonewplayers=atoi(buf3);
      sprintf(mess,"Freeze flag is %d.\n\r",nonewplayers);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"bigfreeze")==0){
      if(GET_LEVEL(ch) < (IMO+5)) return;
      one_argument(buf2,buf3);
      if(*buf3)
        nonewconnect=atoi(buf3);
      sprintf(mess,"Big Freeze flag is %d.\n\r",nonewconnect);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"noshout")==0){
      one_argument(buf2,buf3);
      if(*buf3)
        noshoutflag=atoi(buf3);
      sprintf(mess,"NoShout flag is %d.\n\r",noshoutflag);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"hitchance")==0){
      if(GET_LEVEL(ch) < (IMO+5)) return;
      one_argument(buf2,buf3);
      if(*buf3)
        hitchance=atoi(buf3);
      if(hitchance > 100) hitchance=100;
      sprintf(mess,"Hit chance is %d.\n\r",hitchance);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"mischance")==0){
      if(GET_LEVEL(ch) < (IMO+5)) return;
      one_argument(buf2,buf3);
      if(*buf3)
        mischance=atoi(buf3);
      if(mischance < 0) mischance=0;
      sprintf(mess,"Miss chance is %d.\n\r",mischance);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"helper")==0){
      if(GET_LEVEL(ch) < (IMO+5)) return;
      one_argument(buf2,buf3);
      if(*buf3)
        defaulthelper=atoi(buf3);
      sprintf(mess,"Default helper is %d.\n\r",defaulthelper);
      send_to_char(mess,ch);
      return;
    }
    if(strcmp(buf,"pokermax")==0){
      one_argument(buf2,buf3);
      if(*buf3)
        maxpokerbet=atoi(buf3);
      if(maxpokerbet < 100) maxpokerbet=100;
      sprintf(mess,"Maximum poker bet is %d.\n\r",maxpokerbet);
      send_to_char(mess,ch);
      return;
    }
    if(!(victim = get_char(buf)))
      send_to_char("No-one by that name in the world.\n\r",ch);
    else {
      half_chop(buf2,buf3,buf4);
      k=atoi(buf4);
      if(strcmp("host",buf3)==0){
        strncpy(victim->desc->host,buf4,16);
        return;
      }
      if(strcmp(buf3,"gut")==0)
        GET_GUT(victim)=k;
      else if(strcmp(buf3,"xxx")==0)
        victim->specials.xxx=k;
      else if(strcmp(buf3,"exp")==0)
        victim->points.exp=k;
      else if(strcmp("tit",buf3)==0)
        victim->points.hit=k;
      else if(strcmp("hit",buf3)==0)
        victim->points.hit=victim->points.max_hit=k;
      else if(strcmp("mana",buf3)==0)
        victim->points.mana=victim->points.max_mana=k;
      else if(strcmp("move",buf3)==0)
        victim->points.move=victim->points.max_move=k;
      else if(strcmp("meta",buf3)==0)
        victim->points.metapts=k;
      else if(strcmp("gold",buf3)==0)
        victim->points.gold=k;
      else if(strcmp("bank",buf3)==0)
        victim->points.bank=(long long)k;
      else if(strcmp("hitroll",buf3)==0)
        victim->points.hitroll=k;
      else if(strcmp("damroll",buf3)==0)
        victim->points.damroll=k;
      else if(strcmp("armor",buf3)==0)
        victim->points.armor=k;
      else if(strcmp("mr",buf3)==0)
        victim->specials.magres=k;
      else if(strcmp("lev",buf3)==0){
      if((GET_LEVEL(ch) < 9999) ) return;
        victim->player.level=k;
      } 
      else if(strcmp("align",buf3)==0)
        victim->specials.alignment=k;
      else if(strcmp("str",buf3)==0)
        victim->abilities.str=k;
      else if(strcmp("dex",buf3)==0)
        victim->abilities.dex=k;
      else if(strcmp("wis",buf3)==0)
        victim->abilities.wis=k;
      else if(strcmp("con",buf3)==0)
        victim->abilities.con=k;
      else if(strcmp("int",buf3)==0)
        victim->abilities.intel=k;
      else if(strcmp("pra",buf3)==0)
        victim->specials.spells_to_learn=k;
      else if(strcmp("sex",buf3)==0)
        victim->player.sex=k;
      else if(strcmp("aff",buf3)==0)
        victim->specials.affected_by=k;
      else if(strcmp("act",buf3)==0){
        if(!IS_NPC(victim)){
          victim->specials.act=(k & (~ACT_ISNPC));
          return;
        }
        victim->specials.act=(k | ACT_ISNPC);
      } else
        send_to_char("Huh?\n\r",ch);
      victim->tmpabilities = victim->abilities;
    }
  }
}
void do_invis(struct char_data *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return;
  if(IS_SET(ch->specials.act,PLR_WIZINVIS)){
    REMOVE_BIT(ch->specials.act,PLR_WIZINVIS);
    send_to_char("You are visible again.\n\r",ch);
  } else {
    SET_BIT(ch->specials.act,PLR_WIZINVIS);
    send_to_char("You vanish.\n\r",ch);
  }
  send_to_char("Ok.\n\r",ch);
}
do_banish(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict;
  struct obj_data *dummy;
  char buf[MAX_INPUT_LENGTH];
  int location;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char("Banish whom?\n\r", ch);
  else if (!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
    send_to_char("Couldn't find any such creature.\n\r", ch);
  else if (IS_NPC(vict))
    send_to_char("Can't do that to a beast.\n\r", ch);
  else if (GET_LEVEL(vict) >= IMO)
    send_to_char("It's pointless to banish an immortal.\n\r",ch);
  else if (IS_SET(vict->specials.act, PLR_BANISHED)) {
    REMOVE_BIT(vict->specials.act, PLR_BANISHED);
    send_to_char("You feel forgiven?\n\r", vict);
    act("$N is forgiven.",FALSE,ch,0,vict,TO_CHAR);
  } else {
    SET_BIT(vict->specials.act, PLR_BANISHED);
    for (location = 0; location <= top_of_world; location++)
      if (world[location].number == 6999)
        break;
    if(location == top_of_world){
       send_to_char("Death Room is gone?\n\r",ch);
    } else {
      act("$n disappears in a puff of smoke.",FALSE,vict,0,0,TO_ROOM);
      char_from_room(vict);
      char_to_room(vict,location);
      act("$n appears with an ear-splitting bang.",FALSE,vict,0,0,TO_ROOM);
    }
    send_to_char("You smell fire and brimstone?\n\r", vict);
    act("$N is banished.",FALSE,ch,0,vict,TO_CHAR);
  }
  send_to_char("OK.\n\r",ch);
}
do_flag(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict;
  struct obj_data *dummy;
  char buf[MAX_INPUT_LENGTH];
  int f;

  if (IS_NPC(ch))
    return;
  one_argument(argument, buf);
  if (!*buf)
    send_to_char("Flag whom?\n\r", ch);
  else if (!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
    send_to_char("Couldn't find any such creature.\n\r", ch);
  else if (IS_NPC(vict))
    send_to_char("Can't do that to a beast.\n\r", ch);
  else {
    f= IS_SET(vict->specials.act, PLR_BUILDER) ;
    if (f) {
      REMOVE_BIT(vict->specials.act, PLR_BUILDER);
      send_to_char("Flag removed.\n\r",ch);
    } else {
      SET_BIT(vict->specials.act, PLR_BUILDER);
      send_to_char("Flag set.\n\r",ch);
    }
  }
}
void do_flick(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  struct obj_data *obj;
  char victim_name[240];
  char obj_name[240];
  int eq_pos;

  argument = one_argument(argument, obj_name);
  one_argument(argument, victim_name);
  if (!(victim = get_char_vis(ch, victim_name))) {
    send_to_char("Who?\n\r", ch);
    return;
  } else if (victim == ch) {
    send_to_char("Odd?\n\r", ch);
    return;
  } else if(GET_LEVEL(ch) <= GET_LEVEL(victim)){
    send_to_char("Bad idea.\n\r",ch);
    return;
  }
  if (!(obj = get_obj_in_list(obj_name, victim->carrying))) {
    for (eq_pos = 0; (eq_pos < MAX_WEAR); eq_pos++)
      if(victim->equipment[eq_pos] &&
        (isname(obj_name, victim->equipment[eq_pos]->name))){
        obj = victim->equipment[eq_pos];
        break;
      }
    if (!obj) {
      send_to_char("Can't find that item.\n\r",ch);
      return;
    } else { /* It is equipment */
      obj_to_char(unequip_char(victim, eq_pos), ch);
      send_to_char("Done.\n\r", ch);
    }
  } else {  /* obj found in inventory */
    obj_from_char(obj);
    obj_to_char(obj, ch);
    send_to_char("Done.\n\r", ch);
  }
}
void do_sys(struct char_data *ch, char *argument, int cmd)
{
  char buffer[256];
  int t;
  struct char_data *i;
  struct obj_data *k;
  struct descriptor_data *d;
  static int nits=0,nics=0,nids=0;
  static int timedone=0;
#ifdef RUSAGE
  struct rusage xru;

  if(GET_LEVEL(ch) > IMO){
    getrusage(0,&xru);
    sprintf(buffer,
      "sys time: %d secs\n\rusr time: %d secs\n\rrun time: %d secs\n\r\n\r",
      xru.ru_stime.tv_sec,xru.ru_utime.tv_sec,time(0)-boottime);
    send_to_char(buffer,ch);
  }
#endif
  t=time(0);
  if((GET_LEVEL(ch) > IMO)||((t-timedone) > 60)){
    nits=0;
    for (k=object_list;k;k=k->next) ++nits;
    nics=0;
    for (i=character_list;i;i=i->next) ++nics;
    nids=0;
    for (d=descriptor_list;d;d=d->next) ++nids;
    timedone=t;
  }
  sprintf(buffer,
    "%5d objects\n\r%5d chars\n\r%5d players\n\r%5d rooms\n\r",
    nits,nics,nids,top_of_world);
  send_to_char(buffer,ch);
  sprintf(buffer,"%5d dead monsters\n\r%5d dead players\n\r",
    mobdeaths,pcdeaths);
  send_to_char(buffer,ch);
  sprintf(buffer,"%d drops.\n\r",droptotal);
  send_to_char(buffer,ch);
  sprintf(buffer,"Total output = %d bytes (%d Bps)\n\r",
    grand_total,grand_total/(time(0)-reboottime));
  send_to_char(buffer,ch);
}
void do_oset(struct char_data *ch, char *argument, int cmd)
{
  char buf[256],buf2[256],buf3[256],buf4[256];
  int k;
  struct obj_data *obj;

  if (IS_NPC(ch))
    return;
  if((GET_LEVEL(ch) < 9000)&&(newsflashflag)){
    if(*argument == ' ')
      argument++;
    sprintf(buf,"News Flash: %s just did OSET %s.\n",GET_NAME(ch),argument);
    send_to_all(buf);
  }
  half_chop(argument,buf,buf2);
  if(!*buf){
    send_to_char("What?\n\r",ch);
    return;
  } else {
    if(!(obj = get_obj_vis(ch,buf)))
      send_to_char("Nothing with that name in the world.\n\r",ch);
    else {
      half_chop(buf2,buf3,buf4);
      k=atoi(buf4);
      if(strcmp(buf3,"v0")==0)
        obj->obj_flags.value[0]=k;
      else if(strcmp(buf3,"v1")==0)
        obj->obj_flags.value[1]=k;
      else if(strcmp(buf3,"v2")==0)
        obj->obj_flags.value[2]=k;
      else if(strcmp(buf3,"v3")==0)
        obj->obj_flags.value[3]=k;
      else if(strcmp(buf3,"type")==0)
        obj->obj_flags.type_flag=k;
      else if(strcmp(buf3,"wear")==0)
        obj->obj_flags.wear_flags=k;
      else if(strcmp(buf3,"rent")==0)
        obj->obj_flags.rentlevel=k;
      else if(strcmp(buf3,"extra")==0)
        obj->obj_flags.extra_flags=k;
      else if(strcmp(buf3,"weight")==0)
        obj->obj_flags.weight=k;
      else if(strcmp(buf3,"timer")==0)
        obj->obj_flags.timer=k;
      else if(strcmp(buf3,"a0l")==0)
        obj->affected[0].location=k;
      else if(strcmp(buf3,"a0m")==0)
        obj->affected[0].modifier=k;
      else if(strcmp(buf3,"a1l")==0)
        obj->affected[1].location=k;
      else if(strcmp(buf3,"a1m")==0)
        obj->affected[1].modifier=k;
      else if(strcmp(buf3,"a2l")==0)
        obj->affected[2].location=k;
      else if(strcmp(buf3,"a2m")==0)
        obj->affected[2].modifier=k;
      else
        send_to_char("Huh?\n\r",ch);
    }
  }
}
void do_count(struct char_data *ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  struct char_data *i;
  struct obj_data *k;
  int n;
  extern struct obj_data *object_list;

  one_argument(argument, name);
  n=0;
  if (!*name) {
    send_to_char("What are you looking for?\n\r", ch);
    return;
  }
  for(i=character_list; i; i = i->next)
    if(isname(name, i->player.name) && CAN_SEE(ch, i))
      if(i->in_room != NOWHERE)
        ++n;
  for (k=object_list;k;k=k->next)
    if(isname(name, k->name))
      ++n;
  sprintf(buf,"%d matches\n\r",n);
  send_to_char(buf,ch);
}
void do_join(struct char_data *ch, char *argument, int cmd)
{
  char buf[256],b1[256],b2[256];
  int loc,dir,srm,vrm;

  half_chop(argument,b1,buf);
  one_argument(buf,b2);
  dir=atoi(b1);
  if(cmd==266){
    srm=ch->in_room;
    free(world[srm].dir_option[dir]);
    world[srm].dir_option[dir] = 0;
    return;
  }
  vrm=atoi(b2);
  if(vrm==0){
    send_to_char("To room 0?\n\r",ch);
    return;
  }
  for (loc = 0; loc <= top_of_world; loc++)
    if (world[loc].number == vrm)
      break;
    else if (loc == top_of_world) {
      send_to_char("No room exists with that number.\n\r", ch);
      return;
    }
  if(EXIT(ch,dir)){
    send_to_char("There's already an exit in that direction.\n\r",ch);
    return;
  }
  srm=ch->in_room;
  CREATE(world[srm].dir_option[dir],struct room_direction_data,1);
  world[srm].dir_option[dir]->general_description=0;
  world[srm].dir_option[dir]->keyword=0;
  world[srm].dir_option[dir]->exit_info=0;
  world[srm].dir_option[dir]->key=(-1);
  world[srm].dir_option[dir]->to_room=loc;
  send_to_char("OK.\n\r",ch);
}
void do_joke(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *mob;
  struct obj_data *obj;
  char buf[256],buf2[256];
  int i, v_num, r_num, aflag;

  if (IS_NPC(ch) && (GET_LEVEL(ch) < 999))
    return;
  argument_interpreter(argument, buf, buf2);
  v_num=atoi(buf);
  if(isdigit(buf2[0])){
    aflag=atoi(buf2);
  } else if(isalpha(buf2[0])) {
    if(islower(buf2[0]))
      buf2[0]=toupper(buf2[0]);
    aflag=(-1);
  } else {
    aflag=0;
  }
  if(v_num > 0){
    if((r_num = real_mobile(v_num)) < 0) {
      send_to_char("There is no monster with that number.\n\r", ch);
      return;
    }
    for(i=0;i<jokecount;i++){
      mob = read_mobile(r_num, REAL);
      if(!mob){
        send_to_char("Early exit.\n\r",ch);
      }
      char_to_room(mob,ch->in_room);
      if(aflag >= 0){
        mob->specials.act=(8|aflag);
      } else if(aflag < 0) {
        if(GET_NAME(mob))
          free(GET_NAME(mob));
        GET_NAME(mob)=strdup(buf2);
        if(mob->player.short_descr)
          free(mob->player.short_descr);
        mob->player.short_descr=strdup(buf2);
        sprintf(buf,"%s is standing here.\n\r",buf2);
        if(mob->player.long_descr)
          free(mob->player.long_descr);
        mob->player.long_descr=strdup(buf);
        if(mob->player.description)
          free(mob->player.description);
        mob->player.description=strdup("JEXGIT\n\r");
        mob->specials.act=8;
      }
      do_stand(mob,0,0);
    }
  }
}
static char *loctype[]={
  "EQ", "CA", "IO", "IR"
};
void do_data(struct char_data *ch, char *argument, int cmd)
{
  extern char *command[];
  extern int freq_ct[];
  struct obj_data *o;
  struct char_data *p;
  struct descriptor_data *d;
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int i,j,k,n,flag;
  extern struct zone_data *zone_table;
  extern struct index_data *mob_index, *obj_index;
  extern int top_of_zone_table, top_of_mobt, top_of_objt;

  half_chop(argument,buf,buf2);
  if(*buf == 'a'){
    for (d = descriptor_list; d; d = d->next) {
     if((p=d->character)&&(CAN_SEE(ch,p))){
       sprintf(buf,"%-14s|%7d%7d%7d%|%5d%5d%5d%5d%5d|%4d%4d\n\r",
         GET_NAME(p),GET_MAX_HIT(p),GET_MAX_MANA(p),GET_MAX_MOVE(p),
         GET_STR(p),GET_INT(p),GET_WIS(p),GET_DEX(p),GET_CON(p),
         GET_HITROLL(p),GET_DAMROLL(p));
       send_to_char(buf,ch);
     }
    }
    return;
  } else if(*buf == 'c'){
    k=atoi(buf2);
    for(i=1;i<299;i++){
      if(freq_ct[i] > k){
        sprintf(buf,"%-12.12s %6d\n\r",
          command[i-1],freq_ct[i]);
        send_to_char(buf,ch);
      }
    }
    return;
  } else if(*buf == 'm'){
    k=atoi(buf2);
    for(i=0;i<=top_of_mobt;i++){
      if(mob_index[i].total >= k){
        sprintf(buf,"%5d %-17.17s %6d%6d\n\r",
          mob_index[i].virtual,
          mob_index[i].name ? mob_index[i].name : "???",
          mob_index[i].number,mob_index[i].total);
        send_to_char(buf,ch);
      }
    }
    return;
  } else if(*buf == 'o'){
    k=atoi(buf2);
    for(i=0;i<=top_of_objt;i++){
      if(obj_index[i].number >= k){
        sprintf(buf,"%5d %-17.17s %6d\n\r",
          obj_index[i].virtual,
          obj_index[i].name ? obj_index[i].name : "???",
          obj_index[i].number);
        send_to_char(buf,ch);
      }
    }
    return;
  } else {
    send_to_char("data a,c,z,m,o\n\r",ch);
    return;
  }
  i=0; n=atoi(buf);
  for (o=object_list;o;o=o->next)
    if(obj_index[o->item_number].virtual==n){
      if(o->carried_by) {
        flag=1;
      } else if(o->in_obj) {
        flag=2;
      } else if(o->in_room != NOWHERE) {
        flag=3;
      } else {
        flag=0;
      }
      sprintf(buf,"%2d: %s %3d%3d%3d%3d  %s\n\r",
        ++i,
        o->short_description,
        o->obj_flags.value[0],
        o->obj_flags.value[1],
        o->obj_flags.value[2],
        o->obj_flags.value[3],
        loctype[flag]);
      send_to_char(buf,ch);
    }
  send_to_char("OK.\n\r",ch);
}




