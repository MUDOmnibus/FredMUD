/* ************************************************************************
*  file: act.obj3.c  -  Exoo addition
************************************************************************* */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"

extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;

static int wplist[]={
  65536,
      2,
      2,
      4,
      4,
      8,
     16,
     32,
     64,
    128,
    256,
    512,
   1024,
   2048,
   4096,
   4096,
   8192,
  16384,
  32768,
 131072,
 262144,
 524288,
1048576,
      0
};
static char *wpname[]={
 "light",
 "finger",
 "finger",
 "neck",
 "neck",
 "body",
 "head",
 "legs",
 "feet",
 "hands",
 "arms",
 "shield",
 "about",
 "waist",
 "wrist",
 "wrist",
 "wield",
 "hold",
 "radio",
 "face",
 "ears",
 "huh",
 "ankle",
 0
};

void do_dress(struct char_data *ch, char *argument, int cmd)
{
  struct obj_data *obj, *nextobj, *bestobj;
  int i, j, k, best, key, val;
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  char buffer[MAX_STRING_LENGTH];

  argument_interpreter(argument, arg1, arg2);
  if (*arg1) {
    if(strcmp(arg1,"str")==0)
      key = 1;
    else if(strcmp(arg1,"dex")==0)
      key = 2;
    else if(strcmp(arg1,"int")==0)
      key = 3;
    else if(strcmp(arg1,"wis")==0)
      key = 4;
    else if(strcmp(arg1,"con")==0)
      key = 5;
    else if(strcmp(arg1,"ac")==0)
      key = 17;
    else if(strcmp(arg1,"hit")==0)
      key = 18;
    else if(strcmp(arg1,"dam")==0)
      key = 19;
    else if(strcmp(arg1,"mr")==0)
      key = 25;
    else
      key = 17;
  }
/*
  for(i=0;i<MAX_WEAR;++i)
    if(ch->equipment[i])
      obj_to_char(unequip_char(ch,i),ch);
*/
  for(i=0;i<MAX_WEAR;i++){
    if(ch->equipment[i]) continue;
    best = 0; bestobj = NULL;
    for(obj = ch->carrying ; obj ; obj=nextobj){
      nextobj = obj->next_content;
      if(CAN_SEE_OBJ(ch,obj)){
        if(CAN_WEAR(obj,wplist[i])){
          for(j=0;j<MAX_OBJ_AFFECT;j++){
            if(obj->affected[j].location == key){
              val = obj->affected[j].modifier;
              if((key==17) && (GET_ITEM_TYPE(obj)==ITEM_ARMOR))
                val += obj->obj_flags.value[0];
              if(val > best){
                best = val;
                bestobj = obj;
              }
              break;
            }
          }
        }
      }
    }
    if(bestobj){
      obj_from_char(bestobj);
      equip_char(ch, bestobj, i);
      sprintf(buffer,"You wear (%s) the $o for %d.",wpname[i],best);
      act(buffer,TRUE,ch,bestobj,0,TO_CHAR);
    }
  }
  send_to_char("Hmm, better check it out...\n\r", ch);
}
