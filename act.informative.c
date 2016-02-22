#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "limits.h"

/* extern variables */

extern char *action_bits[];
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern char news[MAX_STRING_LENGTH];
extern char info[MAX_STRING_LENGTH];
extern char spellinfo[MAX_STRING_LENGTH];
extern char *dirs[]; 
extern char *where[];
extern char *color_liquid[];
extern char *fullness[];
extern char *spells[];
extern char *command[];
extern struct command_info cmd_info[];
extern int baddoms;
extern char baddomain[BADDOMS][BADSTRLEN];
extern int newbeedoms;
extern char newbeedomain[BADDOMS][BADSTRLEN];
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern int top_of_zone_table;

/* extern functions */

void sprintbit(long vektor, char *names[], char *result);
struct time_info_data age(struct char_data *ch);
void page_string(struct descriptor_data *d, char *str, int keep_internal);

/* intern functions */

void list_obj_to_char(struct obj_data *list,struct char_data *ch, int mode,
  bool show);

int reboottime;

static char *cond_name[]={
  "Alcohol Blood Level    : ",
  "Solid stomach content  : ",
  "Liquid stomach content : "
};

/* Procedures related to 'look' */

void argument_split_2(char *argument, char *first_arg, char *second_arg) {
  int look_at, found, begin;
  found = begin = 0;

  /* Find first non blank */
  for ( ;*(argument + begin ) == ' ' ; begin++);

  /* Find length of first word */
  for (look_at=0; *(argument+begin+look_at) > ' ' ; look_at++)

  /* Make all letters lower case, AND copy them to first_arg */
  *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
  *(first_arg + look_at) = '\0';
  begin += look_at;

  /* Find first non blank */
  for ( ;*(argument + begin ) == ' ' ; begin++);

  /* Find length of second word */
  for ( look_at=0; *(argument+begin+look_at)> ' ' ; look_at++)

  /* Make all letters lower case, AND copy them to second_arg */
  *(second_arg + look_at) = LOWER(*(argument + begin + look_at));
  *(second_arg + look_at)='\0';
  begin += look_at;
}

struct obj_data *get_object_in_equip_vis(struct char_data *ch,
  char *arg, struct obj_data *equipment[], int *j) {

  for ((*j) = 0; (*j) < MAX_WEAR ; (*j)++)
    if (equipment[(*j)])
      if (CAN_SEE_OBJ(ch,equipment[(*j)]))
        if (isname(arg, equipment[(*j)]->name))
          return(equipment[(*j)]);

  return (0);
}

char *find_ex_description(char *word, struct extra_descr_data *list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word,i->keyword))
      return(i->description);
  return(0);
}


static int eqorder[]={
  WEAR_LIGHT,    WEAR_HEAD,
  WEAR_EARS,     WEAR_NECK_1,
  WEAR_FACE,     WEAR_NECK_2,
  WEAR_BODY,     WEAR_ABOUT,
  WEAR_ARMS,     WEAR_HANDS,
  WEAR_FINGER_L, WEAR_WRIST_L,
  WEAR_FINGER_R, WEAR_WRIST_R,
  WEAR_WAISTE,   WEAR_SHIELD,
  WEAR_LEGS,     WEAR_RADIO, 
  WEAR_FEET,     WEAR_ANKLES,
  WIELD,         HOLD
};

