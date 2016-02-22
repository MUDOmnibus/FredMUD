/* ************************************************************************
*  file: modify.c                                         Part of DIKUMUD *
*  Usage: Run-time modification (by users) of game variables              *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************ */


#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"


#define TP_MOB    0
#define TP_OBJ     1
#define TP_ERROR  2


void show_string(struct descriptor_data *d, char *input);



char *string_fields[] =
{
  "nom",
  "short",
  "long",
  "description",
  "title",
  "delete-description",
  "\n"
};




/* maximum length for text field x+1 */
int length[] =
{
  15,
  60,
  256,
  240,
  60
};




char *skill_fields[] = 
{
  "learned",
  "affected",
  "duration",
  "recognize",
  "\n"
};




int max_value[] =
{
  255,
  255,
  10000,
  1
};

/* ************************************************************************
*  modification of malloc'ed strings                                      *
************************************************************************ */

/* Add user input to the 'current' string (as defined by d->str) */
void string_add(struct descriptor_data *d, char *str)
{
  char *scan;
  int terminator = 0;

  /* determine if this is the terminal string, and truncate if so */
  for (scan = str; *scan; scan++)
     if (terminator = (*scan == '@'))
     {
      *scan = '\0';
      break;
     }
  
  if (!(*d->str))
  {
    if (strlen(str) > d->max_str)
    {
      send_to_char("String too long - Truncated.\n\r",
         d->character);
      *(str + d->max_str) = '\0';
      terminator = 1;
    }
    CREATE(*d->str, char, strlen(str) + 3);
    strcpy(*d->str, str);
  } else {
    if (strlen(str) + strlen(*d->str) > d->max_str) {
      send_to_char("String too long. Last line skipped.\n\r",
         d->character);
      terminator = 1;
    } else {
      if (!(*d->str = (char *) realloc(*d->str, strlen(*d->str) + 
         strlen(str) + 3))) {
        perror("string_add");
        exit(1);
      }
      strcat(*d->str, str);
    }
  }

  if (terminator) {
    d->str = 0;
    if (d->connected == CON_EXDSCR) {
      SEND_TO_Q(MENU, d);
      d->connected = CON_SLCT;
    }
  }
  else
     strcat(*d->str, "\n\r");
}


#undef MAX_STR

/* interpret an argument for do_string */
void quad_arg(char *arg, int *type, char *name, int *field, char *string)
{
  char buf[MAX_STRING_LENGTH];
  int i;

  /* determine type */
  arg = one_argument(arg, buf);
  if (is_abbrev(buf, "char"))
     *type = TP_MOB;
  else if (is_abbrev(buf, "obj"))
     *type = TP_OBJ;
  else
  {
    *type = TP_ERROR;
    return;
  }

  /* find name */
  arg = one_argument(arg, name);

  /* field name and number */
  arg = one_argument(arg, buf);
  if (!(*field = old_search_block(buf, 0, strlen(buf), string_fields, 0)))
     return;

  /* string */
  for (; isspace(*arg); arg++);
  for (; *string = *arg; arg++, string++);

  return;
}
 
