
#define NEWWEAR

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
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
extern char *drinks[];
extern int drink_aff[][3];

/* extern functions */

void wear_replace(struct char_data *ch,
             struct obj_data *old_object,
             struct obj_data *new_object, int keyword);
void wear(struct char_data *ch, struct obj_data *obj_object, int keyword);
void update_pos( struct char_data *victim );
struct obj_data *get_object_in_equip_vis(struct char_data *ch,
                         char *arg, struct obj_data **equipment, int *j);
#ifdef NEEDS_STRDUP
char *strdup(char *source);
#endif

int wflag;

void weight_change_object(struct obj_data *obj, int weight)
{
  struct obj_data *tmp_obj;
  struct char_data *tmp_ch;

  if (obj->in_room != NOWHERE) {
    GET_OBJ_WEIGHT(obj) += weight;
  } else if (tmp_ch = obj->carried_by) {
    obj_from_char(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_char(obj, tmp_ch);
  } else if (tmp_obj = obj->in_obj) {
    obj_from_obj(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_obj(obj, tmp_obj);
  } else {
    log("Unknown attempt to subtract weight from an object.");
  }
}



void name_from_drinkcon(struct obj_data *obj)
{
  int i;
   char *new_name;

  for(i=0; (*((obj->name)+i)!=' ') && (*((obj->name)+i)!='\0'); i++)  ;
  if (*((obj->name)+i)==' ') {
    new_name=strdup((obj->name)+i+1);
    free(obj->name);
    obj->name=new_name;
  }
}



void name_to_drinkcon(struct obj_data *obj,int type)
{
  char *new_name;
  extern char *drinknames[];

  CREATE(new_name,char,strlen(obj->name)+strlen(drinknames[type])+2);
  sprintf(new_name,"%s %s",drinknames[type],obj->name);
  free(obj->name);
  obj->name=new_name;
}



void do_drink(struct char_data *ch, char *argument, int cmd)
{
  char buf[KBYTE];
  struct obj_data *temp;
  struct affected_type af;
  int amount,drunkval,i;

  one_argument(argument,buf);
  temp = get_obj_in_list_vis(ch,buf,ch->carrying);
  if(!temp) {
    act("You can't find it!",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  if (temp->obj_flags.type_flag!=ITEM_DRINKCON) {
    act("You can't drink from that!",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  drunkval = drink_aff[temp->obj_flags.value[2]][DRUNK];
  if((GET_COND(ch,DRUNK) > 50)&&(GET_COND(ch,THIRST)!=0)&&(drunkval > 0)){
    act("You lose consciousness.",FALSE, ch, 0, 0, TO_CHAR);
    act("$n passes out..",TRUE, ch, 0, 0, TO_ROOM);
    do_sleep(ch,0,0);
    return;
  }
  if((GET_COND(ch,FULL)>(GET_GUT(ch)-10))&&(GET_COND(ch,THIRST)>0)) {
    act("Your stomach can't contain anymore!",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  if (temp->obj_flags.type_flag==ITEM_DRINKCON){
    if(temp->obj_flags.value[1]>0) {  
      sprintf(buf,"$n drinks %s from $p",drinks[temp->obj_flags.value[2]]);
      act(buf, TRUE, ch, temp, 0, TO_ROOM);
      sprintf(buf,"You drink the %s.\n\r",drinks[temp->obj_flags.value[2]]);
      send_to_char(buf,ch);
      amount = GET_GUT(ch)-GET_COND(ch,THIRST);
      amount = MIN(amount,temp->obj_flags.value[1]);
      weight_change_object(temp, -amount);  /* Subtract amount */
      gain_condition(ch,DRUNK,(drunkval*amount)/4);
      gain_condition(ch,FULL,
        (drink_aff[temp->obj_flags.value[2]][FULL]*amount));
      gain_condition(ch,THIRST,
        (drink_aff[temp->obj_flags.value[2]][THIRST]*amount));
      if(GET_COND(ch,DRUNK) > (GET_GUT(ch)/5))
        act("You feel drunk.",FALSE,ch,0,0,TO_CHAR);
      if(GET_COND(ch,THIRST) > (GET_GUT(ch)/2))
        act("You do not feel thirsty.",FALSE,ch,0,0,TO_CHAR);
      if(GET_COND(ch,FULL) > ((95*GET_GUT(ch))/100))
        act("You are full.",FALSE,ch,0,0,TO_CHAR);
      if(temp->obj_flags.value[3]) {
        act("Oops, it tasted rather strange ?!!?",FALSE,ch,0,0,TO_CHAR);
        act("$n chokes and utters some strange sounds.",
           TRUE,ch,0,0,TO_ROOM);
        af.type = SPELL_POISON;
        af.duration = amount*3;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = AFF_POISON;
        affect_join(ch,&af, FALSE, FALSE);
      }

      /* empty the container, and no longer poison. */
      temp->obj_flags.value[1]-= amount;
      if(!temp->obj_flags.value[1]) {  /* The last bit */
        temp->obj_flags.value[2]=0;
        temp->obj_flags.value[3]=0;
        name_from_drinkcon(temp);
      }
      return;
    }
  }
  act("It's empty already.",FALSE,ch,0,0,TO_CHAR);
  return;
}

void do_eat(struct char_data *ch, char *argument, int cmd)
{
  char buf[256];
  struct obj_data *temp;
  struct affected_type *afp;
  struct affected_type af;
  struct affected_type *hjp;
  void spell_super_heal(short level, struct char_data *ch,
     struct char_data *victim, struct obj_data *obj);
  int t;

  one_argument(argument,buf);
  if(!(temp = get_obj_in_list_vis(ch,buf,ch->carrying))) {
    act("You can't find it!",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  if(temp->obj_flags.type_flag == ITEM_TOKEN){
    GET_META(ch) += temp->obj_flags.value[0];
    act("$n eats $p.",TRUE,ch,temp,0,TO_ROOM);
    act("You swallow the $o.",FALSE,ch,temp,0,TO_CHAR);
    extract_obj(temp);
    return;
  }
  if(temp->obj_flags.type_flag == ITEM_PILL){
    act("$n swallows $p.",TRUE,ch,temp,0,TO_ROOM);
    act("You swallow the $o.",FALSE,ch,temp,0,TO_CHAR);
    extract_obj(temp);
    switch(temp->obj_flags.value[0]){
     case 0:
      spell_super_heal(0,ch,ch,0);
      if(number(1,20) < 7) spell_greenman(0,ch,ch,0);
      break;
     case 1:
      spell_moveheal(0,ch,ch,0);
      if(number(1,20) < 7) spell_greenman(0,ch,ch,0);
      break;
     case 2:
      spell_manaheal(t=temp->obj_flags.value[1],ch,ch,0);
      if(t==0){
        if(number(1,20) < 7) spell_greenman(0,ch,ch,0);
      }
      break;
     case 3:
      do_meta(ch,0,number(1,8));
      spell_greenman(0,ch,ch,0);
      send_to_char("You think you see the Metaphysician.\n\r",ch);
      break;
     case 4:
      SET_BIT(ch->specials.act,PLR_TITLE);
      send_to_char("Title away...\n\r",ch);
      break;
     case 5:
      (ch->specials.spells_to_learn)++;
      send_to_char("Gulp.\n\r",ch);
      break;
     case 6:
      spell_manaheal(t=temp->obj_flags.value[1],ch,ch,0);
      ch->skills[number(1,82)].learned = 100;
      send_to_char("You feel brilliant!\n\r",ch);
      break;
     case 7:
      spell_moveheal(0,ch,ch,0);
      SET_BIT(ch->specials.act,PLR_ECHO);
      send_to_char("Echo .. echo .. echo ..\n\r",ch);
      break;
     case 8:
      if(IS_NPC(ch)) return;
      send_to_char("You are normal again.\n\r", ch);
      for(hjp=ch->affected;hjp;hjp=hjp->next)
        affect_remove(ch,hjp);
      break;
     case 9:
      if(IS_NPC(ch)) return;
      send_to_char("You could have danced all night!!\n\r",ch);
      ch->points.max_hit++ ;
      ch->points.max_mana++;
      ch->points.max_move++;
      break;
     case 10:
      if(IS_NPC(ch)) return;
      send_to_char("Gulp!\n\r",ch);
      gain_condition(ch,DRUNK,-GET_COND(ch,DRUNK));
      break;
     case 11:
      if(!IS_NPC(ch)){
        GET_HIT(ch)<<=1;
        send_to_char("It tastes like dog food!\n\r",ch);
      }
      break;
     case 12:
      if(!IS_NPC(ch)){
        GET_MANA(ch)<<=1;
        send_to_char("It still tastes like dog food!\n\r",ch);
      }
      break;
     case 13:
      if(!IS_NPC(ch)){
        GET_MOVE(ch)<<=1;
        send_to_char("It does taste like dog food!\n\r",ch);
      }
      break;
     case 14:
      for(afp = ch->affected; afp; afp = afp->next)
         afp->duration <<= 1;
      send_to_char("You feel invigorated!\n\r",ch);
      break;
    }
    return;
  }
  if((temp->obj_flags.type_flag != ITEM_FOOD) && (GET_LEVEL(ch) < (IMO+1))) {
    act("Your stomach refuses to eat that!?!",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  if(GET_COND(ch,FULL) > ((95*GET_GUT(ch))/100)){
    act("You are too full to eat more!",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  act("$n eats $p",TRUE,ch,temp,0,TO_ROOM);
  act("You eat the $o.",FALSE,ch,temp,0,TO_CHAR);
  gain_condition(ch,FULL,temp->obj_flags.value[0]);
  if(GET_COND(ch,FULL)> GET_GUT(ch)-10)
    act("You are full.",FALSE,ch,0,0,TO_CHAR);
  if(temp->obj_flags.value[3] && (GET_LEVEL(ch)<IMO)) {
    act("Ooops, it tasted rather strange ?!!?",FALSE,ch,0,0,TO_CHAR);
    act("$n coughs and utters some strange sounds.",FALSE,ch,0,0,TO_ROOM);
    af.type = SPELL_POISON;
    af.duration = temp->obj_flags.value[0]*2;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_POISON;
    affect_join(ch,&af, FALSE, FALSE);
  }
  extract_obj(temp);
}
void do_junk(struct char_data *ch, char *argument, int cmd)
{
  char buf[100];
  struct obj_data *temp;

  one_argument(argument,buf);

  if(!(temp = get_obj_in_list_vis(ch,buf,ch->carrying))) {
    act("You can't find it!",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  if (IS_SET(temp->obj_flags.extra_flags, ITEM_NODROP)) {
    act("You can't junk it! Cursed?",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  act("$n junks $p.",TRUE,ch,temp,0,TO_ROOM);
  act("You junk the $o.",FALSE,ch,temp,0,TO_CHAR);
  extract_obj(temp);
}
void do_pour(struct char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  struct obj_data *from_obj;
  struct obj_data *to_obj;
  int amount;

  argument_interpreter(argument, arg1, arg2);

  if(!*arg1) /* No arguments */
  {
    act("What do you want to pour from?",FALSE,ch,0,0,TO_CHAR);
    return;
  }

  if(!(from_obj = get_obj_in_list_vis(ch,arg1,ch->carrying)))
  {
    act("You can't find it!",FALSE,ch,0,0,TO_CHAR);
    return;
  }

  if(from_obj->obj_flags.type_flag!=ITEM_DRINKCON)
  {
    act("You can't pour from that!",FALSE,ch,0,0,TO_CHAR);
    return;
  }

  if(from_obj->obj_flags.value[1]==0)
  {
    act("The $p is empty.",FALSE, ch,from_obj, 0,TO_CHAR);
    return;
  }

  if(!*arg2)
  {
    act("Where do you want it? Out or in what?",FALSE,ch,0,0,TO_CHAR);
    return;
  }

  if(!str_cmp(arg2,"out"))
  {
    act("$n empties $p", TRUE, ch,from_obj,0,TO_ROOM);
    act("You empty the $p.", FALSE, ch,from_obj,0,TO_CHAR);

    weight_change_object(from_obj, -from_obj->obj_flags.value[1]); /* Empty */

    from_obj->obj_flags.value[1]=0;
    from_obj->obj_flags.value[2]=0;
    from_obj->obj_flags.value[3]=0;
    name_from_drinkcon(from_obj);
    
    return;

  }

  if(!(to_obj = get_obj_in_list_vis(ch,arg2,ch->carrying)))
  {
    act("You can't find it!",FALSE,ch,0,0,TO_CHAR);
    return;
  }

  if(to_obj->obj_flags.type_flag!=ITEM_DRINKCON)
  {
    act("You can't pour anything into that.",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  if((to_obj->obj_flags.value[1]!=0)&&
    (to_obj->obj_flags.value[2]!=from_obj->obj_flags.value[2]))
  {
    act("There is already another liquid in it!",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  if(!(to_obj->obj_flags.value[1]<to_obj->obj_flags.value[0]))
  {
    act("There is no room for more.",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  if(from_obj == to_obj)
  {
    act("That would be silly.",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  sprintf(buf,"You pour the %s into the %s.",
    drinks[from_obj->obj_flags.value[2]],arg2);
  send_to_char(buf,ch);

  /* New alias */
  if (to_obj->obj_flags.value[1]==0) 
    name_to_drinkcon(to_obj,from_obj->obj_flags.value[2]);

  /* First same type liq. */
  to_obj->obj_flags.value[2]=from_obj->obj_flags.value[2];

  /* Then how much to pour */
  from_obj->obj_flags.value[1]-= (amount=
    (to_obj->obj_flags.value[0]-to_obj->obj_flags.value[1]));

  to_obj->obj_flags.value[1]=to_obj->obj_flags.value[0];

  if(from_obj->obj_flags.value[1]<0)    /* There was to little */
  {
    to_obj->obj_flags.value[1]+=from_obj->obj_flags.value[1];
    amount += from_obj->obj_flags.value[1];
    from_obj->obj_flags.value[1]=0;
    from_obj->obj_flags.value[2]=0;
    from_obj->obj_flags.value[3]=0;
    name_from_drinkcon(from_obj);
  }

  /* Then the poison boogie */
  to_obj->obj_flags.value[3]=
    (to_obj->obj_flags.value[3]||from_obj->obj_flags.value[3]);

  /* And the weight boogie */

  weight_change_object(from_obj, -amount);
  weight_change_object(to_obj, amount);   /* Add weight */

  return;
}

void do_sip(struct char_data *ch, char *argument, int cmd)
{
  struct affected_type af;
  char arg[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  struct obj_data *temp;

  one_argument(argument,arg);

  if(!(temp = get_obj_in_list_vis(ch,arg,ch->carrying)))
  {
    act("You can't find it!",FALSE,ch,0,0,TO_CHAR);
    return;
  }

  if(temp->obj_flags.type_flag!=ITEM_DRINKCON)
  {
    act("You can't sip from that!",FALSE,ch,0,0,TO_CHAR);
    return;
  }

  if(GET_COND(ch,DRUNK)>10) /* The pig is drunk ! */
  {
    act("You simply fail to reach your mouth!",FALSE,ch,0,0,TO_CHAR);
    act("$n tries to sip, but fails!",TRUE,ch,0,0,TO_ROOM);
    return;
  }

  if(!temp->obj_flags.value[1])  /* Empty */
  {
    act("But there is nothing in it?",FALSE,ch,0,0,TO_CHAR);
    return;
  }

  act("$n sips from the $o",TRUE,ch,temp,0,TO_ROOM);
  sprintf(buf,"It tastes like %s.\n\r",drinks[temp->obj_flags.value[2]]);
  send_to_char(buf,ch);

  gain_condition(ch,DRUNK,(int)(drink_aff[temp->obj_flags.value[2]][DRUNK]/4));

  gain_condition(ch,FULL,(int)(drink_aff[temp->obj_flags.value[2]][FULL]/4));

  gain_condition(ch,THIRST,(int)(drink_aff[temp->obj_flags.value[2]][THIRST]/4));

  weight_change_object(temp, -1);  /* Subtract one unit */

  if(GET_COND(ch,DRUNK)>10)
    act("You feel drunk.",FALSE,ch,0,0,TO_CHAR);

  if(GET_COND(ch,THIRST)>20)
    act("You do not feel thirsty.",FALSE,ch,0,0,TO_CHAR);

  if(GET_COND(ch,FULL)>20)
    act("You are full.",FALSE,ch,0,0,TO_CHAR);

  if(temp->obj_flags.value[3]&&!IS_AFFECTED(ch,AFF_POISON)) /* The shit was poisoned ! */
  {
    act("But it also had a strange taste!",FALSE,ch,0,0,TO_CHAR);

    af.type = SPELL_POISON;
    af.duration = 3;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_POISON;
    affect_to_char(ch,&af);
  }

  temp->obj_flags.value[1]--;

  if(!temp->obj_flags.value[1])  /* The last bit */
  {
    temp->obj_flags.value[2]=0;
    temp->obj_flags.value[3]=0;
    name_from_drinkcon(temp);
  }

  return;

}


void do_taste(struct char_data *ch, char *argument, int cmd)
{
  struct affected_type af;
  char arg[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  struct obj_data *temp;

  one_argument(argument,arg);

  if(!(temp = get_obj_in_list_vis(ch,arg,ch->carrying)))
  {
    act("You can't find it!",FALSE,ch,0,0,TO_CHAR);
    return;
  }

  if(temp->obj_flags.type_flag==ITEM_DRINKCON)
  {
    do_sip(ch,argument,0);
    return;
  }

  if(!(temp->obj_flags.type_flag==ITEM_FOOD))
  {
    act("Taste that?!? Your stomach refuses!",FALSE,ch,0,0,TO_CHAR);
    return;
  }

  act("$n tastes the $o", FALSE, ch, temp, 0, TO_ROOM);
  act("You taste the $o", FALSE, ch, temp, 0, TO_CHAR);

  gain_condition(ch,FULL,1);

  if(GET_COND(ch,FULL)>20)
    act("You are full.",FALSE,ch,0,0,TO_CHAR);

  if(temp->obj_flags.value[3]&&!IS_AFFECTED(ch,AFF_POISON)) /* The shit was poisoned ! */
  {
    act("Ooups, it did not taste good at all!",FALSE,ch,0,0,TO_CHAR);

    af.type = SPELL_POISON;
    af.duration = 2;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_POISON;
    affect_to_char(ch,&af);
  }

  temp->obj_flags.value[0]--;

  if(!temp->obj_flags.value[0])  /* Nothing left */
  {
    act("There is nothing left now.",FALSE,ch,0,0,TO_CHAR);
    extract_obj(temp);
  }

  return;

}



/* functions related to wear */

perform_wear(struct char_data *ch, struct obj_data *obj_object, int keyword)
{
  char buffer[MAX_STRING_LENGTH];
  struct char_data *i;

  switch(keyword) {
    case 0 :
      act("$n light $p and holds it.", FALSE, ch, obj_object,0,TO_ROOM);
      break;
    case 1 : 
      act("$n wears $p on $s finger.", TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 2 : 
      act("$n wears $p around $s neck.", TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 3 : 
      act("$n wears $p on $s body.", TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 4 : 
      act("$n wears $p on $s head.",TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 5 : 
      act("$n wears $p on $s legs.",TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 6 : 
      act("$n wears $p on $s feet.",TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 7 : 
      act("$n wears $p on $s hands.",TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 8 : 
      act("$n wears $p on $s arms.",TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 9 : 
      act("$n wears $p about $s body.",TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 10 : 
      act("$n wears $p about $s waist.",TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 11 : 
      act("$n wears $p around $s wrist.",TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 12 : 
      act("$n wields $p.",TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 13 : 
      act("$n grabs $p.",TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 14 : 
      act("$n starts using $p as a shield.", TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 15 : 
      act("$n starts using $p as a radio.", TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 16 : 
      act("$n wears $p on $s face.",TRUE, ch, obj_object,0,TO_ROOM);
      break;
    case 17 : 
      act("$n wears $p on $s ears.",TRUE, ch, obj_object,0,TO_ROOM);
      break;
  }
}


#ifndef NEWWEAR
void wear(struct char_data *ch, struct obj_data *obj_object, int keyword)
{
  char buffer[MAX_STRING_LENGTH];

#ifdef USE_PRIME
  if(IS_SET(obj_object->obj_flags.extra_flags,ITEM_PRIME) &&
     (GET_LEVEL(ch) < (IMO>>3))){
    sprintf(buffer,"You are too lowly to use %s.\n\r",
      obj_object->short_description);
    send_to_char(buffer,ch);
    return;
  } else if(IS_SET(obj_object->obj_flags.extra_flags,ITEM_ANTIPRIME) &&
     (GET_LEVEL(ch) > (IMO>>1))){
    sprintf(buffer,"%s won't fit, it's too small.\n\r",
      obj_object->short_description);
    send_to_char(buffer,ch);
    return;
  }
#endif
  switch(keyword) {
    case 0: {  /* LIGHT SOURCE */
      if (ch->equipment[WEAR_LIGHT])
        send_to_char("You are already holding a light source.\n\r", ch);
      else {
        send_to_char("Ok.\n\r", ch);
        perform_wear(ch,obj_object,keyword);
        obj_from_char(obj_object);
        equip_char(ch,obj_object, WEAR_LIGHT);
        if (obj_object->obj_flags.value[2])
          world[ch->in_room].light++;
      }
    } break;

    case 1: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_FINGER)) {
        if ((ch->equipment[WEAR_FINGER_L]) && (ch->equipment[WEAR_FINGER_R])) {
          send_to_char(
            "You are already wearing something on your fingers.\n\r", ch);
        } else {
          perform_wear(ch,obj_object,keyword);
          if (ch->equipment[WEAR_FINGER_L]) {
            sprintf(buffer, "You put the %s on your right finger.\n\r", 
              fname(obj_object->name));
            send_to_char(buffer, ch);
            obj_from_char(obj_object);
            equip_char(ch, obj_object, WEAR_FINGER_R);
          } else {
            sprintf(buffer, "You put the %s on your left finger.\n\r", 
              fname(obj_object->name));
            send_to_char(buffer, ch);
            obj_from_char(obj_object);
            equip_char(ch, obj_object, WEAR_FINGER_L);
          }
        }
      } else {
        send_to_char("You can't wear that on your finger.\n\r", ch);
      }
    } break;
    case 2: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_NECK)) {
        if ((ch->equipment[WEAR_NECK_1]) && (ch->equipment[WEAR_NECK_2])) {
          send_to_char("You can't wear any more around your neck.\n\r", ch);
        } else {
          send_to_char("OK.\n\r", ch);
          perform_wear(ch,obj_object,keyword);
          if (ch->equipment[WEAR_NECK_1]) {
            obj_from_char(obj_object);
            equip_char(ch, obj_object, WEAR_NECK_2);
          } else {
            obj_from_char(obj_object);
            equip_char(ch, obj_object, WEAR_NECK_1);
          }
        }
      } else {
              send_to_char("You can't wear that around your neck.\n\r", ch);
      }
    } break;
    case 3: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_BODY)) {
        if (ch->equipment[WEAR_BODY]) {
          send_to_char("You already wear something on your body.\n\r", ch);
        } else {
          send_to_char("OK.\n\r", ch);
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch,  obj_object, WEAR_BODY);
        }
      } else {
        send_to_char("You can't wear that on your body.\n\r", ch);
      }
    } break;
    case 4: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_HEAD)) {
        if (ch->equipment[WEAR_HEAD]) {
          send_to_char("You already wear something on your head.\n\r", ch);
        } else {
          send_to_char("OK.\n\r", ch);
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_HEAD);
        }
      } else {
        send_to_char("You can't wear that on your head.\n\r", ch);
      }
    } break;
    case 5: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_LEGS)) {
        if (ch->equipment[WEAR_LEGS]) {
          send_to_char("You already wear something on your legs.\n\r", ch);
        } else {
          send_to_char("OK.\n\r", ch);
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_LEGS);
        }
      } else {
        send_to_char("You can't wear that on your legs.\n\r", ch);
      }
    } break;
    case 6: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_FEET)) {
        if (ch->equipment[WEAR_FEET]) {
          send_to_char("You already wear something on your feet.\n\r", ch);
        } else {
          send_to_char("OK.\n\r", ch);
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_FEET);
        }
      } else {
        send_to_char("You can't wear that on your feet.\n\r", ch);
      }
    } break;
    case 7: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_HANDS)) {
        if (ch->equipment[WEAR_HANDS]) {
          send_to_char("You already wear something on your hands.\n\r", ch);
        } else {
          send_to_char("OK.\n\r", ch);
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_HANDS);
        }
      } else {
        send_to_char("You can't wear that on your hands.\n\r", ch);
      }
    } break;
    case 8: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_ARMS)) {
        if (ch->equipment[WEAR_ARMS]) {
          send_to_char("You already wear something on your arms.\n\r", ch);
        } else {
          send_to_char("OK.\n\r", ch);
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_ARMS);
        }
      } else {
        send_to_char("You can't wear that on your arms.\n\r", ch);
      }
    } break;
    case 9: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_ABOUT)) {
        if (ch->equipment[WEAR_ABOUT]) {
          send_to_char("You already wear something about your body.\n\r", ch);
        } else {
          send_to_char("OK.\n\r", ch);
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_ABOUT);
        }
      } else {
        send_to_char("You can't wear that about your body.\n\r", ch);
      }
    } break;
    case 10: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_WAISTE)) {
        if (ch->equipment[WEAR_WAISTE]) {
          send_to_char("You already wear something about your waiste.\n\r",
            ch);
        } else {
          send_to_char("OK.\n\r", ch);
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch,  obj_object, WEAR_WAISTE);
        }
      } else {
        send_to_char("You can't wear that about your waist.\n\r", ch);
      }
    } break;
    case 11: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_WRIST)) {
        if ((ch->equipment[WEAR_WRIST_L]) && (ch->equipment[WEAR_WRIST_R])) {
          send_to_char(
            "You already wear something around both your wrists.\n\r", ch);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          if (ch->equipment[WEAR_WRIST_L]) {
            sprintf(buffer, "You wear the %s around your right wrist.\n\r", 
              fname(obj_object->name));
            send_to_char(buffer, ch);
            equip_char(ch,  obj_object, WEAR_WRIST_R);
          } else {
            sprintf(buffer, "You wear the %s around your left wrist.\n\r", 
              fname(obj_object->name));
            send_to_char(buffer, ch);
            equip_char(ch, obj_object, WEAR_WRIST_L);
          }
        }
      } else {
        send_to_char("You can't wear that around your wrist.\n\r", ch);
      }
    } break;

    case 12:
      if (CAN_WEAR(obj_object,ITEM_WIELD)) {
        if (ch->equipment[WIELD]) {
          send_to_char("You are already wielding something.\n\r", ch);
        } else {
          /* Cleric execption has been removed, and is temporarily placed */
          /* at the end of this file                                      */

          if(GET_OBJ_WEIGHT(obj_object) > GET_STR(ch)) {
            send_to_char("It is too heavy for you to use.\n\r",ch);
          } else {
            send_to_char("OK.\n\r", ch);
            perform_wear(ch,obj_object,keyword);
            obj_from_char(obj_object);
            equip_char(ch, obj_object, WIELD);
          }
        }
      } else {
        send_to_char("You can't wield that.\n\r", ch);
      }
      break;

    case 13:
      if (CAN_WEAR(obj_object,ITEM_HOLD)) {
        if (ch->equipment[HOLD]) {
          send_to_char("You are already holding something.\n\r", ch);
        } else {
          /* Cleric execption has been removed, and is temporarily placed */
          /* at the end of this file                                      */

          send_to_char("OK.\n\r", ch);
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, HOLD);
        }
      } else {
        send_to_char("You can't hold this.\n\r", ch);
      }
      break;
    case 14: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_SHIELD)) {
        if ((ch->equipment[WEAR_SHIELD])) {
          send_to_char(
            "You are already using a shield\n\r", ch);
        } else {
          perform_wear(ch,obj_object,keyword);
          sprintf(buffer, "You start using the %s.\n\r", 
            fname(obj_object->name));
          send_to_char(buffer, ch);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_SHIELD);
        }
      } else {
        send_to_char("You can't use that as a shield.\n\r", ch);
      }
    } break;
    case 15: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_RADIO)) {
        if ((ch->equipment[WEAR_RADIO])) {
          send_to_char(
            "You are already using a communicator.\n\r", ch);
        } else {
          perform_wear(ch,obj_object,keyword);
          sprintf(buffer, "You start using the %s.\n\r", 
            fname(obj_object->name));
          send_to_char(buffer, ch);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_RADIO);
        }
      } else {
        send_to_char("You can't use that as a radio.\n\r", ch);
      }
    } break;
    case 16: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_FACE)) {
        if ((ch->equipment[WEAR_FACE])) {
          send_to_char(
            "You are already wearing something on your face.\n\r", ch);
        } else {
          perform_wear(ch,obj_object,keyword);
          sprintf(buffer, "You wear the %s.\n\r", 
            fname(obj_object->name));
          send_to_char(buffer, ch);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_FACE);
        }
      } else {
        send_to_char("You can't wear that on your face.\n\r", ch);
      }
    } break;
    case 17: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_EARS)) {
        if ((ch->equipment[WEAR_EARS])) {
          send_to_char(
            "You are already wearing something on your ears.\n\r", ch);
        } else {
          perform_wear(ch,obj_object,keyword);
          sprintf(buffer, "You wear the %s.\n\r", 
            fname(obj_object->name));
          send_to_char(buffer, ch);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_EARS);
        }
      } else {
        send_to_char("You can't wear that on your ears.\n\r", ch);
      }
    } break;
    case 18: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_ANKLES)) {
        if ((ch->equipment[WEAR_ANKLES])) {
          send_to_char(
            "You are already wearing something on your ankles.\n\r",ch);
        } else {
          perform_wear(ch,obj_object,keyword);
          sprintf(buffer, "You wear the %s.\n\r", 
            fname(obj_object->name));
          send_to_char(buffer, ch);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_ANKLES);
        }
      } else {
        send_to_char("You can't wear that on your ankles.\n\r", ch);
      }
    } break;
    case -1: {
      sprintf(buffer,"Wear %s where?.\n\r", fname(obj_object->name));
      send_to_char(buffer, ch);
    } break;
    case -2: {
      sprintf(buffer,"You can't wear the %s.\n\r", fname(obj_object->name));
      send_to_char(buffer, ch);
    } break;
    default: {
      log("Unknown type called in wear.");
    } break;
  }
}
#endif
int where_wear(struct obj_data *obj_object)
{
  if (CAN_WEAR(obj_object,ITEM_WEAR_FINGER)) return( 1 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_NECK)) return( 2 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_WRIST)) return( 11 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_WAISTE)) return( 10 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_ARMS)) return( 8 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_HANDS)) return( 7 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_FEET)) return( 6 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_LEGS)) return( 5 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_ABOUT)) return( 9 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_HEAD)) return( 4 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_BODY)) return( 3 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_SHIELD)) return( 14 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_RADIO)) return( 15 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_FACE)) return( 16 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_EARS)) return( 17 );
  if (CAN_WEAR(obj_object,ITEM_WEAR_ANKLES)) return( 18 );
  return(-2);
}
void do_wear(struct char_data *ch, char *argument, int cmd) {
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  char buf[256];
  char buffer[MAX_STRING_LENGTH];
  struct obj_data *obj_object,*next_object;
  int keyword;
  static char *keywords[] = {
    "finger",
    "neck",
    "body",
    "head",
    "legs",
    "feet",
    "hands",
    "arms",
    "about",
    "waist",
    "wrist",
    "",
    "",
    "shield",
    "radio",
    "face",
    "ears",
    "\n"
};

  wflag = 0;
  argument_interpreter(argument, arg1, arg2);
  if (*arg1) {
    if(strcmp(arg1,"all")==0){
      wflag = 1;
      for(obj_object = ch->carrying ; obj_object ; obj_object=next_object){
        next_object = obj_object->next_content;
        if(CAN_SEE_OBJ(ch,obj_object)){
          keyword = where_wear(obj_object);
          if(keyword < 0) continue;
          wear(ch,obj_object,keyword);
        }
      }
      update_pos(ch);
      if(GET_POS(ch) <= POSITION_INCAP)
        damage(ch,ch,1,TYPE_SUFFERING);
      return;
    }
    obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying);
    if (obj_object) {
      if (*arg2) {
        keyword = search_block(arg2, keywords, FALSE); /* Partial Match */
        if (keyword == -1) {
          sprintf(buf, "%s is an unknown body location.\n\r", arg2);
          send_to_char(buf, ch);
        } else {
          wear(ch, obj_object, keyword+1);
        }
      } else {
        keyword = where_wear(obj_object);
        wear(ch, obj_object, keyword);
      }
    } else {
      sprintf(buffer, "You do not seem to have the '%s'.\n\r",arg1);
      send_to_char(buffer,ch);
    }
  } else {
    send_to_char("Wear what?\n\r", ch);
  }
  update_pos(ch);
  if(GET_POS(ch) <= POSITION_INCAP)
    damage(ch,ch,1,TYPE_SUFFERING);
}
void do_wield(struct char_data *ch, char *argument, int cmd) {
char arg1[MAX_STRING_LENGTH];
char arg2[MAX_STRING_LENGTH];
char buffer[MAX_STRING_LENGTH];
struct obj_data *obj_object;
int keyword = 12;

  argument_interpreter(argument, arg1, arg2);
  if (*arg1) {
    obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying);
    if (obj_object) {
      wear(ch, obj_object, keyword);
    } else {
      sprintf(buffer, "You do not seem to have the '%s'.\n\r",arg1);
      send_to_char(buffer,ch);
    }
  } else {
    send_to_char("Wield what?\n\r", ch);
  }
}


