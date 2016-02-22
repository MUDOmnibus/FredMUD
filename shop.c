
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"

#define SHOP_FILE       "tinyworld.shp"
#define MAX_TRADE        5
#define MAX_PROD         5

#define MAX_SALE_ITEMS 100

#define MAXSHOPMESS      4

char *no_such_item1[]={
  "%s Haven't got that on storage - try LIST.",
  "%s I don't have any of those.",
  "%s I don't sell those, try Home Depot.",
  "%s I don't deal in that kind crap.",
  "%s Do you see any of those around here?  I don't."
};
char *no_such_item2[]={
  "%s You don't seem to have that.",
  "%s You don't have any.",
  "%s Where are you hiding it?",
  "%s I don't think you have any.",
  "%s Who are you trying to kid?"
};
char *do_not_buy[]={
  "%s I don't buy THAT. Try another shop.",
  "%s I don't want it.",
  "%s I don't need it.",
  "%s Looks like trash to me, no thanks.",
  "%s Go drop that in the dump."
};
char *missing_cash[]={
  "%s You seem a little short on coins, pal.",
  "%s Sorry, I don't give credit.",
  "%s Go stop at the bank first.",
  "%s On your budget?  No chance.",
  "%s My price is too high for you.  Scram."
};
char *message_buy[]={
  "%s That'll be %d coins, thank you.",
  "%s Yours for only %d coins.",
  "%s I'll take your %d coins, thanks.",
  "%s A shrewd investment for only %d coins.",
  "%s Here it is.  I'll take %d coins."
};
char *message_sell[]={
  "%s You'd get %d coins for it.",
  "%s That's worth %d coins.",
  "%s That's worth %d coins.",
  "%s That's worth %d coins.",
  "%s I'd give you %d coins and not a penny more."
};
struct sale_item {
  int n;
  int k;
  struct obj_data *p;
} salelist[MAX_SALE_ITEMS];

/* extern struct str_app_type str_app[]; */
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern char *apply_types[];

char *fread_string(FILE *fl);

void show_item_list(char buffer[], int shop_nr);
int fill_item_list(struct obj_data *h);
void object_identify_to_char(struct char_data *ch, struct obj_data *o);

struct shop_data
{
  int producing[MAX_PROD];/* Which item to produce (virtual)      */
  float profit_buy;       /* Factor to multiply cost with.        */
  float profit_sell;      /* Factor to multiply cost with.        */
  byte type[MAX_TRADE];   /* Which item to trade.                 */
  int keeper;             /* The mobil who owns the shop (virtual)*/
  int with_who;    /* Who does the shop trade with?  */
  int in_room;    /* Where is the shop?      */
  int open1,open2;  /* When does the shop open?    */
  int close1,close2;  /* When does the shop close?    */
};

extern struct room_data *world;
extern struct time_info_data time_info;

struct shop_data *shop_index;
int number_of_shops;

int is_ok(struct char_data *keeper, struct char_data *ch, int shop_nr)
{
  if (shop_index[shop_nr].open1>time_info.hours){
    do_say(keeper,
    "Come back later!",17);
    return(FALSE);
  } else if (shop_index[shop_nr].close1<time_info.hours)
    if (shop_index[shop_nr].open2>time_info.hours){
      do_say(keeper,
      "Sorry, we have closed, but come back later.",17);
      return(FALSE);
    } else if (shop_index[shop_nr].close2<time_info.hours){
      do_say(keeper,
      "Sorry, come back tomorrow.",17);
      return(FALSE);
    };

  if(!(CAN_SEE(keeper,ch))) {
    do_say(keeper, "I don't trade with someone I can't see!",17);
    return(FALSE);
  };

  switch(shop_index[shop_nr].with_who){
    case 0 : return(TRUE);
    case 1 : return(TRUE);
    default : return(TRUE);
  };
}

