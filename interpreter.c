/* ************************************************************************
*  file: Interpreter.c , Command interpreter module.      Part of DIKUMUD *
*  Usage: Procedures interpreting user command                            *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************* */

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "limits.h"

#define COMMANDO(number,min_pos,pointer,min_level) {      \
  cmd_info[(number)].command_pointer = (pointer);         \
  cmd_info[(number)].minimum_position = (min_pos);        \
  cmd_info[(number)].minimum_level = (min_level); }

#define NOT !
#define AND &&
#define OR ||

#define STATE(d) ((d)->connected)
#define MAX_CMD_LIST 300

extern int baddoms;
extern char baddomain[BADDOMS][BADSTRLEN];
extern int newbeedoms;
extern char newbeedomain[BADDOMS][BADSTRLEN];

extern int freq_ct[MAX_CMD_LIST];
extern char motd[MAX_STRING_LENGTH];
extern struct char_data *character_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
struct command_info cmd_info[MAX_CMD_LIST];


/* external fcntls */

int expand_alias(struct char_data *ch, char *inb, char *outb);
void recover_in_rent(struct char_data *ch);
void unstash_char(struct char_data *ch, char *name);
void stash_char(struct char_data *ch, char *name);
void set_title(struct char_data *ch);
void init_char(struct char_data *ch);
void store_to_char(struct char_file_u *st, struct char_data *ch);
int create_entry(char *name);
int special(struct char_data *ch, int cmd, char *arg);
void log(char *str);

void do_slug_rank(struct char_data *ch, char *arg, int cmd);
void do_heal(struct char_data *ch, char *arg, int cmd);
void do_command_list(struct char_data *ch, char *argument, int cmd);
void do_attack(struct char_data *ch, char *argument, int cmd);
void do_tag(struct char_data *ch, char *argument, int cmd);
void do_xyzzy(struct char_data *ch, char *argument, int cmd);
void do_detonate(struct char_data *ch, char *argument, int cmd);
void do_verybrief(struct char_data *ch, char *argument, int cmd);
void do_shoot(struct char_data *ch, char *argument, int cmd);
void do_move(struct char_data *ch, char *argument, int cmd);
void do_look(struct char_data *ch, char *argument, int cmd);
void do_read(struct char_data *ch, char *argument, int cmd);
void do_say(struct char_data *ch, char *argument, int cmd);
void do_exit(struct char_data *ch, char *argument, int cmd);
void do_snoop(struct char_data *ch, char *argument, int cmd);
void do_quit(struct char_data *ch, char *argument, int cmd);
void do_help(struct char_data *ch, char *argument, int cmd);
void do_who(struct char_data *ch, char *argument, int cmd);
void do_emote(struct char_data *ch, char *argument, int cmd);
void do_echo(struct char_data *ch, char *argument, int cmd);
void do_trans(struct char_data *ch, char *argument, int cmd);
void do_kill(struct char_data *ch, char *argument, int cmd);
void do_stand(struct char_data *ch, char *argument, int cmd);
void do_sit(struct char_data *ch, char *argument, int cmd);
void do_rest(struct char_data *ch, char *argument, int cmd);
void do_sleep(struct char_data *ch, char *argument, int cmd);
void do_wake(struct char_data *ch, char *argument, int cmd);
void do_force(struct char_data *ch, char *argument, int cmd);
void do_get(struct char_data *ch, char *argument, int cmd);
void do_drop(struct char_data *ch, char *argument, int cmd);
void do_news(struct char_data *ch, char *argument, int cmd);
void do_score(struct char_data *ch, char *argument, int cmd);
void do_inventory(struct char_data *ch, char *argument, int cmd);
void do_equipment(struct char_data *ch, char *argument, int cmd);
void do_shout(struct char_data *ch, char *argument, int cmd);
void do_not_here(struct char_data *ch, char *argument, int cmd);
void do_tell(struct char_data *ch, char *argument, int cmd);
void do_wear(struct char_data *ch, char *argument, int cmd);
void do_dress(struct char_data *ch, char *argument, int cmd);
void do_wield(struct char_data *ch, char *argument, int cmd);
void do_grab(struct char_data *ch, char *argument, int cmd);
void do_remove(struct char_data *ch, char *argument, int cmd);
void do_put(struct char_data *ch, char *argument, int cmd);
void do_shutdown(struct char_data *ch, char *argument, int cmd);
void do_join(struct char_data *ch, char *argument, int cmd);
void do_save(struct char_data *ch, char *argument, int cmd);
void do_hit(struct char_data *ch, char *argument, int cmd);
void do_string(struct char_data *ch, char *arg, int cmd);
void do_give(struct char_data *ch, char *arg, int cmd);
void do_stat(struct char_data *ch, char *arg, int cmd);
void do_time(struct char_data *ch, char *arg, int cmd);
void do_weather(struct char_data *ch, char *arg, int cmd);
void do_localtime(struct char_data *ch, char *arg, int cmd);
void do_load(struct char_data *ch, char *arg, int cmd);
void do_purge(struct char_data *ch, char *arg, int cmd);
void do_radio(struct char_data *ch, char *arg, int cmd);
void do_whisper(struct char_data *ch, char *arg, int cmd);
void do_cast(struct char_data *ch, char *arg, int cmd);
void do_at(struct char_data *ch, char *arg, int cmd);
void do_goto(struct char_data *ch, char *arg, int cmd);
void do_ask(struct char_data *ch, char *arg, int cmd);
void do_drink(struct char_data *ch, char *arg, int cmd);
void do_eat(struct char_data *ch, char *arg, int cmd);
void do_pour(struct char_data *ch, char *arg, int cmd);
void do_sip(struct char_data *ch, char *arg, int cmd);
void do_taste(struct char_data *ch, char *arg, int cmd);
void do_order(struct char_data *ch, char *arg, int cmd);
void do_follow(struct char_data *ch, char *arg, int cmd);
void do_rent(struct char_data *ch, char *arg, int cmd);
void do_junk(struct char_data *ch, char *arg, int cmd);
void do_close(struct char_data *ch, char *arg, int cmd);
void do_open(struct char_data *ch, char *arg, int cmd);
void do_lock(struct char_data *ch, char *arg, int cmd);
void do_unlock(struct char_data *ch, char *arg, int cmd);
void do_exits(struct char_data *ch, char *arg, int cmd);
void do_enter(struct char_data *ch, char *arg, int cmd);
void do_leave(struct char_data *ch, char *arg, int cmd);
void do_flee(struct char_data *ch, char *arg, int cmd);
void do_sneak(struct char_data *ch, char *arg, int cmd);
void do_hide(struct char_data *ch, char *arg, int cmd);
void do_backstab(struct char_data *ch, char *arg, int cmd);
void do_pick(struct char_data *ch, char *arg, int cmd);
void do_steal(struct char_data *ch, char *arg, int cmd);
void do_bash(struct char_data *ch, char *arg, int cmd);
void do_rescue(struct char_data *ch, char *arg, int cmd);
void do_grescue(struct char_data *ch, char *arg, int cmd);
void do_kick(struct char_data *ch, char *arg, int cmd);
void do_examine(struct char_data *ch, char *arg, int cmd);
void do_info(struct char_data *ch, char *arg, int cmd);
void do_users(struct char_data *ch, char *arg, int cmd);
void do_where(struct char_data *ch, char *arg, int cmd);
void do_locate(struct char_data *ch, char *arg, int cmd);
void do_levels(struct char_data *ch, char *arg, int cmd);
void do_brief(struct char_data *ch, char *arg, int cmd);
void do_wiznet(struct char_data *ch, char *arg, int cmd);
void do_group(struct char_data *ch, char *arg, int cmd);
void do_restore(struct char_data *ch, char *arg, int cmd);
void do_return(struct char_data *ch, char *argument, int cmd);
void do_possess(struct char_data *ch, char *argument, int cmd);
void do_quaff(struct char_data *ch, char *argument, int cmd);
void do_recite(struct char_data *ch, char *argument, int cmd);
void do_use(struct char_data *ch, char *argument, int cmd);
void do_flag(struct char_data *ch, char *argument, int cmd);
void do_noshout(struct char_data *ch, char *argument, int cmd);
void do_wizhelp(struct char_data *ch, char *argument, int cmd);
void do_compact(struct char_data *ch, char *argument, int cmd);
void do_action(struct char_data *ch, char *arg, int cmd);
void do_practice(struct char_data *ch, char *arg, int cmd);
void do_flick(struct char_data *ch, char *arg, int cmd);
void do_wall(struct char_data *ch, char *arg, int cmd);
void do_set(struct char_data *ch, char *arg, int cmd);
void do_police(struct char_data *ch, char *arg, int cmd);
void do_wizlock(struct char_data *ch, char *arg, int cmd);
void do_noaffect(struct char_data *ch, char *arg, int cmd);
void do_invis(struct char_data *ch, char *arg, int cmd);
void do_notell(struct char_data *ch, char *arg, int cmd);
void do_banish(struct char_data *ch, char *arg, int cmd);
void do_report(struct char_data *ch, char *arg, int cmd);
void do_reload(struct char_data *ch, char *arg, int cmd);
void do_recharge(struct char_data *ch, char *arg, int cmd);
void do_checkrent(struct char_data *ch, char *arg, int cmd);
void do_evaluate(struct char_data *ch, char *arg, int cmd);
void do_bank(struct char_data *ch, char *arg, int cmd);
void do_sys(struct char_data *ch, char *arg, int cmd);
void do_extractrent(struct char_data *ch, char *arg, int cmd);
void do_replacerent(struct char_data *ch, char *arg, int cmd);
void do_newlock(struct char_data *ch, char *arg, int cmd);
void do_dig(struct char_data *ch, char *arg, int cmd);
void do_piss(struct char_data *ch, char *arg, int cmd);
void do_puke(struct char_data *ch, char *arg, int cmd);
void do_mod(struct char_data *ch, char *arg, int cmd);
void do_oset(struct char_data *ch, char *arg, int cmd);
void do_spellinfo(struct char_data *ch, char *arg, int cmd);
void do_title(struct char_data *ch, char *arg, int cmd);
void do_channel(struct char_data *ch, char *arg, int cmd);
void do_talk(struct char_data *ch, char *arg, int cmd);
void do_count(struct char_data *ch, char *arg, int cmd);
void do_assist(struct char_data *ch, char *arg, int cmd);
void do_gtell(struct char_data *ch, char *arg, int cmd);
void do_split(struct char_data *ch, char *arg, int cmd);
void do_rename(struct char_data *ch, char *arg, int cmd);
void do_joke(struct char_data *ch, char *arg, int cmd);
void do_reassign(struct char_data *ch, char *arg, int cmd);
void do_data(struct char_data *ch, char *arg, int cmd);
void do_identify(struct char_data *ch, char *arg, int cmd);
void do_throw(struct char_data *ch, char *arg, int cmd);
void do_switch(struct char_data *ch, char *arg, int cmd);
void do_scan(struct char_data *ch, char *arg, int cmd);
void do_reset(struct char_data *ch, char *arg, int cmd);
void do_areas(struct char_data *ch, char *arg, int cmd);
void do_find(struct char_data *ch, char *arg, int cmd);
void do_alias(struct char_data *ch, char *arg, int cmd);
void do_unalias(struct char_data *ch, char *arg, int cmd);
void do_finger(struct char_data *ch, char *arg, int cmd);