static char *eqfmt[]={
  "%6s:%c%-30s | ",
  "%6s:%c%s\n\r"
};
void equiplist(char buffer[], struct char_data *t, struct char_data *v)
{
  int i,j;
  char *fp, buf[MAX_STRING_LENGTH];
  struct obj_data *o;

  buffer[0]=0;
  for (j=0; j< MAX_WEAR; j++) {
    fp = eqfmt[j&1];
    i = eqorder[j];
    if (o = t->equipment[i]) {
      if (CAN_SEE_OBJ(v,o)){
        sprintf(buf,fp,where[i],
          IS_OBJ_STAT(o,ITEM_HUM) ? '+' : ' ',o->short_description);
      } else {
        sprintf(buf,fp,where[i],' ',"???");
      }
    } else {
      sprintf(buf,fp,where[i],' ',"nothing");
    }
    strcat(buffer,buf);
  }
  if(MAX_WEAR & 1)
    strcat(buffer,"\n\r");
}
void show_obj_to_char(struct obj_data *object, struct char_data *ch, int mode)
{
  char buffer[MAX_STRING_LENGTH] = "\0";
  char *temp_desc;
  struct obj_data *i;
  int temp;
  bool found;

  if ((mode == 0) && object->description)
    strcpy(buffer,object->description);
  else   if (object->short_description && ((mode == 1) ||
        (mode == 2) || (mode==3) || (mode == 4))) 
    strcpy(buffer,object->short_description);
  else if (mode == 5) {
    if((object->obj_flags.type_flag != ITEM_DRINKCON)) {
      strcpy(buffer,"You see nothing special..");
    }
    else /* ITEM_TYPE == ITEM_DRINKCON */
    {
      strcpy(buffer, "It looks like a drink container.");
    }
  }

  if (mode != 3) { 
    found = FALSE;
    if (IS_OBJ_STAT(object,ITEM_INVISIBLE)) {
       strcat(buffer,"(invisible)");
       found = TRUE;
    }
    if(IS_OBJ_STAT(object,ITEM_NODROP) && IS_AFFECTED(ch,AFF_DETECT_EVIL)) {
       strcat(buffer,"..it glows red!");
       found = TRUE;
    }
    if (IS_OBJ_STAT(object,ITEM_MAGIC) && IS_AFFECTED(ch,AFF_DETECT_MAGIC)) {
       strcat(buffer,"..it glows blue!");
       found = TRUE;
    }
    if (IS_OBJ_STAT(object,ITEM_GLOW)) {
      strcat(buffer,"..it is glowing!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object,ITEM_HUM)) {
      strcat(buffer,"..it is humming!");
      found = TRUE;
    }
  }
  strcat(buffer, "\n\r");
  page_string(ch->desc, buffer, 1);
#ifdef BLOWME
  if (((mode == 2) || (mode == 4)) && (GET_ITEM_TYPE(object) == 
    ITEM_CONTAINER)) {
    strcpy(buffer,"The ");
    strcat(buffer,fname(object->name));
    strcat(buffer," contains:\n\r");
    send_to_char(buffer, ch);
    if (mode == 2) list_obj_to_char(object->contains, ch, 1,TRUE);
    if (mode == 4) list_obj_to_char(object->contains, ch, 3,TRUE);
  }
#endif
}

void list_obj_to_char(struct obj_data *list,struct char_data *ch, int mode, 
  bool show) {
  struct obj_data *i;
  bool found;

  found = FALSE;
  for ( i = list ; i ; i = i->next_content ) { 
    if (CAN_SEE_OBJ(ch,i)) {
      show_obj_to_char(i, ch, mode);
      found = TRUE;
    }    
  }  
  if ((! found) && (show)) send_to_char("Nothing\n\r", ch);
}

static char *auras[]={
  " glowing",
  " sick",
  " twitching",
  " quivering",
  " vibrating",
  " stupid",
  " tipsy",
  " drunk",
  " disgustingly drunk"
};

void show_char_to_char(struct char_data *i, struct char_data *ch, int mode)
{
  char buffer[MAX_STRING_LENGTH];
  int f,h,j,k, found, percent;
  struct obj_data *tmp_obj;
  struct char_data *mnt;

  if (mode == 0) {
    if(GET_LEVEL(ch) < (IMO+600)){
      if(!CAN_SEE(ch,i))
        return;
      if(IS_AFFECTED(i, AFF_HIDE)){
        if(CANSEEHID(ch))
          if(GET_LEVEL(i) < (IMO+1))
            send_to_char("You sense a hidden life form in the room.\n\r",ch);
        return;
      }
    }
    if (!(i->player.long_descr)||(GET_POS(i) != i->specials.default_pos)){
      if (!IS_NPC(i)) {  
        strcpy(buffer,GET_NAME(i));
        if(GET_TITLE(i)){
          strcat(buffer," ");
          strcat(buffer,GET_TITLE(i));
        }
      } else {
        strcpy(buffer, i->player.short_descr);
        CAP(buffer);
      }
      if(IS_AFFECTED(i,AFF_INVISIBLE))
         strcat(buffer," (invisible)");
      switch(GET_POS(i)) {
        case POSITION_STUNNED  : 
          strcat(buffer," is lying here, stunned."); break;
        case POSITION_INCAP    : 
          strcat(buffer," is lying here, incapacitated."); break;
        case POSITION_MORTALLYW: 
          strcat(buffer," is lying here, mortally wounded."); break;
        case POSITION_DEAD     : 
          strcat(buffer," is lying here, dead."); break;
        case POSITION_STANDING : 
          strcat(buffer," is standing here."); break;
        case POSITION_SITTING  : 
          strcat(buffer," is sitting here.");  break;
        case POSITION_RESTING  : 
          strcat(buffer," is resting here.");  break;
        case POSITION_SLEEPING : 
          strcat(buffer," is sleeping here."); break;
        case POSITION_FIGHTING :
          if (i->specials.fighting) {
            strcat(buffer," is here, fighting ");
            if (i->specials.fighting == ch)
              strcat(buffer," YOU!");
            else {
              if (i->in_room == i->specials.fighting->in_room)
                if (IS_NPC(i->specials.fighting))
                  strcat(buffer, i->specials.fighting->player.short_descr);
                else
                  strcat(buffer, GET_NAME(i->specials.fighting));
              else
                strcat(buffer, "someone who has already left.");
            }
          } else /* NIL fighting pointer */
              strcat(buffer," is here struggling with thin air.");
          break;
        default : strcat(buffer," is floating here."); break;
      }
      if (IS_AFFECTED(ch, AFF_DETECT_EVIL)) {
        if (IS_EVIL(i))
          strcat(buffer, " (Red Aura)");
      }
      strcat(buffer,"\n\r");
      send_to_char(buffer, ch);
    } else  /* npc with long */ {
      if (IS_AFFECTED(i,AFF_INVISIBLE))
        strcpy(buffer,"*");
      else
        *buffer = '\0';
      if (IS_AFFECTED(ch, AFF_DETECT_EVIL)) {
        if (IS_EVIL(i))
          strcat(buffer, " (Red Aura)");
      }
      strcat(buffer, i->player.long_descr);
      send_to_char(buffer, ch);
    }
    if(!IS_SET(ch->specials.act,PLR_VERYBRIEF)){
      f=0; k=0;
      if (IS_AFFECTED(i,AFF_SANCTUARY)){ f |= 1; k++;}
      if (IS_AFFECTED(i,AFF_POISON)){ f |= 2; k++;}
      if (IS_AFFECTED(i,AFF_REGEN)){ f |= 4; k++;}
      if (IS_AFFECTED(i,AFF_HASTE)){ f |= 8; k++;}
      if (IS_AFFECTED(i,AFF_HYPERREGEN)){ f |= 16; k++;}
      if (IS_AFFECTED(i,AFF_STUPIDITY)){ f |= 32; k++;}
      if ((j=GET_COND(i,DRUNK)) > 0){
        if(j < 10){
          f |= 64; k++;
        } else if(j < 20){
          f |= 128; k++;
        } else {
          f |= 256; k++;
        }
      }
      if(f){
        strcpy(buffer,"$n is");
        for(h=j=0;h<9;h++){
          if(f & 1){
            j++;
            strcat(buffer,auras[h]);
            if((k > 1)&&(j==(k-1)))
              strcat(buffer," and");
            else if(j < k)
              strcat(buffer,",");
          }
          f>>=1;
        }
        strcat(buffer,".");
        act(buffer, FALSE, i, 0, ch, TO_VICT);
      }
    }
  } else if (mode & 1) {
    if (i->player.description){
      send_to_char(i->player.description, ch);
    } else {
      act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);
    }
    if (GET_MAX_HIT(i) > 1000000)
      percent = GET_HIT(i)/(GET_MAX_HIT(i)/100);
    else if (GET_MAX_HIT(i) > 0)
      percent = (100*GET_HIT(i))/GET_MAX_HIT(i);
    else
      percent = -1; /* How could MAX_HIT be < 1?? */
    if (IS_NPC(i))
      strcpy(buffer, i->player.short_descr);
    else
      strcpy(buffer, GET_NAME(i));
    if (percent >= 100)
      strcat(buffer, " is in an excellent condition.\n\r");
    else if (percent >= 90)
      strcat(buffer, " has a few scratches.\n\r");
    else if (percent >= 75)
      strcat(buffer, " has some small wounds and bruises.\n\r");
    else if (percent >= 50)
      strcat(buffer, " has quite a few wounds.\n\r");
    else if (percent >= 30)
      strcat(buffer, " has some big nasty wounds and scratches.\n\r");
    else if (percent >= 15)
      strcat(buffer, " looks pretty hurt.\n\r");
    else if (percent >= 0)
      strcat(buffer, " is in an awful condition.\n\r");
    else
      strcat(buffer, " is bleeding awfully from big wounds.\n\r");
    send_to_char(buffer, ch);
    if(mode == 3)
      return;
    found = FALSE;
    for (j=0; j< MAX_WEAR; j++) {
      if (i->equipment[j]) {
        if (CAN_SEE_OBJ(ch,i->equipment[j])) {
          found = TRUE;
        }
      }
    }
    if (found) {
      act("\n\r$n is using:", FALSE, i, 0, ch, TO_VICT);
      for (j=0; j< MAX_WEAR; j++) {
        if (i->equipment[j]) {
          if (CAN_SEE_OBJ(ch,i->equipment[j])) {
            sprintf(buffer,"%-6s: ",where[j]);
            send_to_char(buffer,ch);
            show_obj_to_char(i->equipment[j],ch,1);
          }
        }
      }
    }
    if(CANSEEINV(ch) && !IS_SET(ch->specials.act,PLR_BRIEF)){
      found = FALSE;
      send_to_char("\n\rYou attempt to peek at the inventory:\n\r", ch);
      for(tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
        if(CAN_SEE_OBJ(ch, tmp_obj) && (number(1,30) < GET_LEVEL(ch))) {
          show_obj_to_char(tmp_obj, ch, 1);
          found = TRUE;
        }
      }
      if (!found)
        send_to_char("You can't see anything.\n\r", ch);
    }
  } else if (mode == 2) {
    /* Lists inventory */
    act("$n is carrying:", FALSE, i, 0, ch, TO_VICT);
    list_obj_to_char(i->carrying,ch,1,TRUE);
  }
}



void list_char_to_char(struct char_data *list, struct char_data *ch, 
  int mode) {
  struct char_data *i;

  for (i = list; i ; i = i->next_in_room) {
    if((ch!=i)&&(CANSEEHID(ch)||(GET_LEVEL(ch) > (IMO+1))||
       (CAN_SEE(ch,i) && !IS_AFFECTED(i, AFF_HIDE))))
      show_char_to_char(i,ch,0); 
  } 
}