int trade_with(struct obj_data *item, int shop_nr)
{
  int counter;

  if(item->obj_flags.cost < 1)
     return(FALSE);
  if(shop_index[shop_nr].type[0] == -2)
    return TRUE;
  for(counter=0;counter<MAX_TRADE;counter++)
    if(shop_index[shop_nr].type[counter]==item->obj_flags.type_flag)
      return(TRUE);
  return(FALSE);
}

int shop_producing(struct obj_data *item, int shop_nr)
{
  int counter;

  if(item->item_number<0) return(FALSE);

  for(counter=0;counter<MAX_PROD;counter++)
    if (shop_index[shop_nr].producing[counter] == item->item_number)
      return(TRUE);
  return(FALSE);
}

void shopping_appraise( char *arg, struct char_data *ch,
   struct char_data *keeper, int shop_nr)
{
  char argm[100], buf[MAX_STRING_LENGTH];
  struct obj_data *temp1;
  int k,n;

  if(!(is_ok(keeper,ch,shop_nr)))
    return;
  one_argument(arg, argm);
  if(!(*argm)) {
    sprintf(buf,"%s what do you want to appraise?",GET_NAME(ch));
    do_tell(keeper,buf,19);
    return;
  }
  if(isdigit(*argm)){
    n = fill_item_list(keeper->carrying);
    k = atoi(argm) - 1;
    if((k < 0)||(k >= n)){
      sprintf(buf, no_such_item1[number(0,MAXSHOPMESS)],GET_NAME(ch));
      do_tell(keeper,buf,19);
      return;
    }
    temp1 = salelist[k].p;
  } else if(!( temp1 = get_obj_in_list_vis(ch,argm,keeper->carrying))) {
    sprintf(buf, no_such_item1[number(0,MAXSHOPMESS)],GET_NAME(ch));
    do_tell(keeper,buf,19);
    return;
  }
  object_identify_to_char(ch,temp1);
  return; 
}
void shopping_buy( char *arg, struct char_data *ch,
   struct char_data *keeper, int shop_nr)
{
  char argm[100], buf[MAX_STRING_LENGTH];
  struct obj_data *temp1;
  struct char_data *temp_char;
  int k,n;

  if(!(is_ok(keeper,ch,shop_nr)))
    return;

  one_argument(arg, argm);
  if(!(*argm)) {
    sprintf(buf,"%s what do you want to buy??",GET_NAME(ch));
    do_tell(keeper,buf,19);
    return;
  }
  if(isdigit(*argm)){
    n = fill_item_list(keeper->carrying);
    k = atoi(argm) - 1;
    if((k < 0)||(k >= n)){
      sprintf(buf, no_such_item1[number(0,MAXSHOPMESS)],GET_NAME(ch));
      do_tell(keeper,buf,19);
      return;
    }
    temp1 = salelist[k].p;
  } else if(!( temp1 = get_obj_in_list_vis(ch,argm,keeper->carrying))) {
    sprintf(buf, no_such_item1[number(0,MAXSHOPMESS)],GET_NAME(ch));
    do_tell(keeper,buf,19);
    return;
  }

  if(temp1->obj_flags.cost <= 0) {
    sprintf(buf, no_such_item1[number(0,MAXSHOPMESS)],GET_NAME(ch));
    do_tell(keeper,buf,19);
    extract_obj(temp1);
    return;
  }

  if(GET_GOLD(ch) < (int) (temp1->obj_flags.cost*
    shop_index[shop_nr].profit_buy) && GET_LEVEL(ch)<(IMO+1)) {
    sprintf(buf, missing_cash[number(0,MAXSHOPMESS)],GET_NAME(ch));
    do_tell(keeper,buf,19);
    return;
  }
  