char *command[]=
{ "north",        /* 1 */
  "east",
  "south",
  "west",
  "up",
  "down",
  "report",
  "exits",
  "backstab",
  "get",
  "drink",       /* 11 */
  "eat",
  "wear",
  "wield",
  "look",
  "score",
  "say",
  "shout",
  "tell",
  "inventory",
  "qui",         /* 21 */
  "bounce",
  "smile",
  "dance",
  "kill",
  "cackle",
  "laugh",
  "giggle",
  "shake",
  "puke",
  "growl",       /* 31 */    
  "scream",
  "grescue",
  "comfort",
  "nod",
  "sigh",
  "'",
  "help",
  "who",
  "emote",
  "assist",        /* 41 */
  "stand",
  "sit",
  "rest",
  "sleep",
  "wake",
  "force",
  "talk",
  "hug",
  "snuggle",
  "cuddle",       /* 51 */
  "nuzzle",
  "cry",
  "news",
  "equipment",
  "buy",
  "sell",
  "value",
  "list",
  "drop",
  "goto",         /* 61 */
  "weather",
  "read",
  "pour",
  "grab",
  "remove",
  "put",
  "rename",
  "save",
  "hit",
  "string",      /* 71 */
  "give",
  "quit",
  "stat",
  "bellow",
  "time",
  "load",
  "purge",
  "shutdown",
  "scratch",
  "radio",        /* 81 */
  "replacerent",
  "whisper",
  "cast",
  "at",
  "ask",
  "order",
  "sip",
  "taste",
  "snoop",
  "follow",      /* 91 */
  "rent",
  "stats",
  "poke",
  "advance",
  "bird",
  "grin",
  "bow",
  "open",
  "close",
  "lock",        /* 101 */
  "unlock",
  "leave",
  "applaud",
  "blush",
  "burp",
  "chuckle",
  "clap",
  "cough",
  "curtsey",
  "fart",        /* 111 */
  "flip",
  "fondle",
  "frown",
  "gasp",
  "glare",
  "groan",
  "grope",
  "hiccup",
  "lick",
  "love",        /* 121 */
  "moan",
  "nibble",
  "pout",
  "purr",
  "ruffle",
  "shiver",
  "shrug",
  "sing",
  "slap",
  "smirk",       /* 131 */
  "snap",
  "sneeze",
  "snicker",
  "sniff",
  "snore",
  "spit",
  "squeeze",
  "stare",
  "strut",
  "thank",       /* 141 */
  "twiddle",
  "wave",
  "whistle",
  "wiggle",
  "wink",
  "yawn",
  "snowball",
  "extractrent",
  "hold",
  "flee",        /* 151 */
  "sneak",
  "hide",
  "kiss",
  "pick",
  "steal",
  "bash",
  "rescue",
  "kick",
  "french",
  "arf",        /* 161 */
  "massage",
  "tickle",
  "practice",
  "pat",
  "examine",
  "take",
  "info",
  "sulk",
  "practise",
  "curse",       /* 171 */
  "use",
  "where",
  "levels",
  "bob",
  "pray",
  ",",
  "beg",
  "salute",
  "cringe",
  "daydream",    /* 181 */
  "fume",
  "grovel",
  "hop",
  "nudge",
  "peer",
  "point",
  "ponder",
  "drool",
  "snarl",
  "spank",       /* 191 */
  "shoot",
  "bark",
  "taunt",
  "think",
  "whine",
  "worship",
  "yodel",    /* 198 */
  "brief",
  "wiznet",
  "cost",    /* 201 */
  "group",
  "restore",
  "return",
  "possess",      /* 205 */
  "quaff",
  "recite",
  "users",
  "flag",
  "noshout",
  "wizhelp",   /* 211 */
  "goose",
  "compact",
  "flick",
  "wall",
  "set",
  "police",
  "wizlock",
  "noaffect",
  "invis",
  "notell",
  "banish",
  "reload",
  "enter",
  "checkrent",   /* 225 */
  "evaluate",
  "balance",
  "deposit",
  "withdraw",
  "sys",
  "log",
  "mstat",
  "pstat",
  "write",
  "bet",        /* 235 */
  "newlock",
  "glance",
  "dig",
  "oset",
  "spellinfo",  /* 240 */
  "title",
  "watch",
  "join",
  "channel",
  "transfer",
  "recharge",
  "card",
  "nosummon",
  "fold",
  "count",   /* 250 */
  "echo",
  "gtell",
  "congratulate",
  "split",
  "xyzzy",
  "socials",
  "commands",
  "detonate",
  "verybrief",
  "joke",      /* 260 */
  "piss",
  "howl",
  "mod",
  "stick",
  "junk",
  "unjoin",
  "reassign",
  "data",
  "localtime",
  "mwhere",     /* 270 */
  "pwhere",
  "locate",
  "identify",
  "attack",
  "tag",
  "norelocate",
  "rank",
  "heal",
  "spend",
  "throw",  /* 280 */
  "auto",
  "scan",
  "ptell",
  "reset",
  "dress",
  "autoexits",
  "autoloot",
  "switch",
  "cheer",
  "blink",  /* 290 */
  "puzzle",
  "repeat",
  "find",
  "alias",
  "unalias",
  "areas",
  "finger",
  "appraise",
  "\n"
};