void do_string(struct char_data *ch, char *arg, int cmd)
{
  char name[MAX_STRING_LENGTH], string[MAX_STRING_LENGTH];
  int field, type;
  struct char_data *mob;
  struct obj_data *obj;
  struct extra_descr_data *ed, *tmp;

  if(IS_NPC(ch)) return;

  quad_arg(arg, &type, name, &field, string);
  if (type == TP_ERROR) {
    send_to_char(
     "Syntax: string ('obj'|'char') <name> <field> [<string>].\n\r",ch);
    return;
  }
  if (!field) {
    send_to_char("No field by that name. Try 'help string'.\n\r",
       ch);
    return;
  }

  if (type == TP_MOB) {
    /* locate the beast */
    if (!(mob = get_char_vis(ch, name))) {
      send_to_char("I don't know anyone by that name...\n\r",
         ch);
      return;
    }
    switch(field) {
      case 1:
        if (!IS_NPC(mob) && GET_LEVEL(ch) < (IMO+25)) {
          send_to_char("You can't change that field for players.", ch);
          return;
        }
        ch->desc->str = &GET_NAME(mob);
        if (!IS_NPC(mob))
          send_to_char(
            "WARNING: You have changed the name of a player.\n\r", ch);
      break;
      case 2:
         if (!IS_NPC(mob)) {
          send_to_char(
           "That field is for monsters only.\n\r", ch);
           return;
         }
         ch->desc->str = &mob->player.short_descr;
      break;
      case 3:
         if (!IS_NPC(mob)) {
           send_to_char(
           "That field is for monsters only.\n\r", ch);
           return;
         }
         ch->desc->str = &mob->player.long_descr;
      break;
      case 4:ch->desc->str = &mob->player.description; break;
      case 5:
         if (IS_NPC(mob)) {
           send_to_char("Monsters have no titles.\n\r", ch);
           return;
         }
         ch->desc->str = &mob->player.title;
      break;
      default:
         send_to_char(
            "That field is undefined for monsters.\n\r", ch);
         return;
      break;
    }
  } else {  /* type == TP_OBJ */
    /* locate the object */
    if (!(obj = get_obj_vis(ch, name))) {
      send_to_char("Can't find such a thing here..\n\r", ch);
      return;
    }
    switch(field) {
      case 1: ch->desc->str = &obj->name; break;
      case 2: ch->desc->str = &obj->short_description; break;
      case 3: ch->desc->str = &obj->description; break;
      case 4:
        if (!*string) {
          send_to_char("You have to supply a keyword.\n\r", ch);
          return;
        }
        /* try to locate extra description */
        for (ed = obj->ex_description; ; ed = ed->next)
          if (!ed) /* the field was not found. create a new one. */
          {
            CREATE(ed , struct extra_descr_data, 1);
            ed->next = obj->ex_description;
            obj->ex_description = ed;
            CREATE(ed->keyword, char, strlen(string) + 1);
            strcpy(ed->keyword, string);
            ed->description = 0;
            ch->desc->str = &ed->description;
            send_to_char("New field.\n\r", ch);
            break;
          }
          else if (!str_cmp(ed->keyword, string)) /* the field exists */
          {
            free(ed->description);
            ed->description = 0;
            ch->desc->str = &ed->description;
            send_to_char(
              "Modifying description.\n\r", ch);
            break;
          }
        ch->desc->max_str = MAX_STRING_LENGTH;
        return; /* the stndrd (see below) procedure does not apply here */
      break;
      case 6: 
        if (!*string) {
          send_to_char("You must supply a field name.\n\r", ch);
          return;
        }
        /* try to locate field */
        for (ed = obj->ex_description; ; ed = ed->next)
          if (!ed) {
            send_to_char("No field with that keyword.\n\r", ch);
            return;
          }
          else if (!str_cmp(ed->keyword, string))
          {
            free(ed->keyword);
            if (ed->description)
              free(ed->description);
            
            /* delete the entry in the desr list */            
            if (ed == obj->ex_description)
              obj->ex_description = ed->next;
            else
            {
              for(tmp = obj->ex_description; tmp->next != ed; 
                tmp = tmp->next);
              tmp->next = ed->next;
            }
            free(ed);

            send_to_char("Field deleted.\n\r", ch);
            return;
          }
      break;        
      default:
         send_to_char(
            "That field is undefined for objects.\n\r", ch);
         return;
      break;
    }
  }

  if(*ch->desc->str)
    free(*ch->desc->str);

  if (*string) { /* there was a string in the argument array */
    if (strlen(string) > length[field - 1]) {
      send_to_char("String too long - truncated.\n\r", ch);
      *(string + length[field - 1]) = '\0';
    }
    CREATE(*ch->desc->str, char, strlen(string) + 1);
    strcpy(*ch->desc->str, string);
    ch->desc->str = 0;
    send_to_char("Ok.\n\r", ch);
  } else {
    send_to_char("Enter string. terminate with '@'.\n\r", ch);
    *ch->desc->str = 0;
    ch->desc->max_str = length[field - 1];
  }
}

void do_title(struct char_data *ch, char *arg, int cmd)
{
  int len;
  char s[MAX_STRING_LENGTH];
  char name[128],tit[128];
  struct char_data *vict;

  if(IS_SET(ch->specials.act,PLR_CRIMINAL)){
    send_to_char("Your ability to title has been taken away.\n\r",ch);
    return;
  }
  if(GET_LEVEL(ch) < 30){
    if(!IS_SET(ch->specials.act,PLR_TITLE)){
      send_to_char("You can't now.\n\r",ch);
      return;
    } else {
      REMOVE_BIT(ch->specials.act,PLR_TITLE);
    }
  }
  if(cmd==175){
    half_chop(arg,name,tit);
    if(vict=get_char(name)){
      if(vict->player.title)
        free(vict->player.title);
      vict->player.title=(char *)strdup(tit);
      send_to_char("OK.\n\r",ch);
    } else {
      send_to_char("Whom?\n\r",ch);
    }
    return;
  }
  if(*arg == ' ') ++arg;
  len=strlen(arg);
  if((len==0)||(len > 60))
    strcpy(tit,"the Inept Titler");
  else
    strcpy(tit,arg);
  if(ch->player.title)
    free(ch->player.title);
  ch->player.title=(char *)strdup(tit);
  send_to_char("Hmmm, OK.\n\r",ch);
} 
void do_rename(struct char_data *ch, char *arg, int cmd)
{
  char s[MAX_STRING_LENGTH];
  char name[MAX_STRING_LENGTH],tit[4*MAX_STRING_LENGTH], *p;
  struct obj_data *obj;
/*
      case 1: ch->desc->str = &obj->name; break;
      case 2: ch->desc->str = &obj->short_description; break;
      case 3: ch->desc->str = &obj->description; break;
*/
  if(cmd==68){
    half_chop(arg,name,tit);
    if(!*name){
      send_to_char("Huh?\n\r",ch);
      return;
    }
    if(strlen(tit) > 80){
      send_to_char("Blow me!\n\r",ch);
      return;
    }
    if(obj=get_obj_in_list_vis(ch,name,ch->carrying)){
      if(obj->short_description)
        free(obj->short_description);
      obj->short_description=(char *)strdup(tit);
      p=(char *)rindex(tit,' ');
      if(p)
        p++;
      else
        p=tit;
      if((strlen(obj->name)+strlen(p)) > 100){
        send_to_char("Bite my ass!\n\r",ch);
        return;
      }
      if(obj->name){
        sprintf(s,"%s %s",obj->name,p);
        free(obj->name);
      } else
        strcpy(s,p);
      obj->name=(char *)strdup(s);
      send_to_char("OK.\n\r",ch);
    } else {
      send_to_char("What?\n\r",ch);
    }
    return;
  }
} 