void do_grab(struct char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  struct obj_data *obj_object;
  int keyword = 13;

  argument_interpreter(argument, arg1, arg2);
  if (*arg1) {
    obj_object = get_obj_in_list(arg1, ch->carrying);
    if (obj_object) {
      if (obj_object->obj_flags.type_flag == ITEM_LIGHT)
        wear(ch, obj_object, WEAR_LIGHT);
      else
        wear(ch, obj_object, 13);
    } else {
      sprintf(buffer, "You do not seem to have the '%s'.\n\r",arg1);
      send_to_char(buffer,ch);
    }
  } else {
    send_to_char("Hold what?\n\r", ch);
  }
}


void do_remove(struct char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_STRING_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  char buffer1[MAX_STRING_LENGTH];
  struct obj_data *obj_object;
  struct char_data *tmp_char;
  int i, j;

  one_argument(argument, arg1);

  if (*arg1) {
    if(!str_cmp(arg1,"all")){
      for(i=0;i<MAX_WEAR;++i){
        if(ch->equipment[i]){
          obj_to_char(unequip_char(ch,i),ch);
        }
      }
      send_to_char("Done.\n\r",ch);
      return;
    }
    obj_object = get_object_in_equip_vis(ch, arg1, ch->equipment, &j);
    if (obj_object) {
      if (CAN_CARRY_N(ch) != IS_CARRYING_N(ch)) {
        obj_to_char(unequip_char(ch, j), ch);
        if (obj_object->obj_flags.type_flag == ITEM_LIGHT)
          if (obj_object->obj_flags.value[2])
            world[ch->in_room].light--;
        act("You stop using $p.",FALSE,ch,obj_object,0,TO_CHAR);
        act("$n stops using $p.",TRUE,ch,obj_object,0,TO_ROOM);
      } else {
        send_to_char("You can't carry that many items.\n\r", ch);
      }
    } else {
      send_to_char("You are not using it.\n\r", ch);
    }
  } else {
    send_to_char("Remove what?\n\r", ch);
  }
}