char *fill[]=
{ "in",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

int search_block(char *arg, char **list, bool exact)
{
  register int i,l;

  /* Make into lower case, and get length of string */
  for(l=0; *(arg+l); l++)
    *(arg+l)=LOWER(*(arg+l));

  if (exact) {
    for(i=0; **(list+i) != '\n'; i++)
      if (!strcmp(arg, *(list+i)))
        return(i);
  } else {
    if (!l)
      l=1; /* Avoid "" to match the first available string */
    for(i=0; **(list+i) != '\n'; i++)
      if (!strncmp(arg, *(list+i), l))
        return(i);
  }

  return(-1);
}


int old_search_block(char *argument,int begin,int length,char **list,int mode)
{
  int guess, found, search;
        
  /* If the word contain 0 letters, then a match is already found */
  found = (length < 1);
  guess = 0;
  /* Search for a match */
  if(mode)
  while ( NOT found AND *(list[guess]) != '\n' ) {
    found=(length==strlen(list[guess]));
    for(search=0;( search < length AND found );search++)
      found=(*(argument+begin+search)== *(list[guess]+search));
    guess++;
  } else {
    while ( NOT found AND *(list[guess]) != '\n' ) {
      found=1;
      for(search=0;( search < length AND found );search++)
        found=(*(argument+begin+search)== *(list[guess]+search));
      guess++;
    }
  }
  return ( found ? guess : -1 ); 
}

#define NFEM 12

static char *FunnyErrorMess[]={
  "Huh?\n\r",
  "Try that again?\n\r",
  "What the heck does that mean?\n\r",
  "Argle bargle glop-glyph?\n\r",
  "You new at this game?\n\r",
  "I'll go look that one up.\n\r",
  "In English, please.\n\r",
  "Take two aspirin and call me in the morning.\n\r",
  "You type much?\n\r",
  "What?\n\r",
  "Hello? Hello? Anybody out there?\n\r",
  "Same to you, pal!\n\r"
};

int command_interpreter(struct char_data *ch, char *argument) 
{
  int look_at, cmd, begin;
  char *p,buf[MAX_STRING_LENGTH];

  if(ch->alp && expand_alias(ch,argument,buf)){
    p = buf;
  } else {
    p = argument;
  }
  REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);
  
  for (begin = 0 ; (*(p + begin ) == ' ' ) ; begin++ );
 
  for (look_at = 0; *(p + begin + look_at ) > ' ' ; look_at++)
    *(p+begin+look_at) = LOWER(*(p+begin+look_at));

  cmd = old_search_block(p,begin,look_at,command,0);
  
  if(cmd >= 0) freq_ct[cmd]++;
  if (!cmd)
    return(1);
  if ( cmd>0 && GET_LEVEL(ch)<cmd_info[cmd].minimum_level ) {
    send_to_char(FunnyErrorMess[number(0,NFEM-1)],ch);
    ch->specials.argles++;	/* SLUG_CHANGE 11-17-96 */ 
    return(1);
  }
  if ( cmd>0 && (cmd_info[cmd].command_pointer != 0)) {
    if( GET_POS(ch) < cmd_info[cmd].minimum_position )
      switch(GET_POS(ch)) {
        case POSITION_DEAD:
          send_to_char("Lie still; you are DEAD!!! :-( \n\r", ch);
        break;
        case POSITION_INCAP:
        case POSITION_MORTALLYW:
          send_to_char(
            "You are in a pretty bad shape, unable to do anything!\n\r",
            ch);
        break;
        case POSITION_STUNNED:
          send_to_char(
          "All you can do right now, is think about the stars!\n\r", ch);
        break;
        case POSITION_SLEEPING:
          send_to_char("In your dreams, or what?\n\r", ch);
        break;
        case POSITION_RESTING:
          send_to_char("Nah... You feel too relaxed to do that..\n\r",
            ch);
        break;
        case POSITION_SITTING:
          send_to_char("Maybe you should get on your feet first?\n\r",ch);
        break;
        case POSITION_FIGHTING:
          send_to_char("No way! You are fighting for your life!\n\r", ch);
        break;
      } else {
      if(special(ch, cmd, p + begin + look_at))
        return(1);  

	ch->specials.commands++;	/*SLUG_CHANGE 11-17-96 */
      ((*cmd_info[cmd].command_pointer)
      (ch, p + begin + look_at, cmd));
    }
    return(1);
  }
  if ( cmd>0 && (cmd_info[cmd].command_pointer == 0))
    send_to_char(
    "Sorry, but that command has yet to be implemented...\n\r", ch);
  else { 
    send_to_char(FunnyErrorMess[number(0,NFEM-1)],ch);
    ch->specials.argles++; /* SLUG_CHANGE 11-17-96 */
  }
  return(1);
}

void argument_interpreter(char *argument,char *first_arg,char *second_arg )
{
        int look_at, found, begin;

        found = begin = 0;

        do
        {
                /* Find first non blank */
                for ( ;*(argument + begin ) == ' ' ; begin++);

                /* Find length of first word */
                for ( look_at=0; *(argument+begin+look_at)> ' ' ; look_at++)

                        /* Make all letters lower case,
                           AND copy them to first_arg */
                        *(first_arg + look_at) =
                        LOWER(*(argument + begin + look_at));

                *(first_arg + look_at)='\0';
                begin += look_at;

        }
        while( fill_word(first_arg));

        do
        {
                /* Find first non blank */
                for ( ;*(argument + begin ) == ' ' ; begin++);

                /* Find length of first word */
                for ( look_at=0; *(argument+begin+look_at)> ' ' ; look_at++)

                        /* Make all letters lower case,
                           AND copy them to second_arg */
                        *(second_arg + look_at) =
                        LOWER(*(argument + begin + look_at));

                *(second_arg + look_at)='\0';
                begin += look_at;

        }
        while( fill_word(second_arg));
}

int is_number(char *str)
{
  int look_at;

  if(*str=='\0')
    return(0);

  for(look_at=0;*(str+look_at) != '\0';look_at++)
    if((*(str+look_at)<'0')||(*(str+look_at)>'9'))
      return(0);
  return(1);
}

/*  Quinn substituted a new one-arg for the old one.. I thought returning a 
    char pointer would be neat, and avoiding the func-calls would save a
    little time... If anyone feels pissed, I'm sorry.. Anyhow, the code is
    snatched from the old one, so it outta work..

void one_argument(char *argument,char *first_arg )
{
  static char dummy[MAX_STRING_LENGTH];
  argument_interpreter(argument,first_arg,dummy);
}

*/