  if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))) {
    sprintf(buf,"%s: You can't carry that many items.\n\r",fname(temp1->name));
    send_to_char(buf, ch);
    return;
  }

  if ((IS_CARRYING_W(ch) + temp1->obj_flags.weight) > CAN_CARRY_W(ch)) {
    strcpy(buf,"I guess it's too heavy for you.\n\r");
    send_to_char(buf, ch);
    return;
  }
  act("$n buys $p.", FALSE, ch, temp1, 0, TO_ROOM);
  sprintf(buf, message_buy[number(0,MAXSHOPMESS)], GET_NAME(ch),
    (int) (temp1->obj_flags.cost* shop_index[shop_nr].profit_buy));
  do_tell(keeper,buf,19);
  sprintf(buf,"You now have %s.\n\r", temp1->short_description);
  send_to_char(buf,ch);
  if(GET_LEVEL(ch) <= IMO)
    GET_GOLD(ch)-=(int)(temp1->obj_flags.cost* shop_index[shop_nr].profit_buy);

  if(shop_producing(temp1,shop_nr))
    temp1 = read_object(temp1->item_number, REAL);
  else
    obj_from_char(temp1);
  obj_to_char(temp1,ch);
  return; 
}

void shopping_sell( char *arg, struct char_data *ch,
   struct char_data *keeper,int shop_nr)
{
  char argm[100], buf[MAX_STRING_LENGTH];
  struct obj_data *temp1;
  struct char_data *temp_char;

  if(!(is_ok(keeper,ch,shop_nr)))
    return;

  one_argument(arg, argm);

  if(!(*argm)) {
    sprintf(buf, "%s What do you want to sell??" ,GET_NAME(ch));
    do_tell(keeper,buf,19);
    return;
  }

  if(!( temp1 = get_obj_in_list_vis(ch,argm,ch->carrying))) {
    sprintf(buf, no_such_item2[number(0,MAXSHOPMESS)] ,GET_NAME(ch));
    do_tell(keeper,buf,19);
    return;
  }

  if(!(trade_with(temp1,shop_nr))||(temp1->obj_flags.cost<1)) {
    sprintf(buf, do_not_buy[number(0,MAXSHOPMESS)], GET_NAME(ch));
    do_tell(keeper,buf,19);
    return;
  }

  act("$n sells $p.", FALSE, ch, temp1, 0, TO_ROOM);

  sprintf(buf,message_sell[number(0,MAXSHOPMESS)],
    GET_NAME(ch),
    (int) (temp1->obj_flags.cost* shop_index[shop_nr].profit_sell));
  do_tell(keeper,buf,19);
  sprintf(buf,"The shopkeeper now has %s.\n\r", temp1->short_description);
  send_to_char(buf,ch);
  GET_GOLD(ch)+=(int)(temp1->obj_flags.cost*shop_index[shop_nr].profit_sell);

  obj_from_char(temp1);
  obj_to_char(temp1,keeper);
  return;
}

void shopping_value( char *arg, struct char_data *ch, 
  struct char_data *keeper, int shop_nr)
{
  char argm[100], buf[MAX_STRING_LENGTH];
  struct obj_data *temp1;

  if(!(is_ok(keeper,ch,shop_nr)))
    return;

  one_argument(arg, argm);

  if(!(*argm)) {
    sprintf(buf,"%s What do you want me to valuate??", GET_NAME(ch));
    do_tell(keeper,buf,19);
    return;
  }

  if(!( temp1 = get_obj_in_list_vis(ch,argm,ch->carrying))) {
    sprintf(buf,no_such_item2[number(0,MAXSHOPMESS)],
      GET_NAME(ch));
    do_tell(keeper,buf,19);
    return;
  }

  if(!(trade_with(temp1,shop_nr))) {
    sprintf(buf, do_not_buy[number(0,MAXSHOPMESS)], GET_NAME(ch));
    do_tell(keeper,buf,19);
    return;
  }
  sprintf(buf,message_sell[number(0,MAXSHOPMESS)],
    GET_NAME(ch),(int) (temp1->obj_flags.cost*
      shop_index[shop_nr].profit_sell));
  do_tell(keeper,buf,19);

  return;
}

void shopping_list( char *arg, struct char_data *ch,
   struct char_data *keeper, int shop_nr)
{
  char buffer[8*MAX_STRING_LENGTH];
  int n;

