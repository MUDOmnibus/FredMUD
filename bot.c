
/* bot functions */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "limits.h"

/* extern vars */

extern struct time_info_data time_info;
extern char *dirletters;
extern struct command_info cmd_info[];
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct index_data *mob_index;

extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int top_of_mobt;
extern int top_of_objt;

/* extern functions */

void do_action(struct char_data *ch, char *argument, int cmd);
#ifdef NEEDS_STRDUP
char *strdup(char *source);
#endif
 
char *rand_player_room(struct char_data *ch);
char *rand_player_world(struct char_data *ch);
char *rand_playername_file();
char *rand_char_room(struct char_data *ch);
char *rand_mob_room(struct char_data *ch);

char *rand_player_room(struct char_data *ch) {
  struct char_data *tmp;
  int pcount;

  pcount = 0;
  for(tmp=world[ch->in_room].people;tmp;tmp=tmp->next_in_room)
    if(!IS_NPC(tmp)) pcount++;
  if(!pcount) return(NULL);
  pcount = number(0,pcount-1);
  for(tmp=world[ch->in_room].people;pcount||IS_NPC(tmp);tmp=tmp->next_in_room)
    if(!IS_NPC(tmp)) pcount--;
  return tmp ? GET_NAME(tmp) : GET_NAME(ch);
} /* end rand_player_room */
  
char *rand_player_world(struct char_data *ch) {
  struct char_data *c;
  struct descriptor_data *d;
  int pcount =0;

  for(d=descriptor_list;d;d=d->next) {
    if(!d->connected) {
      c = d->original ? d->original : d->character;
      if(CAN_SEE(ch,c)) pcount++;
    }
  }
  if(!pcount) return(NULL);
  pcount=number(0,pcount-1);
 
  for(d=descriptor_list;pcount || d->connected;d=d->next)
    if(!d->connected) pcount--;
  c = d->original ? d->original : d->character;
  return c ? GET_NAME(c) : GET_NAME(ch);

} /* end rand_player_world*/


char *rand_playername_file() {
  int pcount;
  pcount = number(0,top_of_p_table);
  return((player_table + pcount)->name);
} /* end rand_player_file */

char *rand_mob_room(struct char_data *ch) {
  struct char_data *tmp;
  int pcount=0;

for(tmp=world[ch->in_room].people;tmp;tmp=tmp->next_in_room)
  if(IS_NPC(tmp)) pcount++;
if(!pcount) return(NULL);
pcount = number(0,pcount-1);
for(tmp=world[ch->in_room].people;pcount|| !IS_NPC(tmp);tmp=tmp->next_in_room)
  if(IS_NPC(tmp)) pcount--;
return tmp ? tmp->player.short_descr : 0;

} /* end rand_mob_room */

char *rand_char_room(struct char_data *ch) {
  struct char_data *tmp;
  int pcount;

  pcount = 0;
  for(tmp=world[ch->in_room].people;tmp;tmp=tmp->next_in_room)
    pcount++;
  if(!pcount) return(NULL);
  pcount = number(0,pcount-1);
  for(tmp=world[ch->in_room].people;pcount;tmp=tmp->next_in_room)
    pcount--;
  if(!tmp)
    return 0;
  return IS_NPC(tmp) ? tmp->player.short_descr : GET_NAME(tmp);
} /* end rand_char_room */

struct obj_data *rand_object() {
  extern int top_of_objt;
  struct obj_data *otmp = NULL;

  while(!otmp)
    otmp = read_object(number(0,top_of_objt-1),REAL);
  return(otmp);

} /* end rand_object */

#define NOBS 12

char *oblist[]={
  "jackass", "scumbag",
  "shithead", "goofball",
  "jerk", "lowlife",
  "moron", "toad",
  "idiot", "slimeball",
  "mole", "dickweed"
};

char *rand_obscenity()
{
  return oblist[number(0,NOBS-1)];
}

#define NGRS 8

char *grlist[]={
  "Hello", "Greetings", "Good Morning",
  "Good Evening", "Salutations", "Bonjour",
  "Yo", "Hi"
};

