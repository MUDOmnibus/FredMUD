
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#ifdef SYSV
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
#include "structs.h"
#include "utils.h"
#include "handler.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"

extern struct room_data *world;
extern int revdir[];

#ifdef NEEDS_STRDUP
char *strdup(char *s);
#endif

void do_save(struct char_data *ch, char *argument, int cmd)
{
  act("Save at Midgaard Bank!",TRUE,ch,0,0,TO_CHAR);
  act("$n just typed save! The fool!",TRUE,ch,0,0,TO_ROOM);
}
void do_mod(struct char_data *ch, char *argument, int cmd)
{
  int i,j,room,dir,oroom,odir;
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  char what[MAX_STRING_LENGTH], stuff[MAX_STRING_LENGTH];
  char keyword[MAX_STRING_LENGTH], dirstring[MAX_STRING_LENGTH];
  struct room_data *rm;
  extern int top_of_world;
  extern struct index_data *obj_index;
  extern char *room_bits[];

  room = ch->in_room;
  rm = &world[room];
  if((GET_LEVEL(ch) < (IMO+99))&&
     (!IS_SET(ch->specials.act,PLR_BUILDER) ||
      !IS_SET(rm->room_flags,MODROOM))){
    send_to_char("You can't modify this room.\n\r",ch);
    return;
  }
  argument=one_argument(argument,what);
  if(*what == 'e'){  /* exit */
    sprintf(buf2,"Hmm, game got <%s>, has <%s>.\n\r",what,argument);
    send_to_char(buf2,ch);
    half_chop(argument,keyword,dirstring);
    if(!*keyword || !*dirstring){
      sprintf(buf,"Usage: mod exit name direction.\n\r");
      send_to_char(buf,ch);
      return;
    }
    dir = atoi(dirstring);
    if((dir < 0)||(dir > 5)){
      send_to_char("Illegal direction!\n\r",ch);
      return;
    }
    if(!world[room].dir_option[dir]){
      send_to_char("There's no there there.\n\r",ch);
      return;
    }
    if(world[room].dir_option[dir]->keyword){
      send_to_char("Damn, already a door there!\n\r",ch);
      return;
    }
    sprintf(buf,"You are making an exit <%s> in direction %d\n\r",
      keyword,dir);
    act(buf,TRUE,ch,0,0,TO_CHAR);
    world[room].dir_option[dir]->keyword=strdup(keyword);
    world[room].dir_option[dir]->exit_info=(EX_ISDOOR | EX_CLOSED);
    oroom=world[room].dir_option[dir]->to_room;
    odir=revdir[dir];
    if(world[oroom].dir_option[odir]){
      if(world[oroom].dir_option[odir]->to_room==room){
        if(!world[oroom].dir_option[odir]->keyword){
          world[oroom].dir_option[odir]->keyword=strdup(keyword);
          world[oroom].dir_option[odir]->exit_info=(EX_ISDOOR | EX_CLOSED);
        }
      }
    }
  } else if(*what == 'n'){  /* name */
    if(!argument[0]){
      send_to_char("Usage: mod name short_name_of_room\n\r",ch);
      return;
    }
    free(world[room].name);
    if(argument)
      for(;isspace(*argument);++argument);
    world[room].name=strdup(argument);
  } else if(*what == 'd'){  /* description */
    if(!argument[0]){
      if(ch->desc->str)
        free(ch->desc->str);
      ch->desc->str = &world[room].description;
      if(*ch->desc->str)
        free(*ch->desc->str);
      *ch->desc->str = 0;
      ch->desc->max_str = 255;
      return;
    }
    free(world[room].description);
    world[room].description=strdup(argument);
  } else if(*what == 'f'){   /* flags */
    if(!*argument){
      sprintf(buf,"Current room flags are %d = ",(long) rm->room_flags);
      send_to_char(buf,ch);
      sprintbit((long) rm->room_flags,room_bits,buf);
      strcat(buf,"\n\r");
      send_to_char(buf,ch);
      return;
    }
    i=atoi(argument);
    if(i < 0){
      send_to_char("Room NOT set.\n\r",ch);
      return;
    }
    if(GET_LEVEL(ch) < IMO){
/*
      i |= MODROOM;
*/
      i &= 0x7fff;
    }
    world[ch->in_room].room_flags=i;
    send_to_char("Room set.\n\r",ch);
    return;
  } else {
    send_to_char("You can only modify NAME, EXIT, FLAGS and DESCR.\n\r",ch);
    return;
  }
  send_to_char("Yo dude.\n\r",ch);
}