void do_look(struct char_data *ch, char *argument, int cmd)
{
  char buffer[MAX_STRING_LENGTH];
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  int keyword_no,i,j,bits,temp,newroom;
  bool found;
  struct obj_data *tmp_object, *found_object;
  struct char_data *tmp_char;
  char *tmp_desc;
  static char *exitchar = "NESWUD";
  static char *keywords[]= { 
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "in",
    "at",
    "",  /* Look at '' case */
    "\n" };

  if (!ch->desc)
    return;
  if (GET_POS(ch) < POSITION_SLEEPING)
    send_to_char("You can't see anything but stars!\n\r", ch);
  else if (GET_POS(ch) == POSITION_SLEEPING)
    send_to_char("You can't see anything, you're sleeping!\n\r", ch);
  else if (IS_AFFECTED(ch, AFF_BLIND) )
    send_to_char("You can't see a damn thing, you're blinded!\n\r", ch);
  else if(IS_DARK(ch->in_room) &&
           (!OMNI(ch)) && (!CANINFRA(ch)))
    send_to_char("It is pitch black...\n\r", ch);
  else {
    argument_split_2(argument,arg1,arg2);
    keyword_no = search_block(arg1, keywords, FALSE); /* Partial Match */
    if ((keyword_no == -1) && *arg1) {
      keyword_no = 7;
      strcpy(arg2, arg1); /* Let arg2 become the target object (arg1) */
    }
    if((cmd == 237) && (keyword_no != 7)){
      send_to_char("Who?\n\r",ch);
      return;
    }
    found = FALSE;
    tmp_object = 0;
    tmp_char   = 0;
    tmp_desc   = 0;
    switch(keyword_no) {
      /* look <dir> */
      case 0 :
      case 1 :
      case 2 : 
      case 3 : 
      case 4 :
      case 5 : {   
        if (EXIT(ch, keyword_no)) {
          if (EXIT(ch, keyword_no)->general_description) {
            send_to_char(EXIT(ch, keyword_no)-> general_description, ch);
          } else {
            send_to_char("You see nothing special.\n\r", ch);
          }
          if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_CLOSED) && 
            (EXIT(ch, keyword_no)->keyword)) {
              sprintf(buffer, "The %s is closed.\n\r",
                fname(EXIT(ch, keyword_no)->keyword));
              send_to_char(buffer, ch);
          }  else {
            if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_ISDOOR) &&
                EXIT(ch, keyword_no)->keyword) {
              sprintf(buffer, "The %s is open.\n\r",
                fname(EXIT(ch, keyword_no)->keyword));
              send_to_char(buffer, ch);
            }
          }
          if((CANFARSEE(ch)) && (CAN_GO(ch,keyword_no))){
           newroom=world[ch->in_room].dir_option[keyword_no]->to_room;
           list_char_to_char(world[newroom].people,ch,0);
          }
        } else {
            send_to_char("Nothing special there...\n\r", ch);
        }
      }
      break;

      /* look 'in'  */
      case 6: {
        if (*arg2) {
          /* Item carried */
          bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM |
                   FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);
          if (bits) { /* Found something */
            if (GET_ITEM_TYPE(tmp_object)== ITEM_DRINKCON)
            {
              if (tmp_object->obj_flags.value[1] <= 0) {
                act("It is empty.", FALSE, ch, 0, 0, TO_CHAR);
              } else {
                temp=((tmp_object->obj_flags.value[1]*3)/tmp_object->obj_flags.value[0]);
                sprintf(buffer,"It's %sfull of a %s liquid.\n\r",
                fullness[temp],color_liquid[tmp_object->obj_flags.value[2]]);
                send_to_char(buffer, ch);
              }
            } else if (GET_ITEM_TYPE(tmp_object) == ITEM_CONTAINER) {
              if (!IS_SET(tmp_object->obj_flags.value[1],CONT_CLOSED)) {
                send_to_char(fname(tmp_object->name), ch);
                switch (bits) {
                  case FIND_OBJ_INV :
                    send_to_char(" (carried) : \n\r", ch);
                    break;
                  case FIND_OBJ_ROOM :
                    send_to_char(" (here) : \n\r", ch);
                    break;
                  case FIND_OBJ_EQUIP :
                    send_to_char(" (used) : \n\r", ch);
                    break;
                }
                list_obj_to_char(tmp_object->contains, ch, 2, TRUE);
              } else
                send_to_char("It is closed.\n\r", ch);
            } else {
              send_to_char("That is not a container.\n\r", ch);
            }
          } else { /* wrong argument */
            send_to_char("You do not see that item here.\n\r", ch);
          }
        } else { /* no argument */
          send_to_char("Look in what?!\n\r", ch);
        }
      }
      break;
      case 7 : {
        if (*arg2) {
          bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM |
            FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &tmp_char, &found_object);
          if (tmp_char) {
            show_char_to_char(tmp_char, ch, (cmd==237) ? 3 : 1);
            if (ch != tmp_char) {
              act("$n looks at you.", TRUE, ch, 0, tmp_char, TO_VICT);
              act("$n looks at $N.", TRUE, ch, 0, tmp_char, TO_NOTVICT);
            }
            return;
          }
          if(cmd==237){
            send_to_char("Who?\n\r",ch);
            return;
          }
          if (!found) {
            tmp_desc = find_ex_description(arg2, 
              world[ch->in_room].ex_description);
            if (tmp_desc) {
              page_string(ch->desc, tmp_desc, 0);
              return; /* RETURN SINCE IT WAS A ROOM DESCRIPTION */
              /* Old system was: found = TRUE; */
            }
          }
          /* Search for extra descriptions in items */
          /* Equipment Used */
          if (!found) {
            for (j = 0; j< MAX_WEAR && !found; j++) {
              if (ch->equipment[j]) {
                if (CAN_SEE_OBJ(ch,ch->equipment[j])) {
                  tmp_desc = find_ex_description(arg2, 
                    ch->equipment[j]->ex_description);
                  if (tmp_desc) {
                    page_string(ch->desc, tmp_desc, 1);
                    found = TRUE;
                  }
                }
              }
            }
          }
          /* In inventory */
          if (!found) {
            for(tmp_object = ch->carrying; 
              tmp_object && !found; 
              tmp_object = tmp_object->next_content) {
              if CAN_SEE_OBJ(ch, tmp_object) {
                tmp_desc = find_ex_description(arg2, 
                  tmp_object->ex_description);
                if (tmp_desc) {
                  page_string(ch->desc, tmp_desc, 1);
                  found = TRUE;
                }
              }
            }
          }

          /* Object In room */

          if (!found) {
            for(tmp_object = world[ch->in_room].contents; 
              tmp_object && !found; 
              tmp_object = tmp_object->next_content) {
              if CAN_SEE_OBJ(ch, tmp_object) {
                tmp_desc = find_ex_description(arg2, 
                  tmp_object->ex_description);
                if (tmp_desc) {
                  page_string(ch->desc, tmp_desc, 1);
                  found = TRUE;
                }
              }
            }
          }
          /* wrong argument */

          if (bits) { /* If an object was found */
            if (!found)
              show_obj_to_char(found_object, ch, 5); /* Show no-description */
            else
              show_obj_to_char(found_object, ch, 6); /* Find hum, glow etc */
          } else if (!found) {
            send_to_char("You do not see that here.\n\r", ch);
          }
        } else {
          /* no argument */

          send_to_char("Look at what?\n\r", ch);
        }
      }
      break;


      /* look ''    */ 
      case 8 : {
        if(world[ch->in_room].name){
          sprintf(buffer,"%s\n\r",world[ch->in_room].name);
          send_to_char(buffer, ch);
        }
        if (!IS_SET(ch->specials.act, PLR_BRIEF))
          send_to_char(world[ch->in_room].description, ch);
        if (IS_SET(ch->specials.act, PLR_AUTOEXIT)){
          strcpy(buffer,"Exits: ");
          j = strlen(buffer);
          for(i=0;i<6;i++)
            if(EXIT(ch,i))
              if(EXIT(ch, i)->to_room != NOWHERE &&
                 !IS_SET(EXIT(ch, i)->exit_info, EX_CLOSED))
                   buffer[j++] = exitchar[i];
          buffer[j]=0;
          strcat(buffer,"\n\r");
          send_to_char(buffer,ch);
        }
        list_obj_to_char(world[ch->in_room].contents, ch, 0,FALSE);
        list_char_to_char(world[ch->in_room].people, ch, 0);
      }
      break;

      /* wrong arg  */
      case -1 : 
        send_to_char("Sorry, I didn't understand that!\n\r", ch);
        break;
    }
  }
}

/* end of look */


void do_bank(struct char_data *ch, char *argument, int cmd)
{
  send_to_char("You can only do that at the bank.\n\r",ch);
}


void do_read(struct char_data *ch, char *argument, int cmd)
{
  char buf[100];

  /* This is just for now - To be changed later.! */
  sprintf(buf,"at %s",argument);
  do_look(ch,buf,15);
}



void do_examine(struct char_data *ch, char *argument, int cmd)
{
  char name[100], buf[100];
  int bits;
  struct char_data *tmp_char;
  struct obj_data *tmp_object;

  sprintf(buf,"at %s",argument);
  do_look(ch,buf,15);

  one_argument(argument, name);

  if (!*name)
  {
    send_to_char("Examine what?\n\r", ch);
    return;
  }

  bits = generic_find(name, FIND_OBJ_INV | FIND_OBJ_ROOM |
         FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_ITEM_TYPE(tmp_object)==ITEM_DRINKCON) ||
        (GET_ITEM_TYPE(tmp_object)==ITEM_CONTAINER)) {
      send_to_char("When you look inside, you see:\n\r", ch);
      sprintf(buf,"in %s",argument);
      do_look(ch,buf,15);
    } else {
      if(GET_ITEM_TYPE(tmp_object)==ITEM_FIREWEAPON){
        sprintf(buf,"There are %d shots left.\n\r",
           tmp_object->obj_flags.value[0]);
        send_to_char(buf,ch);
      }
    }
  }
}