char *rand_greeting()
{
  return grlist[number(0,NGRS-1)];
}
int bot(struct char_data *ch, int cmd, char *arg)
{
  int i,k,n;
  ubyte flag;
  sbyte c;
  char *p,*s,*st,buf[256];
  struct char_data *tmp;
  struct obj_data *otmp, *o;

  if(cmd)
    return(FALSE);
  if(!ch->player.description)
    return(FALSE);
  n=ch->specials.xxx;
  c=ch->player.description[n];
  if(!n && !c)
    return(FALSE);
  ++n;
  switch(c){
    case  0  : n=0;
    case '\n': break;
    case '0' :
    case '1' :
    case '2' :
    case '3' :
    case '4' :
    case '5' :
      do_move(ch,"",c-'0'+1);
      break;
    case 'A' :
      for(i=0;;i++){
        c=ch->player.description[n++];
        if(!c || (c=='.'))
          break;
        if(c=='#'){
          c=ch->player.description[n++];
          if(c=='d'){  /* char variables */
            c=dirletters[number(0,5)];
            buf[i]=c;
          } else {  /* string variables */
            flag=0;
            switch(c){
              case 'f': flag=1; st=rand_playername_file(); break;
              case 'x': st=rand_obscenity(); break;
              case 'y': st=rand_greeting(); break;
              case 'r': st=rand_player_room(ch); break;
              case 'w': st=rand_player_world(ch); break;
              default: st=GET_NAME(ch); break;
            }
            if(st){
              strcpy(buf+i,st);
              if(flag && islower(buf[i]))
                buf[i]=toupper(buf[i]);
              i+=(strlen(st)- 1);
            }
          }
        } else {
          buf[i]=c;
        }
      }
      buf[i]=0;
      command_interpreter(ch,buf);
      break;
    case 'B' :
      otmp = read_object(1599,VIRTUAL);
      if(otmp){
        otmp->obj_flags.timer=0;
        SET_BIT(otmp->obj_flags.extra_flags,ITEM_POOFSOON);
        obj_to_room(otmp,ch->in_room);
      }
      break;
    case 'C' :
      for(tmp=world[ch->in_room].people;tmp&&(tmp==ch);tmp=tmp->next_in_room);
      if(tmp){
        sprintf(buf,"2 coins %s",GET_NAME(tmp));
        do_give(ch,buf,0);
      }
      break;
    case 'D' :
      do_drop(ch,"all",0);
      break;
    case 'E':
      if(s=ch->player.title){
        if(p=(char *)index(s,'#')){
          strcpy(buf,s);
          s=buf;
          p=(char *)index(s,'#');
          *p = '0'+number(0,5);
        }
        command_interpreter(ch,s);
      }
      break;
    case 'F' :
      for(tmp=world[ch->in_room].people;tmp&&(tmp==ch);tmp=tmp->next_in_room);
      if(tmp){
        sprintf(buf,"%s",GET_NAME(tmp));
        do_follow(ch,buf,0);
      }
      break;
    case 'G' :
      do_get(ch,"all",0);
      break;
    case 'H':
      if(ch->master){
        tmp=ch->master;
        act("$n utters the words 'judicandus dies'.",TRUE,ch,0,tmp,TO_ROOM);
        cast_cure_light(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,tmp,0);
      }
      break;
    case 'I' :
      for(tmp=world[ch->in_room].people;tmp&&(tmp==ch);tmp=tmp->next_in_room);
      if(tmp){
        k=number(6,264);
        if(cmd_info[k].command_pointer==do_action){
          sprintf(buf,"%s",GET_NAME(tmp));
          do_action(ch,buf,k);
        }
      }
      break;
    case 'J' :
      k=number(6,264);
      if(cmd_info[k].command_pointer==do_action)
        do_action(ch,"",k);
      break;
    case 'L' :
      sprintf(buf,"I'm having a swell time at %s!",
        world[ch->in_room].name);
      do_shout(ch,buf,0);
      break;
/*
    case 'M':
      break;
      i = number(0,top_of_mobt-1);
      tmp = read_mobile(i,REAL);
      if(tmp)
        char_to_room(tmp,ch->in_room);
      break;
    case 'O':
      break;
      i = number(0,top_of_objt-1);
      otmp = read_object(i,REAL);
      if(otmp)
        obj_to_room(otmp,ch->in_room);
      break;
*/
    case 'P' :
      sprintf(buf,"bob %s %s","bot",random_bot_program());
      command_interpreter(ch,buf);
      break;
    case 'R' :
      do_move(ch,"",number(1,6));
      break;
    case 'S' :
      for(o=ch->carrying;o;o=otmp) {
        otmp = o->next_content;
        sprintf(buf, "sell %s",fname(o->name));
        if(!command_interpreter(ch,buf))
          break;
      }
      break;
    case 'T' :
      teleport(ch,0);
      break;
    case 'W':
      spell_word_of_recall(IMO,ch,ch,0);
      break;
  }
  ch->specials.xxx=n;
  return(FALSE);
}

#define NMHLINES 8

static char *mh_lines[]={
  "Hey Mel, this Mud sucks!",
  "Wipe the player file, Mel, ya creep!",
  "Guess what sux, Mel!",
  "Where's my ole pal, ThisMudSux?",
  "About time for a player file wipe, isn't it?",
  "Let's wipe this game out!",
  "Time for a new player file!",
  "This MUD sux!"
};

int mudhead(struct char_data *ch, int cmd, char *arg)
{
  static char mudhead_path[] = "s3g2222dx0000p1r.";
  static bool moving = FALSE;
  static int index=0, lt = 0;
  int n, t;
  char buf[256];

  if((cmd==94) && (GET_LEVEL(ch) > (IMO+1))){
    if(!moving){
      moving=TRUE; index=0; return(FALSE);
    }
  }
  if(cmd)
    return(FALSE);
  if (!moving) {
    if (time_info.hours == 12) {
      do_shout(ch,"Hey, it's noon, time for me to get up!",0);
      moving = TRUE;
      index = 0;
    }
  }
  if(!moving || (GET_POS(ch) < POSITION_SLEEPING))
    return FALSE;
  t = time(0);
  if((t - lt) < 5)
    return;
  switch (n=mudhead_path[index]) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
      if(CAN_GO(ch,n-'0'))
        do_move(ch,"",n-'0'+1);
      break;
    case 'd':
      do_drop(ch,"all",0);
      break;
    case 'g':
      do_get(ch,"all",0);
      break;
    case 'r':
      GET_POS(ch) = POSITION_RESTING;
      act("$n falls down.",FALSE,ch,0,0,TO_ROOM);
      break;
    case 's':
      GET_POS(ch) = POSITION_STANDING;
      act("$n just manages to stand up.",FALSE,ch,0,0,TO_ROOM);
      break;
    case 'p':
      do_piss(ch,"",0);
      break;
    case 'x':
      do_shout(ch,mh_lines[number(0,NMHLINES-1)],0);
      break;
    case '.':
      moving = FALSE;
      index=0;
      break;
  }
  index++;
  lt = t;
  return TRUE;
}