/* find the first sub-argument of a string, return pointer to first char in
   primary argument, following the sub-arg                  */
char *one_argument(char *argument, char *first_arg )
{
  int found, begin, look_at;

        found = begin = 0;

        do
        {
                /* Find first non blank */
                for ( ;isspace(*(argument + begin)); begin++);

                /* Find length of first word */
                for (look_at=0; *(argument+begin+look_at) > ' ' ; look_at++)

                        /* Make all letters lower case,
                           AND copy them to first_arg */
                        *(first_arg + look_at) =
                        LOWER(*(argument + begin + look_at));

                *(first_arg + look_at)='\0';
    begin += look_at;
  }
        while (fill_word(first_arg));

  return(argument+begin);
}
  
  






int fill_word(char *argument)
{
  return ( search_block(argument,fill,TRUE) >= 0);
}





/* determine if a given string is an abbreviation of another */
int is_abbrev(char *arg1, char *arg2)
{
  if (!*arg1)
     return(0);

  for (; *arg1; arg1++, arg2++)
     if (LOWER(*arg1) != LOWER(*arg2))
        return(0);

  return(1);
}

/* return first 'word' plus trailing substring of input string */
void half_chop(char *string, char *arg1, char *arg2)
{
  for (; isspace(*string); string++);
  for (; !isspace(*arg1 = *string) && *string; string++, arg1++);
  *arg1 = '\0';
  for (; isspace(*string); string++);
  for (; *arg2 = *string; string++, arg2++);
  *arg2 = '\0';
}

int special(struct char_data *ch, int cmd, char *arg)
{
  register struct obj_data *i;
  register struct char_data *k;
  int j;

  /* special in room? */
  if (world[ch->in_room].funct)
     if ((*world[ch->in_room].funct)(ch, cmd, arg))
        return(1);
  /* special in equipment list? */
  for (j = 0; j <= (MAX_WEAR - 1); j++)
     if (ch->equipment[j] && ch->equipment[j]->item_number>=0)
        if (obj_index[ch->equipment[j]->item_number].func)
           if ((*obj_index[ch->equipment[j]->item_number].func)
              (ch, cmd, arg))
              return(1);
  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (i->item_number>=0)
      if (obj_index[i->item_number].func)
          if ((*obj_index[i->item_number].func)(ch, cmd, arg))
             return(1);
  /* special in mobile present? */
  for (k = world[ch->in_room].people; k; k = k->next_in_room)
     if ( IS_MOB(k) )
        if (mob_index[k->nr].func)
           if ((*mob_index[k->nr].func)(ch, cmd, arg))
              return(1);
  /* special in object present? */
  for (i = world[ch->in_room].contents; i; i = i->next_content)
     if (i->item_number>=0)
        if (obj_index[i->item_number].func)
           if ((*obj_index[i->item_number].func)(ch, cmd, arg))
              return(1);
  return(0);
}