  if(!(is_ok(keeper,ch,shop_nr)))
    return;

  n = fill_item_list(keeper->carrying);
  if(n > 0){
    show_item_list(buffer,shop_nr);
    page_string(ch->desc, buffer, 1);
  } else
    send_to_char("Nothing for sale.\n\r",ch);
}

int shop_keeper(struct char_data *ch, int cmd, char *arg)
{
  char argm[100], buf[MAX_STRING_LENGTH];
  struct obj_data *temp1;
  struct char_data *temp_char;
  struct char_data *keeper;
  int shop_nr,n;

  keeper = 0;
  for (temp_char = world[ch->in_room].people; (!keeper) && (temp_char) ; 
    temp_char = temp_char->next_in_room)
  if (IS_MOB(temp_char))
    if (mob_index[temp_char->nr].func == shop_keeper)
      keeper = temp_char;
  for(shop_nr=0 ; shop_index[shop_nr].keeper != keeper->nr; shop_nr++);
  if((cmd == 298) && (ch->in_room == 
     real_room(shop_index[shop_nr].in_room)))
  {
    shopping_appraise(arg,ch,keeper,shop_nr);
    return(TRUE);
  }
  if((cmd == 56) && (ch->in_room == 
     real_room(shop_index[shop_nr].in_room)))
  {
    shopping_buy(arg,ch,keeper,shop_nr);
    return(TRUE);
  }
  if((cmd ==57 ) && (ch->in_room == 
     real_room(shop_index[shop_nr].in_room))) /* Sell */
  {
    shopping_sell(arg,ch,keeper,shop_nr);
    return(TRUE);
  }
  if((cmd == 58) && (ch->in_room == 
     real_room(shop_index[shop_nr].in_room))) /* value */
  {
    shopping_value(arg,ch,keeper,shop_nr);
    return(TRUE);
  }
  if((cmd==59)&&(ch->in_room==real_room(shop_index[shop_nr].in_room))){
    /* List */
    shopping_list(arg,ch,keeper,shop_nr);
    return(TRUE);
  }
  if (cmd == 156){
    send_to_char("Oops.\n\r",ch);
    hit(keeper,ch,-1);
    return(TRUE);
  }
  if ((cmd==84) || (cmd==207) || (cmd==172)) {   /* Cast, recite, use */
    act("$N tells you 'No magic here - kid!'.", FALSE, ch, 0, keeper, TO_CHAR);
    return TRUE;
  }
  return(FALSE);
}

void boot_the_shops()
{
  char *buf;
  int temp;
  int count;
  FILE *shop_f;

  if (!(shop_f = fopen(SHOP_FILE, "r")))
  {
    perror("Error in boot shop\n");
    exit(0);
  }

  number_of_shops = 0;

  for(;;) {
    buf = fread_string(shop_f);
    if(*buf == '#') {
      if(!number_of_shops)
        CREATE(shop_index, struct shop_data, 1);
      else if(!(shop_index=
        (struct shop_data*) realloc(shop_index,(number_of_shops + 1)*
        sizeof(struct shop_data))))
        {
          perror("Error in boot shop\n");
          exit(0);
        }
      for(count=0;count<MAX_PROD;count++) {
        fscanf(shop_f,"%d \n", &temp);
        if (temp >= 0)
          shop_index[number_of_shops].producing[count]= real_object(temp);
        else
          shop_index[number_of_shops].producing[count]= temp;
      }
      fscanf(shop_f,"%f \n",
        &shop_index[number_of_shops].profit_buy);
      fscanf(shop_f,"%f \n",
        &shop_index[number_of_shops].profit_sell);
      for(count=0;count<MAX_TRADE;count++) {
         fscanf(shop_f,"%d \n", &temp);
         shop_index[number_of_shops].type[count] = (byte) temp;
      }
      fscanf(shop_f,"%d \n", &shop_index[number_of_shops].keeper);
      shop_index[number_of_shops].keeper = 
        real_mobile(shop_index[number_of_shops].keeper);
      fscanf(shop_f,"%d \n", &shop_index[number_of_shops].with_who);
      fscanf(shop_f,"%d \n", &shop_index[number_of_shops].in_room);
      fscanf(shop_f,"%d \n", &shop_index[number_of_shops].open1);
      fscanf(shop_f,"%d \n", &shop_index[number_of_shops].close1);
      fscanf(shop_f,"%d \n", &shop_index[number_of_shops].open2);
      fscanf(shop_f,"%d \n", &shop_index[number_of_shops].close2);
      number_of_shops++;
    } else if(*buf == '$'){
        break;
    }
  }
  fclose(shop_f);
}
void assign_the_shopkeepers()
{
  int i;
  
  for(i=0 ; i<number_of_shops ; i++)
    mob_index[shop_index[i].keeper].func = shop_keeper;
}