void do_exits(struct char_data *ch, char *argument, int cmd)
{
  int door;
  char *s, buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  char *exits[] =
  {  
    "North",
    "East ",
    "South",
    "West ",
    "Up   ",
    "Down "
  };

  *buf = '\0';
  for (door = 0; door <= 5; door++)
    if (EXIT(ch, door))
      if(EXIT(ch, door)->to_room != NOWHERE){
        if(IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
          sprintf(buf+strlen(buf), "%s - Closed\n\r", exits[door]);
        else if(IS_DARK(EXIT(ch, door)->to_room) && (!OMNI(ch)) &&
          (!CANINFRA(ch)))
          sprintf(buf+strlen(buf), "%s - Too dark to tell\n\r", exits[door]);
        else {
          s=world[EXIT(ch, door)->to_room].name;
          if(s){
            sprintf(buf+strlen(buf),"%s - %s (%d)\n\r",exits[door],
              s,EXIT(ch, door)->to_room);
          } else {
            sprintf(buf+strlen(buf), "%s - %s (%d)\n\r", exits[door],
              "Somewhere Strange",EXIT(ch, door)->to_room);
          }
        }
      }
  sprintf(buf2, "Obvious exits from %s (%d):\n\r",
    world[ch->in_room].name,ch->in_room);
  send_to_char(buf2, ch);
  if (*buf)
    send_to_char(buf, ch);
  else
    send_to_char("None.\n\r", ch);
}

void do_score(struct char_data *ch, char *argument, int cmd)
{
  extern char *player_bits[];
  struct time_info_data playing_time;
  char buf[MAX_STRING_LENGTH],buf2[512];
  struct time_info_data real_time_passed(time_t t2, time_t t1);
  struct affected_type *aff;
  int i,f,n,pp;

  sprintf(buf,
   "Age: %d years old, Alignment: %d, Level: %d, Spell level: %d.\n\r",
    GET_AGE(ch),GET_ALIGNMENT(ch),GET_LEVEL(ch),GET_SPELL_LEVEL(ch));
  send_to_char(buf,ch);
  sprintf(buf, 
    "You have %d(%d) hit, %d(%d) mana and %d(%d) moves.\n\r",
    GET_HIT(ch),GET_MAX_HIT(ch),
    GET_MANA(ch),GET_MAX_MANA(ch),
    GET_MOVE(ch),GET_MAX_MOVE(ch));
  send_to_char(buf,ch);
  sprintf(buf,"EXPERIENCE: %d, GOLD: %d, MR: %d, META: %d\n\r",
    GET_EXP(ch),GET_GOLD(ch),ch->specials.magres,ch->points.metapts);
  send_to_char(buf,ch);
  sprintf(buf,"Str: %d  Int: %d  Wis: %d  Dex: %d  Con: %d\n\r",
      GET_STR(ch),GET_INT(ch),GET_WIS(ch),GET_DEX(ch),GET_CON(ch));
  send_to_char(buf,ch);
  sprintf(buf,"Armor: %d, Hit Roll: %d, Damage Roll: %d, Practices: %d\n\r",
    ch->points.armor,
    ch->points.hitroll,ch->points.damroll,ch->specials.spells_to_learn);
  send_to_char(buf,ch);
  playing_time = real_time_passed((time(0)-ch->player.time.logon) +
     ch->player.time.played, 0);
  sprintf(buf,"You have been playing for %d days and %d hours.\n\r",
    playing_time.day,
    playing_time.hours);    
  send_to_char(buf, ch);
  if(!IS_NPC(ch) && (GET_LEVEL(ch) < (IMO-1))){
    if(GET_LEVEL(ch) <= 1250){
      f=XP4LEV(GET_LEVEL(ch)+1);
      if(f > GET_EXP(ch))
        sprintf(buf,"You need %d more experience points to gain a level.\n\r",
          f-GET_EXP(ch));
      else
        sprintf(buf,"You have %d MORE exp pts than you need to advance.\n\r",
          GET_EXP(ch)-f);
    } else {
      f=XP4LEV(GET_LEVEL(ch)+1-1250);
      if(f > GET_META(ch))
        sprintf(buf,"You need %d more meta points to gain a level.\n\r",
          f-GET_META(ch));
      else
        sprintf(buf,"You have %d MORE meta pts than you need to advance.\n\r",
          GET_META(ch)-f);
    }
    send_to_char(buf,ch);
  } 
  sprintf(buf,"You have made %d kills and died %d times.\n\r",
    ch->points.kills,ch->points.deaths);
  send_to_char(buf,ch);
  if(ch->specials.act){
    sprintbit(ch->specials.act, player_bits, buf2);
    sprintf(buf,"Your flags are: %s\n\r",buf2);
    send_to_char(buf,ch);
  }
  if(ch->affected) {
    i=0;
    sprintf(buf,"Spells:\n\r", ch);
    for(aff = ch->affected; aff; aff = aff->next) {
      if(GET_INT(ch) >= 40){
        sprintf(buf2,"%-18s%5d",spells[aff->type-1],aff->duration);
      } else {
        sprintf(buf2,"%18s",spells[aff->type-1]);
      }
      strcat(buf,buf2);
      i++;
      if(i==3){
        strcat(buf,"\n\r");
        send_to_char(buf,ch);
        buf[0]=0;
        i=0;
      } else {
        strcat(buf," | ");
      }
    }
    if(i){
      strcat(buf,"\n\r");
      send_to_char(buf,ch);
    }
  }
  for(i=0;i<3;i++){
    sprintf(buf,"%s%d\n\r",cond_name[i],GET_COND(ch,i));
    send_to_char(buf,ch);
  }
  sprintf(buf,"Your GUT can hold %d units.\n\r",GET_GUT(ch));
  send_to_char(buf,ch);
  switch(GET_POS(ch)) {
    case POSITION_DEAD : 
      send_to_char("You are DEAD!\n\r", ch); break;
    case POSITION_MORTALLYW :
      send_to_char("You are mortally wounded!\n\r", ch); break;
    case POSITION_INCAP : 
      send_to_char("You are incapacitated, slowly fading away\n\r", ch); break;
    case POSITION_STUNNED : 
      send_to_char("You are stunned! You can't move\n\r", ch); break;
    case POSITION_SLEEPING : 
      send_to_char("You are sleeping.\n\r",ch); break;
    case POSITION_RESTING  : 
      send_to_char("You are resting.\n\r",ch); break;
    case POSITION_SITTING  : 
      send_to_char("You are sitting.\n\r",ch); break;
    case POSITION_FIGHTING :
      if (ch->specials.fighting)
        act("You are fighting $N.\n\r", FALSE, ch, 0,
             ch->specials.fighting, TO_CHAR);
      else
        send_to_char("You are fighting thin air.\n\r", ch);
      break;
    case POSITION_STANDING : 
      send_to_char("You are standing.\n\r",ch); break;
    default :
      send_to_char("You are floating.\n\r",ch); break;
  }
}

void do_time(struct char_data *ch, char *argument, int cmd)
{
  char buf[100], *suf;
  int weekday, day;
  extern struct time_info_data time_info;
  extern const char *weekdays[];
  extern const char *month_name[];

  sprintf(buf, "It is %d o'clock %s, on ",
    ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
    ((time_info.hours >= 12) ? "pm" : "am") );

  weekday = ((35*time_info.month)+time_info.day+1) % 7;/* 35 days in a month */

  strcat(buf,weekdays[weekday]);
  strcat(buf,"\n\r");
  send_to_char(buf,ch);

  day = time_info.day + 1;   /* day in [1..35] */

  if (day == 1)
    suf = "st";
  else if (day == 2)
    suf = "nd";
  else if (day == 3)
    suf = "rd";
  else if (day < 20)
    suf = "th";
  else if ((day % 10) == 1)
    suf = "st";
  else if ((day % 10) == 2)
    suf = "nd";
  else if ((day % 10) == 3)
    suf = "rd";
  else
    suf = "th";

  sprintf(buf, "The %d%s Day of the %s, Year %d.\n\r",
    day,
    suf,
    month_name[time_info.month],
    time_info.year);

  send_to_char(buf,ch);
}


void do_weather(struct char_data *ch, char *argument, int cmd)
{
  extern struct weather_data weather_info;
  static char buf[100];
  char static *sky_look[4]= {
  "cloudless",
  "cloudy",
  "rainy",
  "lit by flashes of lightning"};

  if (OUTSIDE(ch)) {
    sprintf(buf, 
    "The sky is %s and %s.\n\r",
      sky_look[weather_info.sky],
      (weather_info.change >=0 ? "you feel a warm wind from south" :
       "your foot tells you bad weather is due"));
    send_to_char(buf,ch);
  } else
    send_to_char("You have no feeling about the weather at all.\n\r", ch);
}


void do_help(struct char_data *ch, char *argument, int cmd)
{
  extern char *spells[];   /* The list of spells (spells.c)         */
  extern int top_of_helpt;
  extern struct help_index_element *help_index;
  extern FILE *help_fl;
  extern char help[MAX_STRING_LENGTH];

  int i, no, chk, bot, top, mid, minlen;
  char buf[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH];

  if (!ch->desc)
    return;
  for(;isspace(*argument); argument++)  ;
  if (*argument)
  {
    if (!help_index)
    {
      send_to_char("No help available.\n\r", ch);
      return;
    }
    bot = 0;
    top = top_of_helpt;

    for (;;)
    {
      mid = (bot + top) / 2;
      minlen = strlen(argument);

      if (!(chk = strn_cmp(argument, help_index[mid].keyword, minlen)))
      {
        fseek(help_fl, help_index[mid].pos, 0);
        *buffer = '\0';
        for (;;)
        {
          fgets(buf, 80, help_fl);
          if (*buf == '#')
            break;
          strcat(buffer, buf);
          strcat(buffer, "\r");
        }
        page_string(ch->desc, buffer, 1);
        return;
      }
      else if (bot >= top)
      {
        send_to_char("There is no help on that word.\n\r", ch);
        return;
      }
      else if (chk > 0)
        bot = ++mid;
      else
        top = --mid;
    }
    return;
  }
  send_to_char(help, ch);
}

do_wizhelp(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  int no, i;

  if (IS_NPC(ch))
    return;
  send_to_char("The following privileged commands are available:\n\r\n\r", ch);
  *buf = '\0';
  for (no = 1, i = 0; *command[i] != '\n'; i++)
    if ((GET_LEVEL(ch) >= cmd_info[i+1].minimum_level) &&
      (cmd_info[i+1].minimum_level >= IMO)) {
      sprintf(buf + strlen(buf), "%-15s", command[i]);
      if (!(no % 5))
        strcat(buf, "\n\r");
      no++;
    }
  strcat(buf, "\n\r");
  page_string(ch->desc, buf, 1);
}

void do_who(struct char_data *ch, char *argument, int cmd)
{
  struct descriptor_data *d;
  struct char_data *p;
  char buf[MAX_STRING_LENGTH];
  int tlev,lev,tz,z,flag,condtype;

  if(argument){
    while(argument[0]==' ') ++argument;
    switch(argument[0]){
      case '+': condtype=1; ++argument; break;
      case '-': condtype=2; ++argument; break;
      case '=': condtype=3; ++argument; break;
      case '*': condtype=4; ++argument; z=world[ch->in_room].zone; break;
      case 'i': condtype=5; break;
      default: condtype=1;
    }
    if(condtype < 4)
      lev=atoi(argument);
  } else {
    condtype=0;
  }
  send_to_char("Players\n\r-------\n\r", ch);
  for (d = descriptor_list; d; d = d->next) {
   if(!d->connected){
    p=d->original ? d->original : d->character;
    if(CAN_SEE(ch,p)) {
     tlev=GET_LEVEL(p);
     if(condtype){
      if(condtype==4)
       tz=world[p->in_room].zone;
      switch(condtype){
       case 1: flag=(tlev >= lev); break;
       case 2: flag=(tlev <= lev); break;
       case 3: flag=(tlev == lev); break;
       case 4: flag=(tz == z); break;
       case 5: flag=p->specials.it; break;
      }
      if(!flag) continue;
     }
     sprintf(buf, "<%4d> %s %s\n\r",tlev,GET_NAME(p),
      p->player.title ? p->player.title : "the Dog");
     send_to_char(buf, ch);
    }
   }
  }
}

extern char *connected_types[];

void do_users(struct char_data *ch, char *argument, int cmd)
{
  char line[1024],buf[2048];
  struct descriptor_data *d;
  struct char_data *p;
  int n=0,flag,t;
  static most=0;

  one_argument(argument,line);
  if(!strcmp("-l",line)){
    buf[0]=0;
    for (d = descriptor_list; d; d = d->next) {
      if(!d->connected){
        p=d->original ? d->original : d->character;
        if(!CAN_SEE(ch,p)) continue;
        strcat(buf,GET_NAME(p));
        strcat(buf," ");
      }
    }
    strcat(buf,"\n\r");
    send_to_char(buf,ch);
    return;
  }
  flag=0;
  line[0]=0;
  for (d=descriptor_list;d;d=d->next) {
    if(!flag){
      p=d->original ? d->original : d->character;
      if(p){
        if(!CAN_SEE(ch,p)) continue;
        sprintf(line,"%2d%3d ",d->descriptor,p->specials.timer);
        sprintf(line+6,"%-15s%4d ",
         (d->connected==CON_PLYNG) ? GET_NAME(p) : "Not in game",GET_LEVEL(p));
        sprintf(line+26,"%5d%3d ",
          world[p->in_room].number,GET_POS(p));
      } else
        sprintf(line,"%2d    %5s%14s          ",
          d->descriptor,"UNDEF",connected_types[d->connected]);
      sprintf(line+35,"%-16s",d->host);
      strcat(line,"\n\r");
      send_to_char(line, ch);
      line[0]=0;
    }
    n++;
  }
  if(n > most) most=n;
  sprintf(line,"%d/%d active connections\n\r",n,most);
  send_to_char(line,ch);
  t=time(0)+30-reboottime;
  sprintf(line,"Running time %d:%02d\n\r",t/3600,(t%3600)/60);
  send_to_char(line,ch);
}
void do_radio(struct char_data *ch, char *argument, int cmd)
{
  char line[256];
  struct descriptor_data *d;
  struct char_data *v;
  struct obj_data *o;
  int n=0,t;

  line[0]=0;
  for (d=descriptor_list;d;d=d->next) {
    if (d->original){
      v=d->original;
    } else if (d->character){
      v=d->character;
    } else
      continue;
    if(!CAN_SEE(ch,v)) continue;
    if(!(o=v->equipment[WEAR_RADIO])) continue;
    ++n;
    sprintf(line+strlen(line),"%-18s%08x/%08x",
        GET_NAME(v),
        o->obj_flags.value[1],
        o->obj_flags.value[2]);
    if(n&1){
      strcat(line," | ");
    } else {
      strcat(line,"\n\r");
      send_to_char(line,ch);
      line[0]=0;
    }
  }
  if(n&1){
    strcat(line,"\n\r");
    send_to_char(line, ch);
  }
}
void do_inventory(struct char_data *ch, char *argument, int cmd) {
  send_to_char("You are carrying:\n\r", ch);
  list_obj_to_char(ch->carrying, ch, 1, TRUE);
}
/*
void do_equipment(struct char_data *ch, char *argument, int cmd) {
int j;
bool found;

  send_to_char("You are using:\n\r", ch);
  found = FALSE;
  for (j=0; j< MAX_WEAR; j++) {
    if (ch->equipment[j]) {
      if (CAN_SEE_OBJ(ch,ch->equipment[j])) {
        send_to_char(where[j],ch);
        show_obj_to_char(ch->equipment[j],ch,1);
        found = TRUE;
      } else {
        send_to_char(where[j],ch);
        send_to_char("Something.\n\r",ch);
        found = TRUE;
      }
    }
  }
  if(!found) {
    send_to_char(" Nothing.\n\r", ch);
  }
}
*/
void do_equipment(struct char_data *ch, char *argument, int cmd)
{
  char buffer[MAX_STRING_LENGTH];

  equiplist(buffer,ch,ch);
  send_to_char(buffer,ch);
}
void do_spellinfo(struct char_data *ch, char *argument, int cmd) {
  page_string(ch->desc, spellinfo, 0);
}
void do_news(struct char_data *ch, char *argument, int cmd) {
  page_string(ch->desc, news, 0);
}
void do_info(struct char_data *ch, char *argument, int cmd) {
  page_string(ch->desc, info, 0);
}
void do_where(struct char_data *ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], buf2[256];
  int j,m,n,rm;
  struct obj_data *k;
  struct char_data *i,*p;
  struct descriptor_data *d;

  one_argument(argument, name);
  n=0;
  if (!*name) {
    if (GET_LEVEL(ch) < IMO) {
      send_to_char("What are you looking for?\n\r", ch);
      return;
    } else {
      for (d = descriptor_list; d; d = d->next) {
        if (d->character && (d->connected == CON_PLYNG) &&
             (d->character->in_room != NOWHERE))
         if(CAN_SEE(ch,d->character)) {
          rm=d->character->in_room;
          if (d->original)   /* If switched */
            sprintf(buf, "%-20s - %s [%d] In body of %s\n\r",
              d->original->player.name, world[rm].name,
              world[rm].number, fname(d->character->player.name));
          else
            sprintf(buf, "%-20s - %s [%d]\n\r",
              d->character->player.name, world[rm].name, world[rm].number);
          send_to_char(buf, ch);
        }
      }
      return;
    }
  }
  if(cmd==271){
    for (d = descriptor_list; d; d = d->next) {
      if(!d->connected){
        p=d->original ? d->original : d->character;
        if(isname(name, p->player.name) && CAN_SEE(ch,p)){
          sprintf(buf,"%s is in %s [%d].\n\r",p->player.name,
           world[p->in_room].name,world[p->in_room].number);
          send_to_char(buf,ch);
          return;
        }
      }
    }
    send_to_char("Not here.\n\r",ch);
    return;
  }
  *buf = '\0';
  for (i = character_list; i; i = i->next){
    if((cmd==270)&&(!IS_NPC(i))) continue;  /* mwhere */
    if(isname(name, i->player.name) && CAN_SEE(ch, i) ) {
      if((i->in_room != NOWHERE) && ((GET_LEVEL(ch)>=IMO) ||
          (world[i->in_room].zone == world[ch->in_room].zone))) {
        n++;
        sprintf(buf, "%-30s- %s ",
          IS_NPC(i) ? i->player.short_descr : i->player.name,
          world[i->in_room].name ? world[i->in_room].name : "Somewhere");
        if (GET_LEVEL(ch) >= IMO)
          sprintf(buf2,"[%d]\n\r", world[i->in_room].number);
        else
          strcpy(buf2, "\n\r");
        strcat(buf, buf2);
        send_to_char(buf, ch);
        if (GET_LEVEL(ch) < IMO)
          break;
      }
    }
  }
}
void do_find(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *i,*nx;
  char buf[MAX_STRING_LENGTH];
  int vn,rm;

  if(!*argument)
    vn=1304;
  else{
    vn=atoi(argument);
    if(vn < 0)
      return;
  }
  for(i=character_list;i;i=nx){
    nx=i->next;
    if(IS_MOB(i)){
      if(mob_index[i->nr].virtual==vn){
        rm=world[i->in_room].number;
        sprintf(buf,"%-16s%6d%6d%12d\n\r",
          GET_NAME(i),rm,i->specials.xxx,GET_GOLD(i));
        send_to_char(buf,ch);
      }
    }
  }
}
void do_locate(struct char_data *ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], buf2[256];
  int j,n,rm;
  struct obj_data *k;
  struct char_data *i,*p;

  one_argument(argument, name);
  if (!*name) {
    send_to_char("What are you looking for?\n\r", ch);
    return;
  }
  n=1;
  for (i=character_list;i;i=i->next) if(GET_LEVEL(i) <= GET_LEVEL(ch))
    for(j=0;j<MAX_WEAR;++j) if(i->equipment[j])
      if(isname(name, i->equipment[j]->name)){
        n++;
        sprintf(buf, "%s worn by %s\n\r",
          i->equipment[j]->short_description, GET_NAME(i));
        send_to_char(buf, ch);
      }
  for (k=object_list;k;k=k->next) if(isname(name, k->name)){
    if(k->carried_by) {
      if(GET_LEVEL(ch) >= GET_LEVEL(k->carried_by)){
        sprintf(buf,"%2d: %s carried by %s.\n\r",
          ++n,k->short_description,PERS(k->carried_by,ch));
        send_to_char(buf,ch);
      }
    } else if(k->in_obj) {
      sprintf(buf,"%2d: %s in %s",++n,k->short_description,
        k->in_obj->short_description);
      if(k->in_obj->carried_by){
        if(GET_LEVEL(ch) >= GET_LEVEL(k->in_obj->carried_by)){
          sprintf(buf2," carried by %s\n\r",PERS(k->in_obj->carried_by,ch));
          strcat(buf,buf2);
        }
      } else
        strcat(buf,"\n\r");
      send_to_char(buf,ch);
    } else if(k->in_room != NOWHERE) {
      if(GET_LEVEL(ch) >= IMO)
        sprintf(buf, "%2d: %s in %s [%d]\n\r",++n, k->short_description,
         world[k->in_room].name, world[k->in_room].number);
      else
        sprintf(buf, "%2d: %s in %s.\n\r",++n, k->short_description,
         world[k->in_room].name);
      send_to_char(buf, ch);
    }
  }
  if (!*buf)
    send_to_char("Couldn't find any such thing.\n\r", ch);
}
void do_levels(struct char_data *ch, char *argument, int cmd)
{
  int i,lo,hi;
  char buf[MAX_STRING_LENGTH];

  if (IS_NPC(ch)) {
    send_to_char("You ain't nothin' but a hound-dog.\n\r", ch);
    return;
  }
  lo = 0;
  one_argument(argument,buf);
  if(*buf)
    lo = atoi(buf);
  if(!lo)
    lo=GET_LEVEL(ch);
  hi=lo+10;
  if(lo > IMO) lo=IMO;
  if(hi > IMO+1) hi=IMO+1;
  *buf = '\0';
  for (i = lo; i < hi; i++){
    sprintf(buf+strlen(buf),"%3d: %12d EXP PTS\n\r",i, XP4LEV(i));
  }
  send_to_char(buf, ch);
}

