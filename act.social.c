/* ************************************************************************
*  file: act.social.c , Implementation of commands.       Part of DIKUMUD *
*  Usage : Social commands.                                               *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************* */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
extern struct room_data *world;

/* extern functions */

void parse_string(char *input, char *output, struct char_data *ch1,
  struct char_data *ch2, struct char_data *to);
int action(int cmd);
char *fread_action(FILE *fl);


struct social_messg
{
  int act_nr;
  int hide;
  int min_victim_position; /* Position of victim */

  /* No argument was supplied */
  char *char_no_arg;
  char *others_no_arg;

  /* An argument was there, and a victim was found */
  char *char_found;    /* if NULL, read no further, ignore args */
  char *others_found;
  char *vict_found;

  /* An argument was there, but no victim was found */
  char *not_found;

  /* The victim turned out to be the character */
  char *char_auto;
  char *others_auto;
} *soc_mess_list = 0;


static int list_top = -1;

char *fread_action(FILE *fl)
{
  char buf[MAX_STRING_LENGTH], *rslt;

  for (;;)
  {
    fgets(buf, MAX_STRING_LENGTH, fl);
    if (feof(fl))
    {
      log("Fread_action - unexpected EOF.");
      exit(0);
    }

    if (*buf == '#')
      return(0);
    {
      *(buf + strlen(buf) - 1) = '\0';
      CREATE(rslt, char, strlen(buf) + 1);
      strcpy(rslt, buf);
      return(rslt);
    }
  }
}
  



void boot_social_messages(void)
{
  FILE *fl;
  int tmp, hide, min_pos;

  if (!(fl = fopen(SOCMESS_FILE, "r")))
  {
    perror("boot_social_messages");
    exit(0);
  }

  for (;;)
  {
    fscanf(fl, " %d ", &tmp);
    if (tmp < 0)
      break;
    fscanf(fl, " %d ", &hide);
    fscanf(fl, " %d \n", &min_pos);

    /* alloc a new cell */
    if (!soc_mess_list)
    {
      CREATE(soc_mess_list, struct social_messg, 1);
      list_top = 0;
    }
    else
      if (!(soc_mess_list = (struct social_messg *)
        realloc(soc_mess_list, sizeof (struct social_messg) *
        (++list_top + 1))))
      {
        perror("boot_social_messages. realloc");
        exit(1);
      }

    /* read the stuff */
    soc_mess_list[list_top].act_nr = tmp;
    soc_mess_list[list_top].hide = hide;
    soc_mess_list[list_top].min_victim_position = min_pos;

    soc_mess_list[list_top].char_no_arg = fread_action(fl);
    soc_mess_list[list_top].others_no_arg = fread_action(fl);

    soc_mess_list[list_top].char_found = fread_action(fl);

    /* if no char_found, the rest is to be ignored */
    if (!soc_mess_list[list_top].char_found)
      continue;

    soc_mess_list[list_top].others_found = fread_action(fl);  
    soc_mess_list[list_top].vict_found = fread_action(fl);

    soc_mess_list[list_top].not_found = fread_action(fl);

    soc_mess_list[list_top].char_auto = fread_action(fl);

    soc_mess_list[list_top].others_auto = fread_action(fl);
  }
  fclose(fl);
}




int find_action(int cmd)
{
  int bot, top, mid;

  bot = 0;
  top = list_top;

  if (top < 0)
    return(-1);

  for(;;)
  {
    mid = (bot + top) / 2;

    if (soc_mess_list[mid].act_nr == cmd)
      return(mid);
    if (bot >= top)
      return(-1);

    if (soc_mess_list[mid].act_nr > cmd)
      top = --mid;
    else
      bot = ++mid;
  }
}


void do_action(struct char_data *ch, char *argument, int cmd)
{
  int act_nr;
  char buf[MAX_INPUT_LENGTH], tmp[MAX_STRING_LENGTH];
  struct social_messg *action;
  struct char_data *i, *vict;

  if ((act_nr = find_action(cmd)) < 0)
  {
    send_to_char("That action is not supported.\n\r", ch);
    return;
  }

  action = &soc_mess_list[act_nr];

  if (action->char_found)
    one_argument(argument, buf);
  else
    *buf = '\0';

  if (!*buf) {
    send_to_char(action->char_no_arg, ch);
    send_to_char("\n\r", ch);
    act(action->others_no_arg, action->hide, ch, 0, 0, TO_ROOM);
    return;
  }

  if (!(vict = get_char_room_vis(ch, buf))) {
    send_to_char(action->not_found, ch);
    send_to_char("\n\r", ch);
  } else if (vict == ch) {
    send_to_char(action->char_auto, ch);
    send_to_char("\n\r", ch);
    act(action->others_auto, action->hide, ch, 0, 0, TO_ROOM);
  } else {
    if (GET_POS(vict) < action->min_victim_position) {
      act("$N is not in a proper position for that.",FALSE,ch,0,vict,TO_CHAR);
    } else {
      act(action->char_found, 0, ch, 0, vict, TO_CHAR);
      act(action->others_found, action->hide, ch, 0, vict, TO_NOTVICT);
      act(action->vict_found, action->hide, ch, 0, vict, TO_VICT);
    }
  }
}