int fill_item_list(struct obj_data *h)
{
  struct obj_data *p;
  int f,i,j,k,n=0;

  for(p=h;p;p=p->next_content){
    f = 0;
    k = p->item_number;
    for(i=0;i<n;i++)
      if(salelist[i].k == k){
        f = 1;
        salelist[i].n++;
        break;
      }
    if(!f){
      salelist[n].n = 1;
      salelist[n].k = k;
      salelist[n].p = p;
      n++;
      if(n==(MAX_SALE_ITEMS-1))
        break;
    }
  }
  salelist[n].p = 0;
  return n;
}
void show_item_list(char buffer[], int shop_nr)
{
  int i;
  char tbuf[256];
  struct obj_data *p;

  strcpy(buffer,"You can buy:\n\r");
  for(i=0;salelist[i].p;i++){
    p = salelist[i].p;
    sprintf(tbuf,"%2d: %-30.30s%7d (%2d)\n\r",
      i+1,
      p->short_description,
      (int)(p->obj_flags.cost * shop_index[shop_nr].profit_buy),
      salelist[i].n);
    strcat(buffer,tbuf);
  }
}
void object_identify_to_char(struct char_data *ch, struct obj_data *j)
{
  int i,virtual;
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  extern char *item_types[];
  extern char *wear_bits[];
  extern char *extra_bits[];
  extern char *drinks[];

  virtual = (j->item_number >= 0) ? obj_index[j->item_number].virtual : 0;
  sprintf(buf,"%s: ",j->short_description);
  sprinttype(GET_ITEM_TYPE(j),item_types,buf2);
  strcat(buf,buf2);
  strcat(buf,"\n\r");
  send_to_char(buf, ch);
  sprintf(buf,"Wgt: %d, Value: %d, Rent Lev: %d\n\r",
      j->obj_flags.weight,j->obj_flags.cost,j->obj_flags.rentlevel);
  send_to_char(buf, ch);
  send_to_char("Worn: ", ch);
  sprintbit(0x7ffffffe & j->obj_flags.wear_flags,wear_bits,buf);
  strcat(buf,"\n\r");
  send_to_char(buf, ch);
  switch(GET_ITEM_TYPE(j)){
    case ITEM_WEAPON:
      sprintf(buf,"Damage dice: %dd%d\n\r",
        j->obj_flags.value[1],
        j->obj_flags.value[2]);
      break;
    case ITEM_ARMOR:
      sprintf(buf,"Armor value: %d\n\r",
        j->obj_flags.value[0]);
      break;
    default:
      strcpy(buf,"More on this type of item later.\n\r");
      break;
  }
  send_to_char(buf, ch);
  for (i=0;i<MAX_OBJ_AFFECT;++i) {
    if(!j->affected[i].location)
      continue;
    sprinttype(j->affected[i].location,apply_types,buf2);
    sprintf(buf,"  Affects: %s by %d\n\r",buf2,j->affected[i].modifier);
    send_to_char(buf, ch);      
  }
}