void do_evaluate(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict;
  char name[256],buf[512],buf2[256];
  int diff,hp,ac,hr,dr;

  one_argument(argument, name);
  if(! *name){
    vict=ch->specials.fighting;
  } else {
    vict = get_char_room_vis(ch, name);
    if(!vict)
      vict=ch->specials.fighting;
  }
  if(!vict){
    send_to_char("Evaluate who?\n\r", ch);
    return;
  }
  if(!IS_NPC(vict) && (GET_LEVEL(ch) < IMO)){
    send_to_char("Why would you want to evaluate another player??\n\r",ch);
    return;
  }
  hp=GET_HIT(vict);
  ac=vict->points.armor;
  hr=vict->points.hitroll;
  dr=vict->points.damroll;
  strcpy(name,IS_NPC(vict) ? vict->player.short_descr : GET_NAME(vict));
  CAP(name);
  sprintf(buf,"%s: HP=%d AC=%d XP=%d HR=%d DR=%d MR=%d BHD=%dD%d.\n\r",
   name,hp,ac,GET_EXP(vict),hr,dr,vict->specials.magres,
   vict->specials.damnodice,vict->specials.damsizedice);
  sprintf(buf2,"Str=%d Int=%d Wis=%d Dex=%d Con=%d. Alignment=%d\n\r",
    GET_STR(vict),GET_INT(vict),GET_WIS(vict),GET_DEX(vict),GET_CON(vict),
    GET_ALIGNMENT(vict));
  strcat(buf,buf2);
  send_to_char(buf,ch);
  if(IS_NPC(vict) && (vict->specials.act)){
    strcpy(buf,"Special characteristics: ");
    sprintbit(vict->specials.act & 0xffffa7,action_bits,buf2);
    strcat(buf,buf2);
    strcat(buf,"\n\r");
    send_to_char(buf,ch);
  }
  if(!IS_NPC(vict))
    return;
  diff = (GET_LEVEL(vict)-GET_LEVEL(ch));
  if(diff < 0)
    sprintf(buf,"%s is %d levels below you.\n\r",
      vict->player.short_descr,-diff);
  else if(diff > 0)
    sprintf(buf,"%s is %d levels above you.\n\r",
      vict->player.short_descr,diff);
  else
    sprintf(buf,"%s is your level.\n\r",vict->player.short_descr);
  send_to_char(buf,ch);
  act("$n evaluates $N.",TRUE,ch,0,vict,TO_ROOM);
}

