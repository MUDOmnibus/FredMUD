/* ************************************************************************
*  file: act.comm.c , Implementation of commands.         Part of DIKUMUD *
*  Usage : Communication.                                                 *
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

/* extern variables */

extern struct room_data *world;
extern struct descriptor_data *descriptor_list;

struct char_data *get_player_vis(struct char_data *ch, char *name);

void do_say(struct char_data *ch, char *argument, int cmd)
{
  int i;
  static char buf[MAX_STRING_LENGTH];

  for (i = 0; *(argument + i) == ' '; i++);

  if (!*(argument + i))
    send_to_char("Yes, but WHAT do you want to say?\n\r", ch);
  else {
    sprintf(buf,"$n says '%s'", argument + i);
    act(buf,FALSE,ch,0,0,TO_ROOM);
    if(!IS_NPC(ch) && IS_SET(ch->specials.act,PLR_REPEAT)) {
        sprintf(buf,"You say '%s'\n\r",argument + i);
        send_to_char(buf,ch);
    }
    else
	send_to_char("Ok.\n\r", ch);
  }
}

void do_shout(struct char_data *ch, char *argument, int cmd)
{
  static char buf1[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  extern int noshoutflag;

  if(noshoutflag && !IS_NPC(ch) && (GET_LEVEL(ch) < IMO)){
    send_to_char("I guess you can't shout now?\n\r",ch);
    return;
  }
  if(!IS_NPC(ch)){
    if (IS_SET(ch->specials.act, PLR_NOSHOUT)) {
      send_to_char("You can't shout!!\n\r", ch);
      return;
    }
  } else {
    if(IS_AFFECTED(ch,AFF_CHARM))
      return;
  }
  for (; *argument == ' '; argument++);
  if (!(*argument))
    send_to_char("Shout? Yes! Fine! Shout we must, but WHAT??\n\r", ch);
  else {
    if(!IS_NPC(ch) && IS_SET(ch->specials.act,PLR_REPEAT)) {
        sprintf(buf1,"You shout '%s'\n\r",argument);
        send_to_char(buf1,ch);
    }
    else
	send_to_char("Ok.\n\r", ch);
    sprintf(buf1, "$n shouts '%s'", argument);
    for (i = descriptor_list; i; i = i->next)
      if (i->character != ch && !i->connected &&
        !IS_SET(i->character->specials.act, PLR_EARMUFFS))
        act(buf1, 0, ch, 0, i->character, TO_VICT);
    if(GET_LEVEL(ch) < IMO)
      WAIT_STATE(ch,(IMO-GET_LEVEL(ch))>>4);
  }
}

void do_tell(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict;
  char *s, name[100], message[MAX_STRING_LENGTH],
  buf[MAX_STRING_LENGTH];

  half_chop(argument,name,message);
  if(!*name || !*message){
    send_to_char("Who do you wish to tell what??\n\r", ch);
    return;
  }
  if(cmd==283)
    vict=get_player_vis(ch,name);
  else
    vict=get_char_vis(ch, name);
  if(!vict)
    send_to_char("No-one by that name here..\n\r", ch);
  else if (ch == vict)
    send_to_char("You try to tell yourself something.\n\r", ch);
  else if((GET_POS(vict) == POSITION_SLEEPING)&&(GET_LEVEL(ch) < IMO+100)){
    act("$E can't hear you.",FALSE,ch,0,vict,TO_CHAR);
  } else if ((!IS_SET(vict->specials.act,PLR_NOTELL)) ||
             ((GET_LEVEL(ch) >= IMO) && (GET_LEVEL(ch) > GET_LEVEL(vict)))){
    if(IS_NPC(ch))
      s=ch->player.short_descr;
    else
      s=CAN_SEE(vict,ch) ? GET_NAME(ch) : "Someone";
    sprintf(buf,"%s tells you '%s'\n\r",s,message);
    send_to_char(buf, vict);
    if(!IS_NPC(ch) && IS_SET(ch->specials.act,PLR_REPEAT)) {
	s=CAN_SEE(ch,vict) ? GET_NAME(vict) : "Someone";
	sprintf(buf,"You tell %s '%s'\n\r",s,message);
	send_to_char(buf,ch); 
    }
    else
  	send_to_char("Ok.\n\r", ch);
  } else {
    act("$E isn't listening now.",FALSE,ch,0,vict,TO_CHAR);
  }
}

void do_whisper(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict;
  char name[100], message[MAX_STRING_LENGTH],
    buf[MAX_STRING_LENGTH];
  char *s;

  half_chop(argument,name,message);

  if(!*name || !*message)
    send_to_char("Who do you want to whisper to.. and what??\n\r", ch);
  else if (!(vict = get_char_room_vis(ch, name)))
    send_to_char("No-one by that name here..\n\r", ch);
  else if (vict == ch)
  {
    act("$n whispers quietly to $mself.",FALSE,ch,0,0,TO_ROOM);
    send_to_char(
      "You can't seem to get your mouth close enough to your ear...\n\r",
       ch);
  } else if(IS_SET(vict->specials.act,PLR_NOTELL)){
    act("$E isn't listening now.",FALSE,ch,0,vict,TO_CHAR);
  } else {
    sprintf(buf,"$n whispers to you, '%s'",message);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    if(!IS_NPC(ch) && IS_SET(ch->specials.act,PLR_REPEAT)) {
        s=CAN_SEE(ch,vict) ? GET_NAME(vict) : "Someone";
        sprintf(buf,"You whisper to %s, '%s'\n\r",s,message);
        send_to_char(buf,ch);
    }
    else
	send_to_char("Ok.\n\r", ch);
    act("$n whispers something to $N.", FALSE, ch, 0, vict, TO_NOTVICT);
  }
}


void do_ask(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict;
  char name[100], message[MAX_STRING_LENGTH],
    buf[MAX_STRING_LENGTH];
  char *s;

  half_chop(argument,name,message);

  if(!*name || !*message)
    send_to_char("Who do you want to ask something.. and what??\n\r", ch);
  else if (!(vict = get_char_room_vis(ch, name)))
    send_to_char("No-one by that name here..\n\r", ch);
  else if (vict == ch)
  {
    act("$n quietly asks $mself a question.",FALSE,ch,0,0,TO_ROOM);
    send_to_char("You think about it for a while...\n\r", ch);
  } else if(IS_SET(vict->specials.act,PLR_NOTELL)){
    act("$E isn't listening now.",FALSE,ch,0,vict,TO_CHAR);
  } else {
    sprintf(buf,"$n asks you '%s'",message);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    if(!IS_NPC(ch) && IS_SET(ch->specials.act,PLR_REPEAT)) {
        s=CAN_SEE(ch,vict) ? GET_NAME(vict) : "Someone";
        sprintf(buf,"You ask %s '%s'\n\r",s,message);
        send_to_char(buf,ch);
    }
    else
	send_to_char("Ok.\n\r", ch);
    act("$n asks $N a question.",FALSE,ch,0,vict,TO_NOTVICT);
  }
}

#define TQSIZE 20
char *tqueue[TQSIZE]={0,0,0,0,0,0,0,0,0,0};
int tqptr=0;

void do_talk(struct char_data *ch, char *argument, int cmd)
{
  struct descriptor_data *d;
  struct char_data *victim;
  struct obj_data *o;
  int chan,ochan,flag,channo,i,j;
  char buf1[MAX_STRING_LENGTH],buf2[MAX_STRING_LENGTH];

  if(IS_NPC(ch) && (GET_LEVEL(ch) < 21))
    return;
  if(GET_LEVEL(ch) < 2){
    send_to_char("At level 1, you can only listen.\n\r",ch);
    return;
  }
  if(!IS_NPC(ch) && IS_SET(ch->specials.act, PLR_NOSHOUT)) {
    send_to_char("You can't shout, OR talk on the radio..\n\r", ch);
    return;
  }
  if(!(o=ch->equipment[WEAR_RADIO])){
    if(GET_LEVEL(ch) >= 900){
      chan=0x7fff;
    } else {
      send_to_char("You don't have a radio.\n\r",ch);
      return;
    }
  } else if((chan=o->obj_flags.value[1])==0){
    send_to_char("Your radio is off.\n\r",ch);
    return;
  }
  channo=1;
  flag=0;
  while(!flag && (channo <= 30)){
    if(chan & (1<<(channo-1)))
      flag=1;
    else
      channo++;
  }
  if(!*argument){
    j=tqptr;
    send_to_char("Recent talks:\n\r",ch);
    for(i=0;i<TQSIZE;i++){
      if(tqueue[j])
        send_to_char(tqueue[j],ch);
      if((++j)==TQSIZE)
        j=0;
    }
    return;
  }
  sprintf(buf1,"%s (%d) - %s\n\r",
    IS_NPC(ch) ? ch->player.short_descr : ch->player.name,channo,argument);
  sprintf(buf2,"Someone (%d) - %s\n\r",channo,argument);
  for(d=descriptor_list;d;d=d->next)
    if(!d->connected){
      if(d->original) continue;
      victim=d->character;
      o=victim->equipment[WEAR_RADIO];
      if(!o)
        continue;
      ochan=o->obj_flags.value[2];
      if(chan & ochan){
        if(CAN_SEE(victim,ch))
          send_to_char(buf1,victim);
        else
          send_to_char(buf2,victim);
      }
    }
  tqueue[tqptr++]=strdup(buf1);
  if(tqptr==TQSIZE) tqptr=0;
  send_to_char("Ok.\n\r",ch);
}
void do_channel(struct char_data *ch, char *argument, int cmd)
{
  struct obj_data *o;
  int chan,f,i;
  char mess[80];

  if(IS_NPC(ch))
    return;
  if(!(o=ch->equipment[WEAR_RADIO])){
    send_to_char("Your don't have a radio.\n\r",ch);
    return;
  }
  if(!(*argument)){
    f=o->obj_flags.value[2];
    sprintf(mess,"You are using channel");
    for(i=0;i<30;i++)
      if(f & (1<<i))
        sprintf(mess+strlen(mess)," %d",i+1);
     strcat(mess,".\n\r");
    send_to_char(mess,ch);
    return;
  }
  chan=atoi(argument);
  if((chan < 0)||(chan > 30)){
    send_to_char("No such channel.\n\r",ch);
    return;
  }
  if(chan==0){
    o->obj_flags.value[1]=0;
    o->obj_flags.value[2]=0;
  } else {
    o->obj_flags.value[1]=(1<<(chan-1));
    o->obj_flags.value[2]=o->obj_flags.value[1];
  }
  send_to_char("Ok.\n\r",ch);
}
void do_gtell(struct char_data *ch, char *argument, int cmd)
{
  char buf[256];
  struct char_data *victim, *k;
  struct follow_type *f;

  if(!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("You don't belong to a group!\n\r", ch);
    return;
  }
  if (!*argument) {
    send_to_char("Tell them what?\n\r",ch);
    return;
  }
  if (ch->master)
    k = ch->master;
  else
    k = ch;
  while(*argument == ' ') ++argument;
  sprintf(buf,"$n tells the group: '%s'", argument);
  if(IS_AFFECTED(k,AFF_GROUP))
    act(buf,FALSE,ch,0,k,TO_VICT);
  for(f=k->followers; f; f=f->next)
    if(IS_AFFECTED(f->follower,AFF_GROUP))
      act(buf,FALSE,ch,0,f->follower,TO_VICT);
  if(!IS_NPC(ch) && IS_SET(ch->specials.act,PLR_REPEAT)) {
        sprintf(buf,"You tell the group: '%s'\n\r",argument);
        send_to_char(buf,ch);
    }
    else
	send_to_char("OK\n\r",ch);
}