void assign_command_pointers ( void )
{
  int position;

  for (position = 0 ; position < MAX_CMD_LIST; position++)
    cmd_info[position].command_pointer = 0;

  COMMANDO(1,POSITION_STANDING,do_move,0);
  COMMANDO(2,POSITION_STANDING,do_move,0);
  COMMANDO(3,POSITION_STANDING,do_move,0);
  COMMANDO(4,POSITION_STANDING,do_move,0);
  COMMANDO(5,POSITION_STANDING,do_move,0);
  COMMANDO(6,POSITION_STANDING,do_move,0);
  COMMANDO(7,POSITION_SLEEPING,do_report,0);
  COMMANDO(8,POSITION_RESTING,do_exits,0);
  COMMANDO(9,POSITION_STANDING,do_backstab,1);  
  COMMANDO(10,POSITION_RESTING,do_get,0);
  COMMANDO(11,POSITION_RESTING,do_drink,0);
  COMMANDO(12,POSITION_RESTING,do_eat,0);
  COMMANDO(13,POSITION_RESTING,do_wear,0);
  COMMANDO(14,POSITION_RESTING,do_wield,0);
  COMMANDO(15,POSITION_RESTING,do_look,0);
  COMMANDO(16,POSITION_DEAD,do_score,0);
  COMMANDO(17,POSITION_RESTING,do_say,0);
  COMMANDO(18,POSITION_RESTING,do_shout,2);
  COMMANDO(19,POSITION_RESTING,do_tell,0);
  COMMANDO(20,POSITION_DEAD,do_inventory,0);
  COMMANDO(22,POSITION_STANDING,do_action,0);
  COMMANDO(23,POSITION_RESTING,do_action,0);
  COMMANDO(24,POSITION_STANDING,do_action,0);
  COMMANDO(25,POSITION_FIGHTING,do_kill,0);
  COMMANDO(26,POSITION_RESTING,do_action,0);
  COMMANDO(27,POSITION_RESTING,do_action,0);
  COMMANDO(28,POSITION_RESTING,do_action,0);
  COMMANDO(29,POSITION_RESTING,do_action,0);
  COMMANDO(30,POSITION_STANDING,do_puke,1);
  COMMANDO(31,POSITION_RESTING,do_action,0);
  COMMANDO(32,POSITION_RESTING,do_action,0);
  COMMANDO(33,POSITION_FIGHTING,do_grescue,0);
  COMMANDO(34,POSITION_RESTING,do_action,0);
  COMMANDO(35,POSITION_RESTING,do_action,0);
  COMMANDO(36,POSITION_RESTING,do_action,0);
  COMMANDO(37,POSITION_RESTING,do_say,0);
  COMMANDO(38,POSITION_DEAD,do_help,0);
  COMMANDO(39,POSITION_DEAD,do_who,0);
  COMMANDO(40,POSITION_SLEEPING,do_emote,1);
  COMMANDO(41,POSITION_STANDING,do_assist,1);
  COMMANDO(42,POSITION_RESTING,do_stand,0);
  COMMANDO(43,POSITION_RESTING,do_sit,0);
  COMMANDO(44,POSITION_RESTING,do_rest,0);
  COMMANDO(45,POSITION_SLEEPING,do_sleep,0);
  COMMANDO(46,POSITION_SLEEPING,do_wake,0);
  COMMANDO(47,POSITION_SLEEPING,do_force,IMO+1500);
  COMMANDO(48,POSITION_SLEEPING,do_talk,1);
  COMMANDO(49,POSITION_RESTING,do_action,0);
  COMMANDO(50,POSITION_RESTING,do_action,0);
  COMMANDO(51,POSITION_RESTING,do_action,0);
  COMMANDO(52,POSITION_RESTING,do_action,0);
  COMMANDO(53,POSITION_RESTING,do_action,0);
  COMMANDO(54,POSITION_SLEEPING,do_news,0);
  COMMANDO(55,POSITION_SLEEPING,do_equipment,0);
  COMMANDO(56,POSITION_STANDING,do_not_here,0);
  COMMANDO(57,POSITION_STANDING,do_not_here,0);
  COMMANDO(58,POSITION_STANDING,do_not_here,0);
  COMMANDO(59,POSITION_STANDING,do_not_here,0);
  COMMANDO(60,POSITION_RESTING,do_drop,0);
  COMMANDO(61,POSITION_DEAD,do_goto,IMO);
  COMMANDO(62,POSITION_RESTING,do_weather,0);
  COMMANDO(63,POSITION_RESTING,do_read,0);
  COMMANDO(64,POSITION_STANDING,do_pour,0);
  COMMANDO(65,POSITION_RESTING,do_grab,0);
  COMMANDO(66,POSITION_DEAD,do_remove,0);
  COMMANDO(67,POSITION_RESTING,do_put,0);
  COMMANDO(68,POSITION_SLEEPING,do_rename,10);
  COMMANDO(69,POSITION_SLEEPING,do_save,0);
  COMMANDO(70,POSITION_FIGHTING,do_hit,0);
  COMMANDO(71,POSITION_SLEEPING,do_string,IMO+1);
  COMMANDO(72,POSITION_RESTING,do_give,0);
  COMMANDO(73,POSITION_DEAD,do_quit,0);
  COMMANDO(74,POSITION_DEAD,do_stat,IMO+1);
  COMMANDO(75,POSITION_DEAD,do_action,0);
  COMMANDO(76,POSITION_DEAD,do_time,0);
  COMMANDO(77,POSITION_DEAD,do_load,IMO+1);
  COMMANDO(78,POSITION_DEAD,do_purge,IMO+1);
  COMMANDO(79,POSITION_DEAD,do_shutdown,9999);
  COMMANDO(80,POSITION_RESTING,do_action,0);
  COMMANDO(81,POSITION_DEAD,do_radio,2);
  COMMANDO(82,POSITION_SLEEPING,do_replacerent,IMO+1500);
  COMMANDO(83,POSITION_RESTING,do_whisper,0);
  COMMANDO(84,POSITION_SITTING,do_cast,1);
  COMMANDO(85,POSITION_DEAD,do_at,IMO);
  COMMANDO(86,POSITION_RESTING,do_ask,0);
  COMMANDO(87,POSITION_RESTING,do_order,1);
  COMMANDO(88,POSITION_RESTING,do_sip,0);
  COMMANDO(89,POSITION_RESTING,do_taste,0);
  COMMANDO(90,POSITION_DEAD,do_snoop,IMO+1);
  COMMANDO(91,POSITION_RESTING,do_follow,0);
  COMMANDO(92,POSITION_STANDING,do_rent,1);
  COMMANDO(93,POSITION_RESTING,do_sys,100);
  COMMANDO(94,POSITION_RESTING,do_action,0);
  COMMANDO(95,POSITION_STANDING,do_not_here,1);
  COMMANDO(96,POSITION_SITTING,do_action,0);
  COMMANDO(97,POSITION_RESTING,do_action,0);
  COMMANDO(98,POSITION_STANDING,do_action,0);
  COMMANDO(99,POSITION_SITTING,do_open,0);
  COMMANDO(100,POSITION_SITTING,do_close,0);
  COMMANDO(101,POSITION_SITTING,do_lock,0);
  COMMANDO(102,POSITION_STANDING,do_unlock,0);
  COMMANDO(103,POSITION_STANDING,do_leave,0);
  COMMANDO(104,POSITION_RESTING,do_action,0);
  COMMANDO(105,POSITION_RESTING,do_action,0);
  COMMANDO(106,POSITION_RESTING,do_action,0);
  COMMANDO(107,POSITION_RESTING,do_action,0);
  COMMANDO(108,POSITION_RESTING,do_action,0);
  COMMANDO(109,POSITION_RESTING,do_action,0);
  COMMANDO(110,POSITION_STANDING,do_action,0);
  COMMANDO(111,POSITION_RESTING,do_action,0);
  COMMANDO(112,POSITION_STANDING,do_action,0);
  COMMANDO(113,POSITION_RESTING,do_action,0);
  COMMANDO(114,POSITION_RESTING,do_action,0);
  COMMANDO(115,POSITION_RESTING,do_action,0);
  COMMANDO(116,POSITION_RESTING,do_action,0);
  COMMANDO(117,POSITION_RESTING,do_action,0);
  COMMANDO(118,POSITION_RESTING,do_action,0);
  COMMANDO(119,POSITION_RESTING,do_action,0);
  COMMANDO(120,POSITION_RESTING,do_action,0);
  COMMANDO(121,POSITION_RESTING,do_action,0);
  COMMANDO(122,POSITION_RESTING,do_action,0);
  COMMANDO(123,POSITION_RESTING,do_action,0);
  COMMANDO(124,POSITION_RESTING,do_action,0);
  COMMANDO(125,POSITION_RESTING,do_action,0);
  COMMANDO(126,POSITION_STANDING,do_action,0);
  COMMANDO(127,POSITION_RESTING,do_action,0);
  COMMANDO(128,POSITION_RESTING,do_action,0);
  COMMANDO(129,POSITION_RESTING,do_action,0);
  COMMANDO(130,POSITION_RESTING,do_action,0);
  COMMANDO(131,POSITION_RESTING,do_action,0);
  COMMANDO(132,POSITION_RESTING,do_action,0);
  COMMANDO(133,POSITION_RESTING,do_action,0);
  COMMANDO(134,POSITION_RESTING,do_action,0);
  COMMANDO(135,POSITION_RESTING,do_action,0);
  COMMANDO(136,POSITION_SLEEPING,do_action,0);
  COMMANDO(137,POSITION_RESTING,do_action,0);
  COMMANDO(138,POSITION_RESTING,do_action,0);
  COMMANDO(139,POSITION_RESTING,do_action,0);
  COMMANDO(140,POSITION_STANDING,do_action,0);
  COMMANDO(141,POSITION_RESTING,do_action,0);
  COMMANDO(142,POSITION_RESTING,do_action,0);
  COMMANDO(143,POSITION_RESTING,do_action,0);
  COMMANDO(144,POSITION_RESTING,do_action,0);
  COMMANDO(145,POSITION_STANDING,do_action,0);
  COMMANDO(146,POSITION_RESTING,do_action,0);
  COMMANDO(147,POSITION_RESTING,do_action,0);
  COMMANDO(148,POSITION_STANDING,do_action,7);
  COMMANDO(149,POSITION_SLEEPING,do_extractrent,IMO+1500);
  COMMANDO(150,POSITION_RESTING,do_grab,1);
  COMMANDO(151,POSITION_FIGHTING,do_flee,1);  
  COMMANDO(152,POSITION_STANDING,do_sneak,1);  
  COMMANDO(153,POSITION_RESTING,do_hide,1);  
  COMMANDO(154,POSITION_RESTING,do_action,0);
  COMMANDO(155,POSITION_STANDING,do_pick,1);  
  COMMANDO(156,POSITION_STANDING,do_steal,1);  
  COMMANDO(157,POSITION_FIGHTING,do_bash,1);  
  COMMANDO(158,POSITION_FIGHTING,do_rescue,1);
  COMMANDO(159,POSITION_FIGHTING,do_kick,1);
  COMMANDO(160,POSITION_RESTING,do_action,0);
  COMMANDO(161,POSITION_RESTING,do_action,0);
  COMMANDO(162,POSITION_RESTING,do_action,0);
  COMMANDO(163,POSITION_RESTING,do_action,0);
  COMMANDO(164,POSITION_RESTING,do_practice,1);
  COMMANDO(165,POSITION_RESTING,do_action,0);
  COMMANDO(166,POSITION_RESTING,do_examine,0);
  COMMANDO(167,POSITION_RESTING,do_get,0); /* TAKE */
  COMMANDO(168,POSITION_SLEEPING,do_info,0);
  COMMANDO(169,POSITION_RESTING,do_action,0);
  COMMANDO(170,POSITION_RESTING,do_practice,1);
  COMMANDO(171,POSITION_RESTING,do_action,0);
  COMMANDO(172,POSITION_SITTING,do_use,1);
  COMMANDO(173,POSITION_DEAD,do_where,1);
  COMMANDO(174,POSITION_DEAD,do_levels,0);
  COMMANDO(175,POSITION_DEAD,do_title,IMO);
  COMMANDO(176,POSITION_SITTING,do_action,0);
  COMMANDO(177,POSITION_SLEEPING,do_emote,1);
  COMMANDO(178,POSITION_RESTING,do_action,0);
  COMMANDO(179,POSITION_STANDING,do_action,0);
  COMMANDO(180,POSITION_RESTING,do_action,0);
  COMMANDO(181,POSITION_SLEEPING,do_action,0);
  COMMANDO(182,POSITION_RESTING,do_action,0);
  COMMANDO(183,POSITION_RESTING,do_action,0);
  COMMANDO(184,POSITION_RESTING,do_action,0);
  COMMANDO(185,POSITION_RESTING,do_action,0);
  COMMANDO(186,POSITION_RESTING,do_action,0);
  COMMANDO(187,POSITION_RESTING,do_action,0);
  COMMANDO(188,POSITION_RESTING,do_action,0);
  COMMANDO(189,POSITION_RESTING,do_action,0);
  COMMANDO(190,POSITION_RESTING,do_action,0);
  COMMANDO(191,POSITION_RESTING,do_action,0);
  COMMANDO(192,POSITION_RESTING,do_shoot,1);
  COMMANDO(193,POSITION_RESTING,do_action,0);
  COMMANDO(194,POSITION_RESTING,do_action,0);
  COMMANDO(195,POSITION_RESTING,do_action,0);
  COMMANDO(196,POSITION_RESTING,do_action,0);
  COMMANDO(197,POSITION_RESTING,do_action,0);
  COMMANDO(198,POSITION_RESTING,do_action,0);
  COMMANDO(199,POSITION_DEAD,do_brief,0);
  COMMANDO(200,POSITION_DEAD,do_wiznet,IMO);
  COMMANDO(201,POSITION_RESTING,do_not_here,0);
  COMMANDO(202,POSITION_SLEEPING,do_group,1);
  COMMANDO(203,POSITION_DEAD,do_restore,IMO+2);
  COMMANDO(204,POSITION_DEAD,do_return,0);
  COMMANDO(205,POSITION_DEAD,do_possess,IMO+1);
  COMMANDO(206,POSITION_RESTING,do_quaff,0);
  COMMANDO(207,POSITION_RESTING,do_recite,0);
  COMMANDO(208,POSITION_DEAD,do_users,2);
  COMMANDO(209,POSITION_SLEEPING,do_flag,IMO+1);
  COMMANDO(210,POSITION_SLEEPING,do_noshout,1);
  COMMANDO(211,POSITION_SLEEPING,do_wizhelp,IMO);
  COMMANDO(212,POSITION_RESTING,do_action,3);
  COMMANDO(213,POSITION_DEAD,do_compact,0);
  COMMANDO(214,POSITION_DEAD,do_flick,IMO+2);
  COMMANDO(215,POSITION_DEAD,do_wall,IMO+2);
  COMMANDO(216,POSITION_DEAD,do_set,IMO+2);
  COMMANDO(217,POSITION_DEAD,do_police,IMO+1);
  COMMANDO(218,POSITION_DEAD,do_wizlock,IMO+1);
  COMMANDO(219,POSITION_DEAD,do_noaffect,IMO+2);
  COMMANDO(220,POSITION_DEAD,do_invis,IMO);
  COMMANDO(221,POSITION_DEAD,do_notell,0);
  COMMANDO(222,POSITION_DEAD,do_banish,IMO+1);
  COMMANDO(223,POSITION_RESTING,do_reload,1);
  COMMANDO(224,POSITION_STANDING,do_enter,1);
  COMMANDO(225,POSITION_DEAD,do_checkrent,IMO+1500);
  COMMANDO(226,POSITION_DEAD,do_evaluate,1);
  COMMANDO(227,POSITION_STANDING,do_bank,1);
  COMMANDO(228,POSITION_STANDING,do_bank,1);
  COMMANDO(229,POSITION_STANDING,do_bank,1);
  COMMANDO(230,POSITION_DEAD,do_sys,IMO);
  COMMANDO(231,POSITION_DEAD,do_flag,IMO+1);
  COMMANDO(232,POSITION_DEAD,do_stat,IMO+1);
  COMMANDO(233,POSITION_DEAD,do_stat,IMO+1);
  COMMANDO(234,POSITION_RESTING,do_not_here,2);
  COMMANDO(235,POSITION_RESTING,do_not_here,1);
  COMMANDO(236,POSITION_SLEEPING,do_newlock,IMO+1);
  COMMANDO(237,POSITION_SLEEPING,do_look,1);
  COMMANDO(238,POSITION_STANDING,do_dig,2);
  COMMANDO(239,POSITION_SLEEPING,do_oset,IMO+2);
  COMMANDO(240,POSITION_SLEEPING,do_spellinfo,1);
  COMMANDO(241,POSITION_SLEEPING,do_title,4);
  COMMANDO(242,POSITION_RESTING,do_not_here,1);
  COMMANDO(243,POSITION_SLEEPING,do_join,IMO+2);
  COMMANDO(244,POSITION_SLEEPING,do_channel,1);
  COMMANDO(245,POSITION_SLEEPING,do_trans,IMO+2);
  COMMANDO(246,POSITION_RESTING,do_recharge,1);
  COMMANDO(247,POSITION_RESTING,do_not_here,1);
  COMMANDO(248,POSITION_DEAD,do_notell,1);
  COMMANDO(249,POSITION_RESTING,do_not_here,1);
  COMMANDO(250,POSITION_DEAD,do_count,IMO+1);
  COMMANDO(251,POSITION_SLEEPING,do_echo,IMO);  
  COMMANDO(252,POSITION_RESTING,do_gtell,1);
  COMMANDO(253,POSITION_RESTING,do_action,0);
  COMMANDO(254,POSITION_RESTING,do_split,1);
  COMMANDO(255,POSITION_RESTING,do_xyzzy,30);
  COMMANDO(256,POSITION_SLEEPING,do_command_list,1);
  COMMANDO(257,POSITION_SLEEPING,do_command_list,1);
  COMMANDO(258,POSITION_FIGHTING,do_detonate,5);
  COMMANDO(259,POSITION_SLEEPING,do_verybrief,1);
  COMMANDO(260,POSITION_DEAD,do_joke,IMO+1);
  COMMANDO(261,POSITION_RESTING,do_piss,2);
  COMMANDO(262,POSITION_RESTING,do_action,1);
  COMMANDO(263,POSITION_DEAD,do_mod,2);
  COMMANDO(264,POSITION_RESTING,do_action,1);
  COMMANDO(265,POSITION_RESTING,do_junk,1);
  COMMANDO(266,POSITION_DEAD,do_join,IMO+2);
  COMMANDO(267,POSITION_DEAD,do_reassign,IMO+2);
  COMMANDO(268,POSITION_DEAD,do_data,IMO+1);
  COMMANDO(269,POSITION_DEAD,do_localtime,1);
  COMMANDO(270,POSITION_SLEEPING,do_where,IMO);
  COMMANDO(271,POSITION_SLEEPING,do_where,IMO);
  COMMANDO(272,POSITION_SLEEPING,do_locate,1);
  COMMANDO(273,POSITION_RESTING,do_identify,1);
  COMMANDO(274,POSITION_STANDING,do_attack,1);
  COMMANDO(275,POSITION_STANDING,do_tag,1);
  COMMANDO(276,POSITION_DEAD,do_notell,1);
  COMMANDO(277,POSITION_SLEEPING,do_slug_rank,1);
  COMMANDO(278,POSITION_FIGHTING,do_heal,30);
  COMMANDO(279,POSITION_RESTING,do_not_here,1);
  COMMANDO(280,POSITION_STANDING,do_throw,2);
  COMMANDO(281,POSITION_STANDING,do_notell,100);
  COMMANDO(282,POSITION_STANDING,do_scan,5);
  COMMANDO(283,POSITION_RESTING,do_tell,2);
  COMMANDO(284,POSITION_RESTING,do_reset,2);
  COMMANDO(285,POSITION_RESTING,do_dress,2);
  COMMANDO(286,POSITION_DEAD,do_notell,0);
  COMMANDO(287,POSITION_DEAD,do_notell,0);
  COMMANDO(288,POSITION_FIGHTING,do_switch,1);
  COMMANDO(289,POSITION_RESTING,do_action,1);
  COMMANDO(290,POSITION_RESTING,do_action,1);
  COMMANDO(291,POSITION_RESTING,do_action,1);
  COMMANDO(292,POSITION_DEAD,do_notell,0);
  COMMANDO(293,POSITION_DEAD,do_find,IMO);
  COMMANDO(294,POSITION_DEAD,do_alias,1);
  COMMANDO(295,POSITION_DEAD,do_unalias,1);
  COMMANDO(296,POSITION_DEAD,do_areas,1);
  COMMANDO(297,POSITION_DEAD,do_finger,1);
  COMMANDO(298,POSITION_RESTING,do_not_here,1);
}
int find_name(char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++) {
     if (!str_cmp((player_table + i)->name, name))
        return(i);
  }
  return(-1);
}