/* modification of malloc'ed strings in chars/objects */
      
/* db stuff *********************************************** */


/* One_Word is like one_argument, execpt that words in quotes "" are */
/* regarded as ONE word                                              */

char *one_word(char *argument, char *first_arg )
{
  int found, begin, look_at;

  found = begin = 0;

  do
  {
    for ( ;isspace(*(argument + begin)); begin++);

    if (*(argument+begin) == '\"') {  /* is it a quote */

      begin++;

      for (look_at=0; (*(argument+begin+look_at) >= ' ') && 
          (*(argument+begin+look_at) != '\"') ; look_at++)
        *(first_arg + look_at) = LOWER(*(argument + begin + look_at));

      if (*(argument+begin+look_at) == '\"')
        begin++;

    } else {

      for (look_at=0; *(argument+begin+look_at) > ' ' ; look_at++)
        *(first_arg + look_at) = LOWER(*(argument + begin + look_at));

    }

    *(first_arg + look_at) = '\0';
    begin += look_at;
  }
  while (fill_word(first_arg));

  return(argument+begin);
}


struct help_index_element *build_help_index(FILE *fl, int *num)
{
  int nr = -1, issorted, i;
  struct help_index_element *list = 0, mem;
  char buf[81], tmp[81], *scan;
  long pos;

  for (;;)
  {
    pos = ftell(fl);
    fgets(buf, 81, fl);
    *(buf + strlen(buf) - 1) = '\0';
    scan = buf;
    for (;;)
    {
      /* extract the keywords */
      scan = one_word(scan, tmp);

      if (!*tmp)
        break;

      if (!list)
      {
        CREATE(list, struct help_index_element, 1);
        nr = 0;
      }
      else
        RECREATE(list, struct help_index_element, ++nr + 1);

      list[nr].pos = pos;
      CREATE(list[nr].keyword, char, strlen(tmp) + 1);
      strcpy(list[nr].keyword, tmp);
    }
    /* skip the text */
    do
      fgets(buf, 81, fl);
    while (*buf != '#');
    if (*(buf + 1) == '~')
      break;
  }
  /* we might as well sort the stuff */
  do
  {
    issorted = 1;
    for (i = 0; i < nr; i++)
      if (str_cmp(list[i].keyword, list[i + 1].keyword) > 0)
      {
        mem = list[i];
        list[i] = list[i + 1];
        list[i + 1] = mem;
        issorted = 0;
      }
  }
  while (!issorted);

  *num = nr;
  return(list);
}



void page_string(struct descriptor_data *d, char *str, int keep_internal)
{
  if (!d)
    return;

  if (keep_internal)
  {
    CREATE(d->showstr_head, char, strlen(str) + 1);
    strcpy(d->showstr_head, str);
    d->showstr_point = d->showstr_head;
  }
  else
    d->showstr_point = str;

  show_string(d, "");
}



void show_string(struct descriptor_data *d, char *input)
{
  char buffer[MAX_STRING_LENGTH], buf[MAX_INPUT_LENGTH];
  register char *scan, *chk;
  int lines = 0, toggle = 1;

  one_argument(input, buf);

  if (*buf)
  {
    if (d->showstr_head)
    {
      free(d->showstr_head);
      d->showstr_head = 0;
    }
    d->showstr_point = 0;
    return;
  }

  /* show a chunk */
  for (scan = buffer;; scan++, d->showstr_point++)
    if((((*scan = *d->showstr_point) == '\n') || (*scan == '\r')) &&
      ((toggle = -toggle) < 0))
      lines++;
    else if (!*scan || (lines >= 22))
    {
      *scan = '\0';
      SEND_TO_Q(buffer, d);

      /* see if this is the end (or near the end) of the string */
      for (chk = d->showstr_point; isspace(*chk); chk++);
      if (!*chk)
      {
        if (d->showstr_head)
        {
          free(d->showstr_head);
          d->showstr_head = 0;
        }
        d->showstr_point = 0;
      }
      return;
    }
}