void wear_replace(struct char_data *ch,
             struct obj_data *old_object,
             struct obj_data *obj_object, int keyword)
{

 switch(keyword) {
  case 0 :
    act("You remove $p and light $P.",FALSE,ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and lights $P.",TRUE,ch,old_object,obj_object,TO_ROOM);
      break;
  case 1 :
    act("You remove $p wear $P on your finger.",
        FALSE,ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p wears $P on $s finger.",
        TRUE,ch,old_object,obj_object,TO_ROOM);
    break;
  case 2 :
    act("You remove $p and wear $P around your neck.",
        FALSE,ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and wears $P around $s neck.",
        TRUE,ch,old_object,obj_object,TO_ROOM);
    break;
  case 3 :
    act("You remove $p and wear $P on your body.",
        FALSE,ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and wears $P on $s bodye",
        TRUE,ch,old_object,obj_object,TO_ROOM);
    break;
  case 4 :
    act("You remove $p and wear $P on your head.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and wears $P on $s head.",
        TRUE,ch,old_object,obj_object,TO_ROOM);
    break;
  case 5 :
    act("You remove $p and wear $P on your legs.",
        FALSE,ch,old_object,obj_object, TO_CHAR);
    act("$n removes $p and wears $P on $s legs.",
        TRUE,ch,old_object,obj_object,TO_ROOM);
    break;
  case 6 :
    act("You remove $p and wear $P on your feet.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n remove $p and wears $P on $s feet.",
        TRUE, ch,old_object,obj_object,TO_ROOM);
    break;
  case 7 :
    act("You remove $p and wear $P on your hands.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n remove $p and wears $P on $s hands.",
        TRUE, ch,old_object,obj_object,TO_ROOM);
    break;
  case 8 :
    act("You remove $p and wear $P on your arms.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and wears $P on $s arms.",
        TRUE, ch, old_object,obj_object,TO_ROOM);
    break;
  case 9 :
    act("You remove $p and wear $P about your body.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and wears $P about $s body.",
        TRUE, ch,old_object,obj_object,TO_ROOM);
    break;
  case 10 :
    act("You remove $p and wear $P about your waist.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and wears $P about $s waist.",
        TRUE, ch,old_object,obj_object,TO_ROOM);
    break;
  case 11 :
    act("You remove $p and wear $P around your wrist.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and wears $P around $s wrist.",
        TRUE, ch,old_object,obj_object,TO_ROOM);
    break;
  case 12 :
    act("You remove $p and wield $P.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and wields $P.",
        TRUE, ch,old_object,obj_object,TO_ROOM);
    break;
  case 13 :
    act("You remove $p and grab $P.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and grabs $P.",
        TRUE, ch,old_object,obj_object,TO_ROOM);
    break;
  case 14 :
    act("You remove $p and start using $P as a shield.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and starts using $P as a shield.",
        TRUE, ch,old_object,obj_object,TO_ROOM);
    break;
  case 15 :
    act("You remove $p and start using $P as a radio.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and starts using $P as a radio.",
        TRUE, ch,old_object,obj_object,TO_ROOM);
    break;
  case 16 :
    act("You remove $p and wear $P on your face.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and wears $P on $s face.",
        TRUE, ch,old_object,obj_object,TO_ROOM);
    break;
  case 17 :
    act("You remove $p and wear $P on your ears.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and wears $P on $s ears.",
        TRUE, ch,old_object,obj_object,TO_ROOM);
    break;
  case 18 :
    act("You remove $p and wear $P on your ankles.",
        FALSE, ch,old_object,obj_object,TO_CHAR);
    act("$n removes $p and wears $P on $s ankles.",
        TRUE, ch,old_object,obj_object,TO_ROOM);
    break;
  }
}
#ifdef NEWWEAR
void wear(struct char_data *ch, struct obj_data *obj_object, int keyword)
{
  char buffer[MAX_STRING_LENGTH];
  struct obj_data *old_object;

  switch(keyword) {
    case 0: {
      if(ch->equipment[WEAR_LIGHT]) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_LIGHT];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_LIGHT),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_LIGHT);
        if(obj_object->obj_flags.value[2])
          world[ch->in_room].light++;
      } else {
        perform_wear(ch,obj_object,keyword);
        obj_from_char(obj_object);
        equip_char(ch,obj_object, WEAR_LIGHT);
        if(obj_object->obj_flags.value[2])
          world[ch->in_room].light++;
      }
    } break;
    case 1: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_FINGER)) {
       if((ch->equipment[WEAR_FINGER_L]) && (ch->equipment[WEAR_FINGER_R])) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_FINGER_L];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_FINGER_L),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_FINGER_L);
        } else {
          perform_wear(ch,obj_object,keyword);
          if (ch->equipment[WEAR_FINGER_L]) {
            sprintf(buffer, "You put the %s on your right finger.\n\r",
              fname(obj_object->name));
            send_to_char(buffer, ch);
            obj_from_char(obj_object);
            equip_char(ch, obj_object, WEAR_FINGER_R);
          } else {
            sprintf(buffer, "You put the %s on your left finger.\n\r",
              fname(obj_object->name));
            send_to_char(buffer, ch);
            obj_from_char(obj_object);
            equip_char(ch, obj_object, WEAR_FINGER_L);
          }
        }
      } else {
        send_to_char("You can't wear that on your finger.\n\r", ch);
      }
    } break;
    case 2: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_NECK)) {
       if((ch->equipment[WEAR_NECK_1]) && (ch->equipment[WEAR_NECK_2])) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_NECK_1];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_NECK_1),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_NECK_1);
        } else {
          perform_wear(ch,obj_object,keyword);
          if (ch->equipment[WEAR_NECK_1]) {
            obj_from_char(obj_object);
            equip_char(ch, obj_object, WEAR_NECK_2);
          } else {
            obj_from_char(obj_object);
            equip_char(ch, obj_object, WEAR_NECK_1);
          }
        }
      } else {
              send_to_char("You can't wear that around your neck.\n\r", ch);
      }
    } break;
    case 3: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_BODY)) {
       if(ch->equipment[WEAR_BODY]) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_BODY];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_BODY),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_BODY);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch,  obj_object, WEAR_BODY);
        }
      } else {
        send_to_char("You can't wear that on your body.\n\r", ch);
      }
    } break;
    case 4: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_HEAD)) {
       if(ch->equipment[WEAR_HEAD]) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_HEAD];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_HEAD),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_HEAD);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_HEAD);
        }
      } else {
        send_to_char("You can't wear that on your head.\n\r", ch);
      }
    } break;
    case 5: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_LEGS)) {
       if(ch->equipment[WEAR_LEGS]) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_LEGS];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_LEGS),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_LEGS);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_LEGS);
        }
      } else {
        send_to_char("You can't wear that on your legs.\n\r", ch);
      }
    } break;
    case 6: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_FEET)) {
       if(ch->equipment[WEAR_FEET]) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_FEET];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_FEET),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_FEET);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_FEET);
        }
      } else {
        send_to_char("You can't wear that on your feet.\n\r", ch);
      }
    } break;
    case 7: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_HANDS)) {
       if(ch->equipment[WEAR_HANDS]) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_HANDS];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_HANDS),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_HANDS);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_HANDS);
        }
      } else {
        send_to_char("You can't wear that on your hands.\n\r", ch);
      }
    } break;
    case 8: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_ARMS)) {
       if(ch->equipment[WEAR_ARMS]) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_ARMS];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_ARMS),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_ARMS);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_ARMS);
        }
      } else {
        send_to_char("You can't wear that on your arms.\n\r", ch);
      }
    } break;
    case 9: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_ABOUT)) {
       if(ch->equipment[WEAR_ABOUT]) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_ABOUT];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_ABOUT),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_ABOUT);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_ABOUT);
        }
      } else {
        send_to_char("You can't wear that about your body.\n\r", ch);
      }
    } break;
    case 10: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_WAISTE)) {
       if(ch->equipment[WEAR_WAISTE]) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_WAISTE];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_WAISTE),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_WAISTE);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch,  obj_object, WEAR_WAISTE);
        }
      } else {
        send_to_char("You can't wear that about your waist.\n\r", ch);
      }
    } break;
    case 11: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_WRIST)) {
       if((ch->equipment[WEAR_WRIST_L]) && (ch->equipment[WEAR_WRIST_R])) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_WRIST_L];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_WRIST_L),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_WRIST_L);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          if (ch->equipment[WEAR_WRIST_L]) {
            sprintf(buffer, "You wear the %s around your right wrist.\n\r",
              fname(obj_object->name));
            send_to_char(buffer, ch);
            equip_char(ch,  obj_object, WEAR_WRIST_R);
          } else {
            sprintf(buffer, "You wear the %s around your left wrist.\n\r",
              fname(obj_object->name));
            send_to_char(buffer, ch);
            equip_char(ch, obj_object, WEAR_WRIST_L);
          }
        }
      } else {
        send_to_char("You can't wear that around your wrist.\n\r", ch);
      }
    } break;
    case 12: {
      if(CAN_WEAR(obj_object,ITEM_WIELD)) {
       if(ch->equipment[WIELD]) {
        if(GET_OBJ_WEIGHT(obj_object) > GET_STR(ch)) {
           send_to_char("It is too heavy for you to use.\n\r",ch);
        } else {
        if(wflag) return;
        old_object=ch->equipment[WIELD];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WIELD),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WIELD);
        }
       } else {
          if(GET_OBJ_WEIGHT(obj_object) > GET_STR(ch)) {
            send_to_char("It is too heavy for you to use.\n\r",ch);
          } else {
            perform_wear(ch,obj_object,keyword);
            obj_from_char(obj_object);
            equip_char(ch, obj_object, WIELD);
          }
        }
      } else {
        send_to_char("You can't wield that.\n\r", ch);
      }
    } break;
    case 13: {
     if(CAN_WEAR(obj_object,ITEM_HOLD)) {
       if(ch->equipment[HOLD]) {
        if(wflag) return;
        old_object=ch->equipment[HOLD];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,HOLD),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,HOLD);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, HOLD);
        }
      } else {
        send_to_char("You can't hold this.\n\r", ch);
      }
    } break;
    case 14: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_SHIELD)) {
       if((ch->equipment[WEAR_SHIELD])) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_SHIELD];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_SHIELD),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_SHIELD);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_SHIELD);
        }
      } else {
        send_to_char("You can't use that as a shield.\n\r", ch);
      }
    } break;
    case 15: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_RADIO)) {
       if((ch->equipment[WEAR_RADIO])) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_RADIO];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_RADIO),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_RADIO);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_RADIO);
        }
      } else {
        send_to_char("You can't use that as a radio.\n\r", ch);
      }
    } break;
    case 16: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_FACE)) {
       if((ch->equipment[WEAR_FACE])) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_FACE];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_FACE),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_FACE);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_FACE);
        }
      } else {
        send_to_char("You can't wear that on your face.\n\r", ch);
      }
    } break;
    case 17: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_EARS)) {
       if((ch->equipment[WEAR_EARS])) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_EARS];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_EARS),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_EARS);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_EARS);
        }
      } else {
        send_to_char("You can't wear that on your ears.\n\r", ch);
      }
    } break;
    case 18: {
      if(CAN_WEAR(obj_object,ITEM_WEAR_ANKLES)) {
       if((ch->equipment[WEAR_ANKLES])) {
        if(wflag) return;
        old_object=ch->equipment[WEAR_ANKLES];
        wear_replace(ch,old_object,obj_object,keyword);
        obj_to_char(unequip_char(ch,WEAR_ANKLES),ch);
        obj_from_char(obj_object);
        equip_char(ch,obj_object,WEAR_ANKLES);
        } else {
          perform_wear(ch,obj_object,keyword);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_ANKLES);
        }
      } else {
        send_to_char("You can't wear that on your ankles.\n\r", ch);
      }
    } break;
    case -1: {
      sprintf(buffer,"Wear %s where?.\n\r", fname(obj_object->name));
      send_to_char(buffer, ch);
    } break;
    case -2: {
      sprintf(buffer,"You can't wear the %s.\n\r", fname(obj_object->name));
      send_to_char(buffer, ch);
    } break;
    default: {
      log("Unknown type called in wear.");
    } break;
  }
}
#endif