static char *badstring[]={
   "shit",
   "fuck",
   "cunt",
   "dick",
   "blow",
   "suck",
   "exoo",
   "nazi",
   "kike",
   "nigg",
   "jew",
   0
};
int _parse_name(char *arg, char *name)
{
  int i,j,la,lb;
  char *p;

  p=name;
  /* skip whitespaces */
  for (; isspace(*arg); arg++);
  for (i = 0; *name = *arg; arg++, i++, name++) 
    if ((*arg <0) || !isalpha(*arg) || i > 15)
       return(1); 
  if (!i)
    return(1);
  la=strlen(p);
  for(i=0;badstring[i];++i){
    lb=strlen(badstring[i]);
    for(j=0;j<=(la-lb);++j)
      if(strncasecmp(p+j,badstring[i],lb)==0)
        return(1);
  }
  return(0);
}
int htoi(char *s)
{
  int n;

  sscanf(s,"%x",&n);
  return(n);
}
void nanny(struct descriptor_data *d, char *arg)
{
 char buf[100];
 int player_i;
 struct obj_data *obj;
 char tmp_name[20];
 struct char_file_u tmp_store;
 struct char_data *ch, *tmp_ch;
 struct descriptor_data *k,*oldk;
 extern struct descriptor_data *descriptor_list;
 extern int nonewplayers;
 short *ptr;
 void do_look(struct char_data *ch, char *argument, int cmd);
 void load_char_objs(struct char_data *ch);
 int load_char(char *name, struct char_file_u *char_element);

 switch (STATE(d)) {
  case CON_NME:  /* wait for input of name */
   if (!d->character) {
    CREATE(d->character, struct char_data, 1);
    clear_char(d->character);
    d->character->desc = d;
   }
   for (; isspace(*arg); arg++)  ;
   if (!*arg)
      close_socket(d);
   else {
    if( _parse_name(arg, tmp_name)) {
     SEND_TO_Q("Illegal name, please try another.\n\r", d);
     SEND_TO_Q("Name: ", d);
     return;
    }
    /* Check if already playing */
    if(!str_cmp(tmp_name,"Mel") && strncmp(d->host,"139.102",7)) {
     SEND_TO_Q("Uh, Mel doesn't play from that site.\n\r",d);
     SEND_TO_Q("Name: ", d);
     return;
    }
    for(k=descriptor_list; k; k = k->next) {
     if ((k->character != d->character) && k->character) {
      if (k->original) {
       if (GET_NAME(k->original) &&
          (str_cmp(GET_NAME(k->original), tmp_name) == 0)) {
        /*
  	SEND_TO_Q("Already playing, cannot connect\n\r", d);
        SEND_TO_Q("Name: ", d);
  	*/
	player_i = load_char(tmp_name, &tmp_store);
     store_to_char(&tmp_store, d->character);
     strcpy(d->pwd, tmp_store.pwd);
     d->pos = player_table[player_i].nr;
	SEND_TO_Q("Password: ", d);
     	STATE(d) = CON_PWDREP;
 	return;
       }
      } else { /* No switch has been made */
       if (GET_NAME(k->character) &&
          (str_cmp(GET_NAME(k->character), tmp_name) == 0))
       {
 	/*
       	SEND_TO_Q("Already playing, cannot connect\n\r", d);
        SEND_TO_Q("Name: ", d);
	*/
	player_i = load_char(tmp_name, &tmp_store);
     store_to_char(&tmp_store, d->character);
     strcpy(d->pwd, tmp_store.pwd);
     d->pos = player_table[player_i].nr;
	SEND_TO_Q("Password: ", d);
     	STATE(d) = CON_PWDREP;
        return;
       }
      }
     }
    }
    if ((player_i = load_char(tmp_name, &tmp_store)) > -1) {
     store_to_char(&tmp_store, d->character);
     strcpy(d->pwd, tmp_store.pwd);
     d->pos = player_table[player_i].nr;
     SEND_TO_Q("Password: ", d);
     STATE(d) = CON_PWDNRM;
    } else {
     /* player unknown gotta make a new */
     CREATE(GET_NAME(d->character), char, 
       strlen(tmp_name) + 1);
     strcpy(GET_NAME(d->character), 
       CAP(tmp_name));
     sprintf(buf, "Did I get that right, %s (Y/N)? ",
        tmp_name);
     SEND_TO_Q(buf, d);
     STATE(d) = CON_NMECNF;
    }
   }
  break;
  case CON_NMECNF: /* wait for conf. of new name */
   /* skip whitespaces */
   for (; isspace(*arg); arg++);
   if (*arg == 'y' || *arg == 'Y') {
    if((nonewplayers)||unfriendly_domain(d->host,newbeedomain,newbeedoms)){
     free(GET_NAME(d->character));
     close_socket(d);
    }
    SEND_TO_Q("New character.\n\r", d);
    sprintf(buf, "Give me a password for %s: ", GET_NAME(d->character));
    SEND_TO_Q(buf, d);
    STATE(d) = CON_PWDGET;
   } else {
    if (*arg == 'n' || *arg == 'N') {
     SEND_TO_Q("Ok, what IS it, then? ", d);
     free(GET_NAME(d->character));
     STATE(d) = CON_NME;
    } else { /* Please do Y or N */
     SEND_TO_Q("Please type Yes or No? ", d);
    }
   }
  break;

  case CON_PWDREP:
   /* skip whitespaces */
for (; isspace(*arg); arg++);
   if (!*arg)
      close_socket(d);
   else {
    if (strncmp((char *)crypt(arg, d->pwd), d->pwd, 10)) {
     SEND_TO_Q("Wrong password.\n\r", d);
     SEND_TO_Q("Password: ", d);
     d->wait=30;
     return;
    }
else {
     free(GET_NAME(d->character));
    close_socket(d);
    break;
SEND_TO_Q("Already playing, replace connection? (Y/N) ",d);
STATE(d) = CON_REPCNF;
}
}
break;

case CON_REPCNF:
   for (; isspace(*arg); arg++);
   if (*arg == 'y' || *arg == 'Y') {
    SEND_TO_Q("Replacing connection.\n\r", d);
    STATE(d) = CON_REPEXE;
   } else {
    if (*arg == 'n' || *arg == 'N') {
     SEND_TO_Q("Whatever.\n\r", d);
     free(GET_NAME(d->character));
    close_socket(d);
   } else { 
     SEND_TO_Q("Please type Yes or No? ", d);
    }
   }
  break;

case CON_REPEXE:



 for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next)
{
     if (!str_cmp(GET_NAME(d->character), GET_NAME(tmp_ch)) &&
      !IS_NPC(tmp_ch)) {
      SEND_TO_Q("Reconnecting.\n\r", d);

stash_char(d->character,0);
free_char(d->character);
close_socket(tmp_ch->desc);
 tmp_ch->desc = d;
      d->character = tmp_ch;
      tmp_ch->specials.timer = 0;
      tmp_ch->specials.xxx = 0;
      STATE(d) = CON_PLYNG;
      act("$n has reconnected.", TRUE, tmp_ch, 0, 0, TO_ROOM);
      sprintf(buf, "%s[%s] has replaced their connection.", GET_NAME(
       d->character), d->host);
      log(buf);
      return;
    }
}
close_socket(d); 
break;

case CON_PWDNRM: /* get pwd for known player */
   /* skip whitespaces */
   for (; isspace(*arg); arg++);
   if (!*arg)
      close_socket(d);
   else {
    if (strncmp((char *)crypt(arg, d->pwd), d->pwd, 10)) {
     SEND_TO_Q("Wrong password.\n\r", d);
     SEND_TO_Q("Password: ", d);
     d->wait=30;
     return;
    }
    for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next)
     if (!str_cmp(GET_NAME(d->character), GET_NAME(tmp_ch)) &&
      !tmp_ch->desc && !IS_NPC(tmp_ch)) {
      SEND_TO_Q("Reconnecting.\n\r", d);
      free_char(d->character);
      tmp_ch->desc = d;
      d->character = tmp_ch;
      tmp_ch->specials.timer = 0;
      tmp_ch->specials.xxx = 0;
      STATE(d) = CON_PLYNG;
      act("$n has reconnected.", TRUE, tmp_ch, 0, 0, TO_ROOM);
      sprintf(buf, "%s[%s] has reconnected.", GET_NAME(
       d->character), d->host);
      log(buf);
      return;
     }
    sprintf(buf, "%s[%s] has connected.", GET_NAME(d->character),
     d->host);
    log(buf);
    SEND_TO_Q(motd, d);
    SEND_TO_Q("\n\r\n*** PRESS RETURN: ", d);
    STATE(d) = CON_RMOTD;
   }
  break;

  case CON_PWDGET: /* get pwd for new player */
   /* skip whitespaces */
   for (; isspace(*arg); arg++);

   if (!*arg || strlen(arg) > 10)
   {
    SEND_TO_Q("Illegal password.\n\r", d);
    SEND_TO_Q("Password: ", d);
    return;
   }

   strncpy(d->pwd, (char *) crypt(arg, d->character->player.name), 10);
   *(d->pwd + 10) = '\0';
   
   SEND_TO_Q("Please retype password: ", d);

   STATE(d) = CON_PWDCNF;
  break;

  case CON_PWDCNF: /* get confirmation of new pwd */
   /* skip whitespaces */
   for (; isspace(*arg); arg++);
   if (strncmp((char *) crypt(arg, d->pwd), d->pwd, 10)) {
    SEND_TO_Q("Passwords don't match.\n\r", d);
    SEND_TO_Q("Retype password: ", d);
    STATE(d) = CON_PWDGET;
    return;
   }
   SEND_TO_Q("What is your sex (M/F/N) ? ", d);
   STATE(d) = CON_QSEX;
  break;

  case CON_QSEX:  /* query sex of new user */
   for (; isspace(*arg); arg++);
   switch (*arg) {
    case 'm':
    case 'M':
     d->character->player.sex = SEX_MALE;
    break;

    case 'f':
    case 'F':
     d->character->player.sex = SEX_FEMALE;
    break;

    case 'n':
    case 'N':
     d->character->player.sex = SEX_NEUTRAL;
    break;

    default:
     SEND_TO_Q("That's not a sex..\n\r", d);
     SEND_TO_Q("What IS your sex? :", d);
     return;
    break;
   }
   init_char(d->character);
   d->pos = create_entry(GET_NAME(d->character));
   save_char(d->character, NOWHERE);
   SEND_TO_Q(motd, d);
   SEND_TO_Q("\n\r\n*** PRESS RETURN: ", d);
   STATE(d) = CON_RMOTD;
   sprintf(buf,"%s [%s] new player.",GET_NAME(d->character), d->host);
   log(buf);
  break;

  case CON_RMOTD:  /* read CR after printing motd */
   SEND_TO_Q(MENU, d);
   STATE(d) = CON_SLCT;
  break;

  case CON_SLCT:
   /* skip whitespaces */
   for (; isspace(*arg); arg++);
   switch (*arg) {
    case '0':
     close_socket(d);
    break;
    case '1':
     reset_char(d->character);
     read_alias(d->character);
     unstash_char(d->character,0);
     stash_char(d->character,0);
     send_to_char(WELC_MESSG, d->character);
     d->character->next = character_list;
     character_list = d->character;
     if((d->character->in_room == NOWHERE)||
        (!world[d->character->in_room].name)){
       if(d->character->player.level < IMO){
         if(IS_SET(d->character->specials.act,PLR_BANISHED))
           char_to_room(d->character, real_room(6999));
         else
           char_to_room(d->character, real_room(3001));
       } else {
         char_to_room(d->character, real_room(2));
       }
     } else {
       if (real_room(d->character->in_room) > -1){
         char_to_room(d->character, real_room(d->character->in_room));
         recover_in_rent(d->character);
         if(GET_HIT(d->character) < 0) GET_HIT(d->character)=1;
       } else {
         char_to_room(d->character, real_room(3001));
       }
     }
     act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);
     STATE(d) = CON_PLYNG;
     if(!GET_LEVEL(d->character)){
      do_start(d->character);
      if(obj = read_object(3012,VIRTUAL))
       obj_to_char(obj,d->character);
      if(obj = read_object(1309,VIRTUAL))
       obj_to_char(obj,d->character);
     }
     do_look(d->character,"",15);
     d->prompt_mode = 1;
     break;
    case '2':
     SEND_TO_Q("Enter a description of your character.\n\r", d);
     SEND_TO_Q("Terminate with a '@'.\n\r", d);
     if (d->character->player.description) {
      SEND_TO_Q("Old description :\n\r", d);
      SEND_TO_Q(d->character->player.description, d);
      free(d->character->player.description);
      d->character->player.description = 0;
     }
     d->str = 
        &d->character->player.description;
     d->max_str = 240;
     STATE(d) = CON_EXDSCR;
    break;

    case '3':
     SEND_TO_Q("Enter a new password: ", d);
     STATE(d) = CON_PWDNEW;
    break;
    default:
     SEND_TO_Q("Wrong option.\n\r", d);
     SEND_TO_Q(MENU, d);
    break;
   }
  break;
  case CON_PWDNEW:
   /* skip whitespaces */
   for (; isspace(*arg); arg++);

   if (!*arg || strlen(arg) > 10)
   {
    SEND_TO_Q("Illegal password.\n\r", d);
    SEND_TO_Q("Password: ", d);
    return;
   }

   strncpy(d->pwd,(char *) crypt(arg, d->character->player.name), 10);
   *(d->pwd + 10) = '\0';

   SEND_TO_Q("Please retype password: ", d);

   STATE(d) = CON_PWDNCNF;
  break;
  case CON_PWDNCNF:
   /* skip whitespaces */
   for (; isspace(*arg); arg++);

   if (strncmp((char *) crypt(arg, d->pwd), d->pwd, 10))
   {
    SEND_TO_Q("Passwords don't match.\n\r", d);
    SEND_TO_Q("Retype password: ", d);
    STATE(d) = CON_PWDNEW;
    return;
   }
   SEND_TO_Q(
    "\n\rDone. You must enter the game to make the change final\n\r",
     d);
   SEND_TO_Q(MENU, d);
   STATE(d) = CON_SLCT;
  break;
  default:
   log("Nanny: illegal state of con'ness");
   abort();
  break;
 }
}