void do_police(struct char_data *ch, char *argument, int cmd)
{
  char name[200];
  struct descriptor_data *d;
  int target;

  one_argument(argument, name);
  if(! *argument)
    return;
  target=atoi(name);
  for (d=descriptor_list;d;d=d->next)
    if(target==d->descriptor)
      close_socket(d);
}
void do_wizlock(struct char_data *ch, char *argument, int cmd)
{
  char buf[200];
  int i,j;

  buf[0]=0;
  one_argument(argument, buf);
  if(*argument){
    j=(-1);
    for(i=0;i<baddoms;++i)
      if(strcmp(baddomain[i],buf)==0){
        j=i; break;
      }
    if(j>=0){
      strcpy(baddomain[j],baddomain[--baddoms]);
    } else {
      if(baddoms < BADDOMS)
        strcpy(baddomain[baddoms++],buf);
    }
  } else {
     for(i=0;i<baddoms;++i){
        sprintf(buf,"%s\n",baddomain[i]);
        send_to_char(buf,ch);
     }
  }
}
void do_newlock(struct char_data *ch, char *argument, int cmd)
{
  char buf[200];
  int i,j;

  buf[0]=0;
  one_argument(argument, buf);
  if(*argument){
    j=(-1);
    for(i=0;i<newbeedoms;++i)
      if(strcmp(newbeedomain[i],buf)==0){
        j=i; break;
      }
    if(j>=0){
      strcpy(newbeedomain[j],newbeedomain[--newbeedoms]);
    } else {
      if(newbeedoms < BADDOMS)
        strcpy(newbeedomain[newbeedoms++],buf);
    }
  } else {
     for(i=0;i<newbeedoms;++i){
        sprintf(buf,"%s\n",newbeedomain[i]);
        send_to_char(buf,ch);
     }
  }
}
do_notell(struct char_data *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return;
  switch(cmd){
    case 292:
    if(IS_SET(ch->specials.act, PLR_REPEAT)) {
      send_to_char("You will no longer see your own SAY's.\n\r", ch);
      REMOVE_BIT(ch->specials.act, PLR_REPEAT);
    } else {
      send_to_char("You will now see your own SAY's.\n\r",ch);
      SET_BIT(ch->specials.act, PLR_REPEAT);
    }
    return;
    case 281:
    if(IS_SET(ch->specials.act, PLR_AUTOCNVRT)) {
      send_to_char("You will no longer convert to META points.\n\r", ch);
      REMOVE_BIT(ch->specials.act, PLR_AUTOCNVRT);
    } else {
      send_to_char("You will automatically convert EXP to META points.\n\r",ch);
      SET_BIT(ch->specials.act, PLR_AUTOCNVRT);
    }
    return;
    case 276:
    if (IS_SET(ch->specials.act, PLR_NORELO)) {
      send_to_char("You can be relocated to again.\n\r", ch);
      REMOVE_BIT(ch->specials.act, PLR_NORELO);
    } else {
      send_to_char("You cannot be relocated to, by mortals.\n\r", ch);
      SET_BIT(ch->specials.act, PLR_NORELO);
    }
    return;
    case 248:
    if (IS_SET(ch->specials.act, PLR_NOSUMMON)) {
      send_to_char("You can be summoned again.\n\r", ch);
      REMOVE_BIT(ch->specials.act, PLR_NOSUMMON);
    } else {
      send_to_char("You cannot be summoned, by mortals.\n\r", ch);
      SET_BIT(ch->specials.act, PLR_NOSUMMON);
    }
    return;
    case 221:
    if (IS_SET(ch->specials.act, PLR_NOTELL)) {
      send_to_char("You can now hear tells again.\n\r", ch);
      REMOVE_BIT(ch->specials.act, PLR_NOTELL);
    } else {
      send_to_char("From now on, you won't hear tells.\n\r", ch);
      SET_BIT(ch->specials.act, PLR_NOTELL);
    }
    return;
    case 286:
    if (IS_SET(ch->specials.act, PLR_AUTOEXIT)) {
      send_to_char("Autoexit is off.\n\r", ch);
      REMOVE_BIT(ch->specials.act, PLR_AUTOEXIT);
    } else {
      send_to_char("Autoexit is on.\n\r", ch);
      SET_BIT(ch->specials.act, PLR_AUTOEXIT);
    }
    return;
    case 287:
    if (IS_SET(ch->specials.act, PLR_AUTOLOOT)) {
      send_to_char("Autoloot is off.\n\r", ch);
      REMOVE_BIT(ch->specials.act, PLR_AUTOLOOT);
    } else {
      send_to_char("Autoloot is on.\n\r", ch);
      SET_BIT(ch->specials.act, PLR_AUTOLOOT);
    }
    return;
  }
}
void do_report(struct char_data *ch, char *argument, int cmd)
{
  char buf[256];
  int t;

  for(;isspace(*argument);++argument);
  if(*argument == 's'){
    sprintf(buf,"reports: I have %d str, %d int, %d wis, %d dex, and %d con.",
      GET_STR(ch),GET_INT(ch),GET_WIS(ch),GET_DEX(ch),GET_CON(ch));
  } else if(*argument == 'a'){
    t=ch->player.time.played+time(0)-ch->player.time.logon;
    sprintf(buf,"reports: I am %d minutes old and have %d XP.",
      t/60,GET_EXP(ch));
  } else {
    sprintf(buf,"reports: I have %d/%d hp, %d/%d mana, %d/%d mp.",
      GET_HIT(ch),GET_MAX_HIT(ch),
      GET_MANA(ch),GET_MAX_MANA(ch),
      GET_MOVE(ch),GET_MAX_MOVE(ch));
    if(*argument == 'x') sprintf(buf+strlen(buf),"  My xp is %u.",
      GET_EXP(ch));
  }
  do_emote(ch,buf,0);
}
void do_command_list(struct char_data *ch, char *arg, int cmd)
{
  extern struct command_info cmd_info[];
  extern char *command[];
  void do_action(struct char_data *ch, char *argument, int cmd);
  int i,j,lev;
  unsigned char f1,f2;
  char buf[MAX_STRING_LENGTH],buf2[80];

  f1=(cmd==256);
  lev=GET_LEVEL(ch);
  buf[0]=0;
  for(i=1,j=0;i<265;++i){
    if(!cmd_info[i].command_pointer) continue;
    if(cmd_info[i].minimum_level > lev) continue;
    f2=(cmd_info[i].command_pointer == do_action);
    if(f1==f2){
      sprintf(buf2,"%-12s",command[i-1]);
      strcat(buf,buf2);
      ++j;
      if(j==6){
        strcat(buf,"\n\r");
        j=0;
      }
    }
  }
  if(j) strcat(buf,"\n\r");
  send_to_char(buf,ch);
}
void do_localtime(struct char_data *ch, char *arg, int cmd)
{
  int t;
  char buf[256];

  t=time(0);
  sprintf(buf,"%s\r",(char *)ctime((time_t *)&t));
  send_to_char(buf,ch);
}
void do_identify(struct char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  extern struct weapon_spell_list wsplist[];
  extern char *shield_fx_name[];
  extern char *eyewear_fx_name[];

  extern char *item_types[];
  extern char *wear_bits[];
  extern char *extra_bits[];
  extern char *drinks[];

  extern char *equipment_types[];
  extern char *affected_bits[];
  extern char *apply_types[];
  extern char *player_bits[];
  extern char *position_types[];
  extern char *connected_types[];
  struct char_data *get_specific_vis(struct char_data *ch,char *name,int type);
  int i,pc,yw,sh,lev;
  bool found;
  struct char_data *victim;
  struct obj_data *obj;
  struct time_info_data age(struct char_data *ch);

  /* Spell Names */
  extern char *spells[];

  /* For Objects */
  extern char *item_types[];
  extern char *extra_bits[];
  extern char *apply_types[];
  extern char *affected_bits[];

  argument = one_argument(argument, arg1);
  if (!*arg1) {
    send_to_char("Identify what?\n\r",ch);
    return;
  }
  if(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)) {
    send_to_char("You feel informed:\n\r", ch);
    sprintf(buf, "Object '%s', Item type: ", obj->name);
    sprinttype(GET_ITEM_TYPE(obj),item_types,buf2);
    strcat(buf,buf2); strcat(buf,"\n\r");
    send_to_char(buf, ch);
    sprintf(buf,"Wgt: %d, Value: %d, RentLevel: %d, Timer: %d, ID: %d\n\r",
      obj->obj_flags.weight,obj->obj_flags.cost,obj->obj_flags.rentlevel,
      obj->obj_flags.timer,obj->oid);
    send_to_char(buf, ch);
    if (obj->obj_flags.bitvector) {
      send_to_char("Item will give you following abilities:  ", ch);
      sprintbit(obj->obj_flags.bitvector,affected_bits,buf);
      strcat(buf,"\n\r");
      send_to_char(buf, ch);
    }
    send_to_char("Item is: ", ch);
    sprintbit(obj->obj_flags.extra_flags,extra_bits,buf);
    strcat(buf,"\n\r");
    send_to_char(buf,ch);
    switch (GET_ITEM_TYPE(obj)) {
      case ITEM_SCROLL : 
      case ITEM_POTION :
        sprintf(buf, "Level %d spells of:\n\r",  obj->obj_flags.value[0]);
        send_to_char(buf, ch);
        if (obj->obj_flags.value[1] >= 1) {
          sprinttype(obj->obj_flags.value[1]-1,spells,buf);
          strcat(buf,"\n\r");
          send_to_char(buf, ch);
        }
        if (obj->obj_flags.value[2] >= 1) {
          sprinttype(obj->obj_flags.value[2]-1,spells,buf);
          strcat(buf,"\n\r");
          send_to_char(buf, ch);
        }
        if (obj->obj_flags.value[3] >= 1) {
          sprinttype(obj->obj_flags.value[3]-1,spells,buf);
          strcat(buf,"\n\r");
          send_to_char(buf, ch);
        }
        break;
      case ITEM_WAND : 
      case ITEM_STAFF : 
        sprintf(buf, "Has %d charges, with %d charges left.\n\r",
          obj->obj_flags.value[1],
          obj->obj_flags.value[2]);
        send_to_char(buf, ch);
        sprintf(buf, "Level %d spell of:\n\r",  obj->obj_flags.value[0]);
        send_to_char(buf, ch);
        if (obj->obj_flags.value[3] >= 1) {
          sprinttype(obj->obj_flags.value[3]-1,spells,buf);
          strcat(buf,"\n\r");
          send_to_char(buf, ch);
        }
        break;
      case ITEM_WEAPON :
        if(obj->obj_flags.wear_flags & ITEM_HOLD)
          send_to_char("This weapon can be held.\n\r",ch);
      case ITEM_FIREWEAPON :
        sprintf(buf, "Damage Dice is '%dD%d'\n\r",
          obj->obj_flags.value[1],
           obj->obj_flags.value[2]);
        send_to_char(buf, ch);
        if(IS_SET(obj->obj_flags.extra_flags,ITEM_SFX)){
          i=obj->obj_flags.value[0];
          if((i>=0)&&(i<=MAX_WEAPON_SPELL) && wsplist[i].spellfun){
            yw=GET_WIS(ch);
            sh=wsplist[i].shift;
            lev=(sh) ? wsplist[i].lev+(yw>>sh) : wsplist[i].lev;
            pc=((yw-wsplist[i].lo)*100)/(wsplist[i].hi-wsplist[i].lo);
            if(pc < 0) pc=0;
            else if(pc > 100) pc=100;
            sprintf(buf,"Weapon spell: %s fires %d%% at level %d.\n\r",
              wsplist[i].name,pc,lev);
            send_to_char(buf,ch);
          }
        }
        break;
      case ITEM_ARMOR :
        sprintf(buf, "AC-apply is %d\n\r", obj->obj_flags.value[0]);
        if(obj->obj_flags.wear_flags & ITEM_WEAR_SHIELD){
          i=obj->obj_flags.value[1];
          if((i<1)||(i>8)) i=0;
          if(i)
            sprintf(buf+strlen(buf),"Protects against %s.\n\r",
              shield_fx_name[i]);
        } else if(obj->obj_flags.wear_flags & ITEM_WEAR_FACE){
          i=obj->obj_flags.value[2];
          if((i<1)||(i>8)) i=0;
          if(i)
            sprintf(buf+strlen(buf),"Special vision effects: %s.\n\r",
              eyewear_fx_name[i]);
        }
        send_to_char(buf, ch);
        break;
      case ITEM_RADIO :
        sprintf(buf, "Sends on %d, receives on %d.\n\r",
          obj->obj_flags.value[1],obj->obj_flags.value[2]);
        send_to_char(buf, ch);
        break;
      case ITEM_SHOVEL :
        sprintf(buf, "Can be used %d more time(s).\n\r",
          obj->obj_flags.value[3]);
        send_to_char(buf, ch);
        break;
      case ITEM_PILL:
        sprintf(buf, "Pill type %d, strength %d.\n\r",
          obj->obj_flags.value[0],1+obj->obj_flags.value[1]);
        send_to_char(buf, ch);
        break;
      case ITEM_TOKEN:
        sprintf(buf, "Worth %d mu's.\n\r",
          obj->obj_flags.value[0]);
        send_to_char(buf, ch);
        break;
    }
    found = FALSE;
    for (i=0;i<MAX_OBJ_AFFECT;i++) {
      if ((obj->affected[i].location != APPLY_NONE) &&
         (obj->affected[i].modifier != 0)) {
        if (!found) {
          send_to_char("Can affect you as :\n\r", ch);
          found = TRUE;
        }
        sprinttype(obj->affected[i].location,apply_types,buf2);
        sprintf(buf,"   Affects: %s by %d\n\r",buf2,obj->affected[i].modifier);
        send_to_char(buf, ch);
      }
    }
  } else {
    send_to_char("Use EVALUATE or CONSIDER on monsters.\n\r",ch);
  }
}
void do_scan(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  int i,newroom;
  static char *dirnames[]= { 
    "North", "East", "South", "West", "Up", "Down"
  };

  if(GET_POS(ch) < POSITION_SLEEPING){
    send_to_char("You can't see anything but stars!\n\r",ch);
    return;
  }
  if(GET_POS(ch) == POSITION_SLEEPING){
    send_to_char("You must be dreaming.\n\r",ch);
    return;
  }
  if(IS_AFFECTED(ch, AFF_BLIND)){
    send_to_char("You can't see a thing, you're blind!\n\r", ch);
    return;
  }
  for(i=0;i<6;i++){
    if(EXIT(ch, i)) {
      if(IS_DARK(ch->in_room) && (!OMNI(ch)) && (!CANINFRA(ch))){
        sprintf(buf,"%s: DARK\n\r",dirnames[i]);
        send_to_char(buf,ch);
        continue;
      }
      if(CAN_GO(ch,i)){
        newroom=world[ch->in_room].dir_option[i]->to_room;
        sprintf(buf,"=-= %s: %s =-=\n\r",
          dirnames[i],
          world[newroom].name ? world[newroom].name : "Bite my ass!");
        send_to_char(buf,ch);
        list_obj_to_char(world[newroom].contents,ch,0,FALSE);
        list_char_to_char(world[newroom].people,ch,0);
      }
    }
  }
}
void do_reset(struct char_data *ch, char *argument, int cmd)
{
  int i;
  char buf[MAX_STRING_LENGTH];

  i = world[ch->in_room].zone;
  if((i<0)||(i>=top_of_zone_table))
    return;
  sprintf(buf,"This zone (%s) will reset in %d ticks.\n\r",
      zone_table[i].name,
      zone_table[i].lifespan-zone_table[i].age);
  send_to_char(buf,ch);
  if(GET_LEVEL(ch) >= IMO){
    sprintf(buf,"The current time between resets is %d ticks.\n\r",
        zone_table[i].lifespan);
    send_to_char(buf,ch);
    one_argument(argument,buf);
    if(*buf == '*'){
      reset_zone(i);
    }
  }
}
void do_areas(struct char_data *ch, char *argument, int cmd)
{
  int i;
  char buf[MAX_STRING_LENGTH];

  for(i=1;i<top_of_zone_table;i+=2){
    sprintf(buf,"%-20s%5d%5d | %-20s%5d%5d\n\r",
      zone_table[i].name,zone_table[i].age,zone_table[i].lifespan,
      zone_table[i+1].name,zone_table[i+1].age,zone_table[i+1].lifespan);
    send_to_char(buf,ch);
  }
}

void do_finger(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *get_player_vis(struct char_data *ch, char *name);
  char buf[MAX_STRING_LENGTH];
  struct char_data *k;
  char name[KBYTE];
  time_t last_logon;
  struct char_file_u cfu;

  one_argument(argument, name);
  if(!*name){
    send_to_char("Finger whom?\n\r",ch);
    return;
  }
  k = get_player_vis(ch, name);
  if (k) {
    send_to_char("On line now.\n\r",ch);
  } else {
    if(load_char(name,&cfu) >= 0){
      sprintf(buf, "Name: %s\n\rLevel: %d\n\rLast Logon: %s\r",
        cfu.name,
        cfu.level,
        (char *) ctime((time_t *) & cfu.last_logon));
      send_to_char(buf,ch);
    } else {
      send_to_char("Who dat?\n\r",ch);
    }
  }
}
