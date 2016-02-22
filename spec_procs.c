
#define FUDGE (100+dice(6,20))

#define INC(X,Y) X=((X < MAX(MAX_STAT,GET_LEVEL(Y))) ? X+1 : X)

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "limits.h"

int defaulthelper=1707;
int droptotal=0;

extern struct command_info cmd_info[];
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern struct index_data *mob_index;
extern int top_of_world;
extern int revdir[];
extern char *dirletters;

/* extern procedures */

struct char_data *rand_player_room(struct char_data *ch);
struct char_data *rand_player_world(struct char_data *ch);
struct char_data *rand_mob_room(struct char_data *ch);
struct char_data *rand_char_room(struct char_data *ch);
struct obj_data *rand_object();
char *rand_playername_file();
char *rand_obscenity();

void spell_word_of_recall(short level, struct char_data *ch,
  struct char_data *victim, struct obj_data *obj);
void cast_magic_missile(short level,struct char_data *ch,char *arg,int type,
  struct char_data *victim, struct obj_data *tar_obj );
void cast_burning_hands(short level,struct char_data *ch,char *arg,int type,
  struct char_data *victim, struct obj_data *tar_obj );
void cast_blindness(short level, struct char_data *ch, char *arg, int type,
  struct char_data *tar_ch, struct obj_data *tar_obj );
void cast_lightning_bolt(short level,struct char_data *ch,char *arg,int type,
  struct char_data *victim, struct obj_data *tar_obj );
void cast_colour_spray(short level,struct char_data *ch,char *arg,int type,
  struct char_data *victim, struct obj_data *tar_obj );
void cast_fireball(short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj );
void cast_sunburst(short level, struct char_data *ch, char *arg, int type,
  struct char_data *victim, struct obj_data *tar_obj );
void cast_energy_drain(short level,struct char_data *ch,char *arg,int type,
  struct char_data *victim, struct obj_data *tar_obj );
void cast_harm(short level,struct char_data *ch,char *arg,int type,
  struct char_data *victim, struct obj_data *tar_obj );
void cast_radiation(short level,struct char_data *ch,char *arg,int type,
  struct char_data *victim, struct obj_data *tar_obj );

void add_follower(struct char_data *ch, struct char_data *leader);
void do_say(struct char_data *ch, char *argument, int cmd);
void do_shout(struct char_data *ch, char *argument, int cmd);
void do_move(struct char_data *ch, char *argument, int cmd);
void do_open(struct char_data *ch, char *argument, int cmd);
void do_action(struct char_data *ch, char *argument, int cmd);
void affect_remove( struct char_data *ch, struct affected_type *af );
void do_get(struct char_data *ch, char *argument, int cmd);
void do_drop(struct char_data *ch, char *argument, int cmd);
void do_flee(struct char_data *ch, char *argument, int cmd);
void hit(struct char_data *ch, struct char_data *victim, int type);
void gain_exp(struct char_data *ch, int gain);
void advance_level(struct char_data *ch);
void set_title(struct char_data *ch);
void cast_cure_critic(short level, struct char_data *ch, char *arg, int type,
     struct char_data *tar_ch, struct obj_data *tar_obj );
void cast_poison(short level, struct char_data *ch, char *arg, int type,
     struct char_data *tar_ch, struct obj_data *tar_obj );
void do_close(struct char_data *ch, char *argument, int cmd);
void do_lock(struct char_data *ch, char *argument, int cmd);
void do_unlock(struct char_data *ch, char *argument, int cmd);

#ifdef NEEDS_STRDUP
char *strdup(char *source);
#endif

void spellcast(struct char_data *ch,struct char_data *vict, int lev);
int check4enemies(struct char_data *ch, int type);
void to_long_long(char *s, long long n);

#define NBOTCMDS 9

static char *botcmds="RTGDIJFCS";

char *random_bot_program()
{
  char *s;
  int i,n;

  n=number(5,13);
  s=(char *)malloc(n+1);
  for(i=0;i<n;i++)
    s[i]=botcmds[number(0,NBOTCMDS-1)];
  s[n]=0;
  return s;
}
struct char_data *gettarget(struct char_data *ch, int flag)
{
  struct char_data *tmp;
  struct descriptor_data *i;

  if(flag){
    for(tmp=world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
      if(!IS_NPC(tmp) && CAN_SEE(ch,tmp) && number(0,2))
        return(tmp);
  } else {
    for(i=descriptor_list;i;i=i->next)
      if(!i->connected)
        if((number(0,9) < 3)&&(GET_LEVEL(i->character) < IMO))
          return(i->character);
  }
  return(0);
}
void bombtosser(struct char_data *ch)
{
  struct obj_data *i, *inext;
  int d,id,rd;

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    inext = i->next_content;
    if(GET_ITEM_TYPE(i)==ITEM_BOMB){
      id=number(0,5);
      for(d=0;d<6;d++){
        rd=(d+id)%6;
        if(CAN_GO(ch,rd)){
          obj_from_room(i);
          obj_to_char(i,ch);
          act("$n lifts the $o.",FALSE,ch,i,0,TO_ROOM);
          throw(ch,i,rd);
          return;
        }
      }
      if(number(1,7)==6){
        defuser(ch,0,0);
        return;
      }
    }
  }
}
void loadhelper(struct char_data *ch, int room)
{
  int num,vn,loadroom;
  struct char_data *tmp, *mob;

  tmp=ch->specials.fighting;
  if(!tmp)
    return;
  switch(mob_index[ch->nr].virtual){
    case  1536: vn=3159; break;
    case  2120: vn=2123; break;
    case  3160: vn=number(3158,3159); break;
    case 12020: vn=12021; break;
    case 16504: vn=16506; break;
    case 16507: vn=number(16508,16509); break;
    case 16510: vn=number(16513,16515); break;
    case 16610: vn=16611; break;
    default:    vn=defaulthelper; break;
  }
  num = real_mobile(vn);
  loadroom = (room >= 0) ? room : ch->in_room;
  if(num > 0){
    mob=read_mobile(num,REAL);
    if(mob){
      char_to_room(mob,loadroom);
      act("$n has arrived.", TRUE, mob, 0, 0, TO_ROOM);
      hit(mob,tmp,TYPE_UNDEFINED);
    }
  }
}
void teleport(struct char_data *ch, int to_room)
{
  if(!to_room){
    do {
      to_room = number(0,top_of_world-1000);
    } while (IS_SET(world[to_room].room_flags, PRIVATE|NORELOCATE|NO_MAGIC));
  } else {
    if(IS_SET(world[to_room].room_flags, PRIVATE | NORELOCATE))
      return;
  }
  act("$n slowly fades out of existence.", FALSE, ch,0,0,TO_ROOM);
  char_from_room(ch);
  char_to_room(ch,to_room);
  act("$n slowly fades into existence.",FALSE, ch,0,0,TO_ROOM);
}
void damage_equips(struct char_data *ch, struct char_data *vict)
{
  int i,j,k;
  struct obj_data *o;
  char buf[MAX_STRING_LENGTH];

  if(ch->in_room != vict->in_room)
    return;
  o=vict->equipment[j=number(1,MAX_WEAR-1)];
  if(o){
    k=(j==WIELD) ? 1 : 0;
    for(i=k;i<3;i++){
      if(o->obj_flags.value[i] > 0){
        if(GET_ITEM_TYPE(o) == ITEM_ARMOR){
          if(IS_SET(o->obj_flags.extra_flags,ITEM_HUM)){
            REMOVE_BIT(o->obj_flags.extra_flags,ITEM_HUM);
            sprintf(buf,"$n kicks %s, CLINK!",o->short_description);
            act(buf,TRUE,ch,0,vict,TO_ROOM);
            return;
          } else {
            obj_to_char(unequip_char(vict,j),vict);
            --(o->obj_flags.value[i]);
            sprintf(buf,"$n kicks %s, CLANK!",o->short_description);
            act("$n kicks $o.",TRUE,ch,o,vict,TO_ROOM);
            return;
          }
        }
      }
    }
  }
}

static int item_for_level[]={
  1707, 1313,1307,2124,1314,1306,1305,3150,1312,3171,
  3170,16606,9630,2860,9190,9191,9192,9193,9194,9195,
  9196, 9197,9198,9199,0
};

int check_for_corpse(struct char_data *ch)
{
  struct obj_data *o;
  int lev;

  lev=GET_LEVEL(ch)+1;
  if(lev <= 1000) return(0);
  if(lev > IMO) return(0);
  for(o=ch->carrying;o;o=o->next_content){
    if((GET_ITEM_TYPE(o)==ITEM_CONTAINER)&&
       (o->obj_flags.value[2]==item_for_level[lev-1001])&&
       (o->obj_flags.value[3])){
      extract_obj(o);
      return(1);
    }
  }
  return(0);
}

int guild(struct char_data *ch, int cmd, char *arg)
{
  char arg1[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH],buf2[128];
  int flag,num,i,j,percent,lev,powerpts;
  extern char *spells[];
  extern struct spell_info_type spell_info[MAX_SPL_LIST];

  if ((cmd!=95)&&(cmd!=164)&&(cmd!=170)) return(FALSE);
  lev=GET_LEVEL(ch);
  if(cmd==95){ /* advance */
    if (!IS_NPC(ch)) {
      if(lev >= IMO){
        send_to_char("You are immortal enough.\n\r",ch);
        return(TRUE);
      } else {
        if(GET_EXP(ch) >= XP4LEV(lev+1)){
          send_to_char("You raise a level\n\r", ch);
          GET_LEVEL(ch)++; advance_level(ch); set_title(ch);
          GET_EXP(ch) -= XP4LEV(lev);
        } else {
          send_to_char("You lack the experience.\n\r",ch);
        }
        return(TRUE);
      }
    }
  }
  for(; *arg==' '; arg++);
  if (!*arg) {
    sprintf(buf,"You have %d practices left.\n\r",
      ch->specials.spells_to_learn);
    send_to_char(buf, ch);
    strcpy(buf,"You can practice any of these skills:\n\r");
    for(i=j=0; *spells[i] != '\n'; i++){
      if(*spells[i] && (spell_info[i+1].min_level <= lev)) {
        sprintf(buf2,"%-25s%4d%s",spells[i],
          ch->skills[i+1].learned,
          (j) ? "\n\r" : " | ");
        strcat(buf,buf2);
        j=1-j;
      }
    }
    if(j) strcat(buf,"\n\r");
    send_to_char(buf, ch);
    return(TRUE);
  }
  num = old_search_block(arg,0,strlen(arg),spells,FALSE);
  if(num == -1) {
    send_to_char("You do not know of this spell...\n\r", ch);
    return(TRUE);
  }
  if (lev < spell_info[num].min_level)
    return(TRUE);
  if (ch->specials.spells_to_learn <= 0) {
    send_to_char("You do not seem to be able to practice now.\n\r", ch);
    return(TRUE);
  }
  if(ch->skills[num].learned >= spell_info[num].max_skill){
    send_to_char("You know this area as well as possible.\n\r",ch);
    return(TRUE);
  }
  if((ch->skills[num].learned > 0) && (ch->skills[num].used == 0)){
    sprintf(buf,
      "You need to USE it successfully before you can practice any more.\n\r");
    send_to_char(buf,ch);
    return(TRUE);
  }
  ch->skills[num].used = 0;
  send_to_char("You Practice for a while...\n\r", ch);
  ch->specials.spells_to_learn--;
  percent=ch->skills[num].learned+number(1,GET_INT(ch));
  ch->skills[num].learned =
    MIN(spell_info[num].max_skill,percent);
  sprintf(buf,"Your skill level is now %d.\n\r",ch->skills[num].learned);
  send_to_char(buf,ch);
  if(ch->skills[num].learned >= spell_info[num].max_skill){
    send_to_char("You're now as proficient as possible.\n\r",ch);
    return(TRUE);
  }
}
int fitness(struct char_data *ch, int cmd, char *arg)
{
  char buf[MAX_STRING_LENGTH], rem[MAX_STRING_LENGTH];
  int f,k,opt,reps,sp,succs,fails,prob;

  if(IS_NPC(ch))
    return(FALSE);
  if(cmd != 164) return(FALSE);
  if (! *arg) {
    send_to_char(" Your practice can cover any of the following:\n\r",ch);
    send_to_char("1 - Strength\n\r2 - Intelligence\n\r3 - Wisdom\n\r",ch);
    send_to_char("4 - Dexterity\n\r5 - Constitution\n\r",ch);
    sprintf(buf,"You have %d practices left.\n\r",
      ch->specials.spells_to_learn);
    send_to_char(buf, ch);
  } else {
    if(ch->specials.spells_to_learn<=0){
      send_to_char("You are out of practices.\n\r",ch);
      return(TRUE);
    }
    half_chop(arg, buf, rem);
    opt = atoi(buf);
    if((opt < 1) || (opt > 5)){
      send_to_char("What?\n\r",ch);
      return(TRUE);
    }
    if(!rem[0])
      reps=1;
    else
      reps=atoi(rem);
    if((reps <= 0)||(reps > ch->specials.spells_to_learn)){
      send_to_char("I don't think so...\n\r",ch);
      return(TRUE);
    }
    succs=fails=0;
    sp = IS_AFFECTED(ch,AFF_POISON);
    while(reps > 0){
      --reps;
      ch->specials.spells_to_learn--;
      prob = 1 + (MAX_STAT/(1+abs(GET_INT(ch))));
      if(prob < 1)
        prob = 1;
      switch(opt){
       case  1: if((sp || (prob < number(0,1000)))){
                  fails++; break;
                }
                INC(ch->abilities.str,ch); succs++;
                send_to_char("You feel stronger?\n\r",ch); break;
       case  2: if((prob < number(0,1000))){
                  fails++; break;
                }
                INC(ch->abilities.intel,ch); succs++;
                send_to_char("You feel smarter?\n\r",ch); break;
       case  3: if((prob < number(0,1000))){
                  fails++; break;
                }
                INC(ch->abilities.wis,ch); succs++;
                send_to_char("You feel wiser?\n\r",ch); break;
       case  4: if((prob < number(0,1000))){
                  fails++; break;
                }
                INC(ch->abilities.dex,ch); succs++;
                send_to_char("You feel more nimble?\n\r",ch); break;
       case  5: if((prob < number(0,1000))){
                  fails++; break;
                }
                INC(ch->abilities.con,ch); succs++;
                send_to_char("You feel healthier?\n\r",ch); break;
      }
    }
    ch->tmpabilities = ch->abilities;
    if((succs==0) && (fails == 1))
      sprintf(buf,"You don't feel enlightened.\n\r");
    else if(fails > 0)
      sprintf(buf,"You didn't feel enlightened %d times.\n\r",fails);
    send_to_char(buf,ch);
  }
  return(TRUE);
}

int dump(struct char_data *ch, int cmd, char *arg) 
{
  struct obj_data *k;
  char buf[100];
  struct char_data *tmp;
  int value=0;
  char *fname(char *namelist);

  for(k = world[ch->in_room].contents; k ; k = world[ch->in_room].contents) {
    sprintf(buf, "The %s vanish in a puff of smoke.\n\r" ,fname(k->name));
    for(tmp=world[ch->in_room].people;tmp;tmp=tmp->next_in_room)
      if (CAN_SEE_OBJ(tmp, k))
        send_to_char(buf,tmp);
    extract_obj(k);
  }
  if(cmd!=60) return(FALSE);
  do_drop(ch, arg, cmd);
  value = 0;
  for(k=world[ch->in_room].contents; k ; k = world[ch->in_room].contents) {
    sprintf(buf, "The %s vanish in a puff of smoke.\n\r",fname(k->name));
    for(tmp = world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
      if (CAN_SEE_OBJ(tmp, k))
        send_to_char(buf,tmp);
      value += MAX(1, MIN(50, k->obj_flags.cost/10));
    extract_obj(k);
  }
  if (value) {
    act("You are rewarded for outstanding performance.",FALSE,ch,0,0,TO_CHAR);
    act("$n has been rewarded for being a good citizen.",TRUE,ch,0,0,TO_ROOM);
    if(GET_LEVEL(ch) < 10)
      gain_exp(ch, value);
  }
}

int mayor(struct char_data *ch, int cmd, char *arg)
{
  static char open_path[] =
    "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
  static char close_path[] =
    "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";
  static char *path;
  static int index;
  static bool move = FALSE;


  if (!move) {
    if (time_info.hours == 6) {
      move = TRUE;
      path = open_path;
      index = 0;
    } else if (time_info.hours == 20) {
      move = TRUE;
      path = close_path;
      index = 0;
    }
  }

  if (cmd || !move || (GET_POS(ch) < POSITION_SLEEPING) ||
    (GET_POS(ch) == POSITION_FIGHTING))
    return FALSE;

  switch (path[index]) {
    case '0' :
    case '1' :
    case '2' :
    case '3' :
      do_move(ch,"",path[index]-'0'+1);
      break;
    case 'W' :
      GET_POS(ch) = POSITION_STANDING;
      act("$n awakens and groans loudly.",FALSE,ch,0,0,TO_ROOM);
      break;
    case 'S' :
      GET_POS(ch) = POSITION_SLEEPING;
      act("$n lies down and instantly falls asleep.",FALSE,ch,0,0,TO_ROOM);
      break;
    case 'a' :
      act("$n says 'Hello Honey!'",FALSE,ch,0,0,TO_ROOM);
      act("$n smirks.",FALSE,ch,0,0,TO_ROOM);
      break;
    case 'b' :
      act("$n says 'What a view! I must get something done about that dump!'",
        FALSE,ch,0,0,TO_ROOM);
      break;
    case 'c' :
      act("$n says 'Vandals! Youngsters today have no respect for anything!'",
        FALSE,ch,0,0,TO_ROOM);
      break;
    case 'd' :
      act("$n says 'Good day, citizens!'", FALSE, ch, 0,0,TO_ROOM);
      break;
    case 'e' :
      act("$n says 'I hereby declare the bazaar open!'",FALSE,ch,0,0,TO_ROOM);
      break;
    case 'E' :
      act("$n says 'I hereby declare Midgaard closed!'",FALSE,ch,0,0,TO_ROOM);
      break;
    case 'O' :
      do_unlock(ch, "gate", 0);
      do_open(ch, "gate", 0);
      break;
    case 'C' :
      do_close(ch, "gate", 0);
      do_lock(ch, "gate", 0);
      break;
    case '.' :
      move = FALSE;
      break;
  }
  index++;
  return FALSE;
}

void npc_steal_coins(struct char_data *ch,struct char_data *victim)
{
  int dir,gold;

  if(IS_NPC(victim)) return;
  if(!victim->desc) return;
  if(GET_LEVEL(victim) >= IMO) return;
  if(AWAKE(victim) && (number(0,GET_LEVEL(ch)) == 0)) {
    act("You discover that $n has $s hands in your wallet.",
      FALSE,ch,0,victim,TO_VICT);
    act("$n tries to steal gold from $N.",TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    act("$n suppresses a laugh.",TRUE,ch,0,0,TO_NOTVICT);
    gold =(int)((GET_GOLD(victim)*number(1,10))/25);
    if(gold > 0){
      GET_GOLD(ch) += gold;
      GET_GOLD(victim) -= gold;
      dir=number(0,5);
      if(CAN_GO(ch,dir))
        do_simple_move(ch,dir,FALSE);
    }
  }
}
void npc_steal_obj(struct char_data *ch,struct char_data *victim, char *oname)
{
  struct obj_data *obj;

  if(IS_SET(world[ch->in_room].room_flags,LAWFUL))
    return;
  if(IS_NPC(victim)) return;
  if(!victim->desc) return;
  if(GET_LEVEL(victim) >= IMO) return;
  if(AWAKE(victim) && (number(0,GET_LEVEL(ch)) == 0)) {
    act("You discover that $n tried to rob you.",
      FALSE,ch,0,victim,TO_VICT);
    act("$n tries to rob $N.",TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    if(obj = get_obj_in_list_vis(victim, oname, victim->carrying)){
      obj_from_char(obj);
      obj_to_char(obj, ch);
    } else
      act("$n farts, quietly.",TRUE,ch,0,0,TO_NOTVICT);
  }
}

int snake(struct char_data *ch, int cmd, char *arg)
{
  if(cmd) return FALSE;
  if(GET_POS(ch)!=POSITION_FIGHTING) return FALSE;
  if(ch->specials.fighting&&(ch->specials.fighting->in_room==ch->in_room)&&
    (number(0,1000) < GET_LEVEL(ch)))
    {
      act("$n bites $N!", 1, ch, 0, ch->specials.fighting, TO_NOTVICT);
      act("$n bites you!", 1, ch, 0, ch->specials.fighting, TO_VICT);
      cast_poison(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL,
         ch->specials.fighting, 0);
      return TRUE;
    }
  return FALSE;
}

int thief(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *cons;
  int n, gold;

  if(cmd==84){ /* CAST */
    if(!IS_NPC(ch))
    for(cons=world[ch->in_room].people; cons; cons = cons->next_in_room)
      if(IS_NPC(cons))
        if(mob_index[cons->nr].func==thief){
          if(GET_LEVEL(cons)!=366){
            do_flee(cons,"",0);
            return FALSE;
          } else {
            gold = GET_GOLD(ch)/10;
            GET_GOLD(ch) -= gold;
            GET_GOLD(cons) += gold;
            for(n=0;n<6;n++){
              if(CAN_GO(cons,n)){
                do_simple_move(cons,n,FALSE);
              }
            }
            return TRUE;
          }
        }
  }
  if(cmd)
    return FALSE;
  if(IS_SET(world[ch->in_room].room_flags,LAWFUL))
    return(FALSE);
  if(IS_AFFECTED(ch,AFF_CHARM))
    affect_from_char(ch,SPELL_CHARM_PERSON);
  if(GET_POS(ch)!=POSITION_STANDING) return FALSE;
  for(cons = world[ch->in_room].people; cons; cons = cons->next_in_room )
    if((!IS_NPC(cons)) && (GET_LEVEL(cons)<IMO) &&
      (GET_LEVEL(cons)>=GET_LEVEL(ch)) && (number(1,3)==1)){
      switch(number(0,9)){
       case 0:
       case 1:
       case 2:
       case 3:
       case 4: npc_steal_coins(ch,cons); break;
       case 5: npc_steal_obj(ch,cons,"bag"); break;
       case 6: npc_steal_obj(ch,cons,"scroll"); break;
       case 7: npc_steal_obj(ch,cons,"potion"); break;
       case 8: npc_steal_obj(ch,cons,"wand"); break;
       case 9: npc_steal_obj(ch,cons,"staff"); break;
      }
    }
  if((GET_LEVEL(ch) > 200)&&(GET_GOLD(ch) > 5000000)){
    teleport(ch,0);
  }
  return TRUE;
}

int sleeper(struct char_data *ch, int cmd, char *arg)
{
  void cast_sleep(short level, struct char_data *ch, char *arg, int type,
    struct char_data *tar_ch, struct obj_data *tar_obj );
  struct char_data *vict;

  if(cmd)
    return(FALSE);
  if(GET_POS(ch) > POSITION_FIGHTING){
    for(vict=world[ch->in_room].people; vict; vict = vict->next_in_room){
      if((!IS_NPC(vict))&&(GET_LEVEL(vict) < IMO)){
        act("$n utters the words 'La la la la...'",1,ch,0,0,TO_ROOM);
        cast_sleep(number(0,4) ? GET_LEVEL(ch) : IMO+1,
            ch, "", SPELL_TYPE_SPELL, vict, 0);
      }
    }
  } else {
    if(number(0,2)){
      act("$n utters the words 'CURE ME!'",1,ch,0,0,TO_ROOM);
      cast_cure_critic(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,ch,0);
      return(FALSE);
    }
    for(vict=world[ch->in_room].people; vict; vict = vict->next_in_room)
      if(IS_NPC(vict) && (GET_LEVEL(vict) < 21))
        do_flee(vict,0,0);
  }
  return(FALSE);
}
int charmer(struct char_data *ch, int cmd, char *arg)
{
  void cast_charm_person(short level,struct char_data *ch,char *arg,int type,
    struct char_data *tar_ch, struct obj_data *tar_obj );
  struct char_data *vict;
  int n;

  if(cmd)
    return(FALSE);
  if(GET_POS(ch)!=POSITION_FIGHTING){
    if(number(0,2)!=1)
      return(FALSE);
    n=0;
    for(vict=world[ch->in_room].people;vict;vict=vict->next_in_room)
      if((!IS_NPC(vict))&&(GET_LEVEL(vict) < IMO)){
        ++n;
        act("$n utters the words 'Flibbidy Jibbit'.",1,ch,0,0,TO_ROOM);
        cast_charm_person(number(0,7) ? GET_LEVEL(ch) : IMO+1,
          ch, "", SPELL_TYPE_SPELL, vict, 0);
        return(TRUE);
      }
    if(n==0){
      extract_char(ch);
      return(TRUE);
    }
    return FALSE;
  }
}
int magic_user(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *tmp,*tmp2,*vict;
  int minhp=0x7fffffff,num;
  int l1,l2;

  if(cmd)
    return FALSE;
  if(!ch->specials.fighting)
    return FALSE;
  if(GET_POS(ch) < POSITION_FIGHTING){
    do_stand(ch,"",0);
    if(number(1,100) < 33)
      return(FALSE);
  }
  if(number(1,7) < 4){
    vict=ch->specials.fighting;
  } else {
    vict=0;
    for(tmp=world[ch->in_room].people; tmp; tmp = tmp->next_in_room )
     if(!IS_NPC(tmp)){
      if((tmp->specials.fighting==ch) && (GET_HIT(tmp) < minhp)){
       minhp=GET_HIT(tmp);
       vict=tmp;
      }
     }
  }
  spellcast(ch,vict,GET_LEVEL(ch));
}
void spellcast(struct char_data *ch,struct char_data *vict, int lev)
{
  if(!vict)
    return;
  if(lev < 5){
      act("$n utters the words 'hahili duvini'.", 1, ch, 0, 0, TO_ROOM);
      cast_magic_missile(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
  } else if(lev < 10){
      act("$n utters the words 'vivani yatima'.",1,ch,0,0,TO_ROOM);
      cast_burning_hands(GET_LEVEL(ch), ch,"",SPELL_TYPE_SPELL,vict,0);
  } else if(lev < 15){
      act("$n utters the words 'ekelector zan'.", 1, ch, 0, 0, TO_ROOM);
      cast_lightning_bolt(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
  } else if(lev < 25){
      act("$n utters the words 'yibbot rym'.", 1, ch, 0, 0, TO_ROOM);
      cast_blindness(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
  } else if(lev < 40){
      act("$n utters the words 'nasson hof'.", 1, ch, 0, 0, TO_ROOM);
      cast_colour_spray(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
  } else if(lev < 90){
      act("$n utters the words 'tubu morg'.",1,ch,0,0,TO_ROOM);
      cast_fireball(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
  } else if(lev < 110) {
    act("$n utters the words 'oliel ese'.", 1, ch, 0, 0, TO_ROOM);
    cast_sunburst(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
  } else if(lev < 200) {
    act("$n utters the words 'zzi zzl'.", 1, ch, 0, 0, TO_ROOM);
    cast_radiation(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
  } else if(lev < 300) {
    act("$n utters the words 'yuck-a-muck'.", 1, ch, 0, 0, TO_ROOM);
    cast_slime(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
  } else if(lev < 900){
    switch(number(0,2)){
      case 0:
    act("$n utters the words 'seven eleven rose'.", 1, ch, 0, 0, TO_ROOM);
    cast_slime(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
    cast_cyclone(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
    break;
      case 1:
    act("$n utters the words 'pada wada mada'.", 1, ch, 0, 0, TO_ROOM);
    cast_gas(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
    cast_radiation(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
    break;
      case 2:
    act("$n utters the words 'mister gold star'.", 1, ch, 0, 0, TO_ROOM);
    cast_radiation(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
    break;
      case 3:
    act("$n utters the word 'bleh'.", 1, ch, 0, 0, TO_ROOM);
    cast_doppelganger(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
    break;
      case 4:
    cast_harm(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
    }
  } else {
    cast_nova(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
  }
  if(!(time(0) & 0xf))
    cast_stupidity(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
  cast_cure_critic(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,ch,0);
}
int lolth(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *vict;

  if(cmd)
    return(FALSE);
  if(!(vict=ch->specials.fighting))
    return(FALSE);
  if(vict->in_room != ch->in_room)
    spell_heal(GET_LEVEL(ch),ch,ch,0);
  act("$n utters the words 'Praise Lolth!'.", 1, ch, 0, 0, TO_ROOM);
  cast_radiation(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
  cast_poison(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
  cast_earthquake(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
  return(TRUE);
}
int big_dragon(struct char_data *ch, int cmd, char *arg)
{
  int mh;
  struct char_data *vict,*tmp;
  void cast_fire_breath( short level, struct char_data *ch, char *arg,int type,
    struct char_data *tar_ch, struct obj_data *tar_obj );
  void cast_frost_breath( short level, struct char_data *ch, char *arg,int type,
    struct char_data *tar_ch, struct obj_data *tar_obj );
  void cast_gas_breath(short level, struct char_data *ch,char *arg,int type,
    struct char_data *tar_ch, struct obj_data *tar_obj );
  void cast_lightning_breath(short level,struct char_data *ch,char *arg,
   int type, struct char_data *tar_ch, struct obj_data *tar_obj );

  if((cmd==156)&&(GET_LEVEL(ch) < IMO)){
    WAIT_STATE(ch,10*PULSE_VIOLENCE);
    return(TRUE);
  }
  if(cmd == 9)  /* backstab */
    for(tmp=world[ch->in_room].people ; tmp ; tmp=tmp->next_in_room)
      if(IS_SET(tmp->specials.act, ACT_SPEC))
        if(mob_index[tmp->nr].func==big_dragon){
          act("$n utters the words 'qassir plaffa'.", 1,tmp, 0, 0, TO_ROOM);
          cast_fire_breath(GET_LEVEL(tmp),tmp, "", SPELL_TYPE_SPELL, ch, 0);
          return FALSE;
        }
  if(cmd) return FALSE;
  defuser(ch,0,0);
  vict=ch->specials.fighting;
  if(!vict){
    do_get(ch,"corpse",0);
    return FALSE;
  }
  if(IS_NPC(vict)){
    stop_fighting(vict);
    stop_fighting(ch);
  } else {
    if(vict->specials.recall_room)
      vict->specials.recall_room=ch->in_room;
  }
  mh=0x7fff;
  vict=0;
  for(tmp=world[ch->in_room].people; tmp; tmp = tmp->next_in_room )
   if(!IS_NPC(tmp))
    if((tmp->specials.fighting==ch) && (GET_HIT(tmp) < mh)){
     mh=GET_HIT(tmp);
     vict=tmp;
    }
  if(!(vict=ch->specials.fighting)) return FALSE;
  damage_equips(ch,vict);
  act("$n utters the words 'qassir porolo'.", 1, ch, 0, 0, TO_ROOM);
  cast_gas_breath(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
  if(!(vict=ch->specials.fighting)) return FALSE;
  act("$n utters the words 'qassir moolim'.", 1, ch, 0, 0, TO_ROOM);
  cast_frost_breath(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
  cast_extremelyburlyheal(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0);
  if(!(vict=ch->specials.fighting)) return FALSE;
  act("$n utters the words 'qassir relata'.", 1, ch, 0, 0, TO_ROOM);
  cast_lightning_breath(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
  if(!(vict=ch->specials.fighting)) return FALSE;
  act("$n utters the words 'qassir plaffa'.", 1, ch, 0, 0, TO_ROOM);
  cast_fire_breath(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
  return TRUE;
}

int little_dragon(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *vict,*tmp;
  void cast_fire_breath( short level, struct char_data *ch, char *arg,int type,
    struct char_data *tar_ch, struct obj_data *tar_obj );

  if(cmd == 9)  /* backstab */
    for(tmp=world[ch->in_room].people ; tmp ; tmp=tmp->next_in_room)
      if(IS_SET(tmp->specials.act, ACT_SPEC))
        if(mob_index[tmp->nr].func==little_dragon){
          if(number(1,IMO+10) < GET_LEVEL(ch))
            return(FALSE);
          act("$n utters the words 'qassir plaffa'.", 1,tmp, 0, 0, TO_ROOM);
          cast_fire_breath(GET_LEVEL(tmp),tmp, "", SPELL_TYPE_SPELL, ch, 0);
          return FALSE;
        }
  if(cmd) return FALSE;
  vict=ch->specials.fighting;
  if(!vict) return FALSE;
  act("$n utters the words 'qassir plaffa'.", 1, ch, 0, 0, TO_ROOM);
  cast_fire_breath(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
  return TRUE;
}

/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

int rooster(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *tmp;

  if (cmd)
    return(0);
  switch (number(0,300)) {
    case 0:
      do_shout(ch, "Cock-a-doodle-doo!!", 0);
      return(1);
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      for(tmp=world[ch->in_room].people; tmp; tmp = tmp->next_in_room )
        if(!IS_NPC(tmp))
          if(GET_POS(tmp)==POSITION_SLEEPING){
            if(IS_AFFECTED(tmp,AFF_SLEEP))
              affect_from_char(tmp,SPELL_SLEEP);
            do_wake(tmp,"",0);
            do_say(ch,"Cock-a-doodle-doo!!", 0);
          }
      return(1);
    default:
      return(0);
  }
}

int mom(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *vict,*tmp;
  int h,n,num;
  char buf[KBYTE];
  struct descriptor_data *d;
  static char warn_flag=0, restore_flag=0;
  static int talking=0,online=0,qnum=0;
  extern int shutdownwarn, top_of_objt;
  extern struct momtalktype **momtalk;
  extern int momtct;
  extern int momsreadflag;

  if(cmd)
    return(0);
  if(shutdownwarn){
    warn_flag++;
    if((warn_flag==14)||(shutdownwarn < 100)){
      warn_flag=0;
      num=shutdownwarn/10;
      h=num%3600;
      num=num/3600;
      sprintf(buf,"Shutdown/Reboot in %d hours, %d minutes and %d seconds.",
        num,h/60,h%60);
      do_talk(ch,buf,0);
      if(!restore_flag && !num && (h < 15)){
        restore_flag = 1;
        for(d=descriptor_list;d;d=d->next) {
          if(!d->original  && (d->connected == CON_PLYNG)){
            vict = d->character;
            if(GET_LEVEL(vict) < IMO){
              GET_MANA(vict) = GET_MAX_MANA(vict);
              GET_HIT(vict)  = GET_MAX_HIT(vict);
              GET_MOVE(vict) = GET_MAX_MOVE(vict);
              update_pos(vict);
              act("You have been restored by $N!",FALSE,vict,0,ch,TO_CHAR);
            }
          }
        }
      }
    }
  } else {
    if(GET_HIT(ch) < GET_MAX_HIT(ch))
      GET_HIT(ch) = GET_MAX_HIT(ch);
    bombtosser(ch);
  }
  if(!momsreadflag)
    read_mom();
  if(!momtct){
    switch(number(0,99)){
      case 0:
      do_talk(ch, "Where the hell is my data file!?!?.", 0);
    }
  } else if(talking){
    do_shout(ch,momtalk[qnum]->line[online++],0);
    if(online >= momtalk[qnum]->n)
      talking=0;
  } else {
    qnum=number(0,49999);
    if(qnum < momtct){
      do_shout(ch,momtalk[qnum]->line[0],0);
      if(momtalk[qnum]->n > 1){
        talking = 1;
        online = 1;
      }
    }
  }
  return TRUE;
}

int fido(struct char_data *ch, int cmd, char *arg)
{

  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return(FALSE);
  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (GET_ITEM_TYPE(i)==ITEM_CONTAINER && i->obj_flags.value[3]) {
      act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
      for(temp = i->contains; temp; temp=next_obj) {
        next_obj = temp->next_content;
        obj_from_obj(temp);
        obj_to_room(temp,ch->in_room);
      }
      extract_obj(i);
      return(TRUE);
    }
  }
  return(FALSE);
}

int janitor(struct char_data *ch, int cmd, char *arg)
{
  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return(FALSE);
  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if(IS_SET(i->obj_flags.wear_flags, ITEM_TAKE)){
      act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
      obj_from_room(i);
      extract_obj(i);
      return(TRUE);
    }
  }
  return(FALSE);
}
int cityguard(struct char_data *ch, int cmd, char *arg)
{
  int shop_keeper(struct char_data *ch, int cmd, char *arg);
  struct char_data *tch, *evil, *tmp;
  int max_evil, i, newroom;

  if(cmd || !AWAKE(ch))
    return (FALSE);
  max_evil = 1001;
  evil = 0;
  for(tch=world[ch->in_room].people; tch; tch = tch->next_in_room) {
    tmp = tch->specials.fighting;
    if(tmp && !IS_NPC(tch)){
      if(GET_ALIGNMENT(tch) < max_evil){
        max_evil = GET_ALIGNMENT(tch);
        evil = tch;
      }
    }
  }
  if (evil && (GET_ALIGNMENT(evil->specials.fighting) >= 0)) {
    act("$n screams 'PROTECT THE INNOCENT!'",FALSE,ch,0,0,TO_ROOM);
    hit(ch, evil, TYPE_UNDEFINED);
    return(TRUE);
  }
  return(FALSE);
}
int levelguard(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *tch, *big;
  int maxlevel, t;

  if(cmd || !AWAKE(ch))
    return (FALSE);
  maxlevel = -1; 
  big = 0;
  for(tch=world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if(!IS_NPC(tch))
      if(tch->specials.fighting) {
        t = GET_LEVEL(tch);
        if(t > maxlevel){
          maxlevel = t;
          big = tch;
        }
      }
  }
  if(big){
    act("$n screams 'PROTECT THE MEEK!'",FALSE,ch,0,0,TO_ROOM);
    hit(ch, big, TYPE_UNDEFINED);
    return(TRUE);
  }
  return(FALSE);
}
int hunter(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *tch;
  int i, newroom;

  if(cmd)
    return(FALSE);
  if(GET_POS(ch)!=POSITION_STANDING)
    return(FALSE);
  if(number(1,10) < 5)
    return(FALSE);
  for(i=0;i<6;++i){
    if(CAN_GO(ch,i)){
      newroom=world[ch->in_room].dir_option[i]->to_room;
      for(tch=world[newroom].people;tch;tch=tch->next_in_room){
        if(GET_POS(tch) == POSITION_FIGHTING){
          do_simple_move(ch,i,FALSE);
          return(TRUE);
        }
      }
    }
  }
  return(FALSE);
}

int pet_shops(struct char_data *ch, int cmd, char *arg)
{
  char buf[MAX_STRING_LENGTH], pet_name[256];
  int pet_room,k;
  struct char_data *pet;
  struct follow_type *j;

  if(IS_NPC(ch))
    return(FALSE);
  pet_room = ch->in_room+1;
  if (cmd==59) { /* List */
    send_to_char("Available pets are:\n\r", ch);
    for(pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      sprintf(buf, "%8d - %s\n\r",1000*GET_LEVEL(pet),pet->player.short_descr);
      send_to_char(buf, ch);
    }
    return(TRUE);
  } else if (cmd==56) { /* Buy */
    arg = one_argument(arg, buf);
    arg = one_argument(arg, pet_name);
    /* Pet_Name is for later use when I feel like it */
    if (!(pet = get_char_room(buf, pet_room))) {
      send_to_char("There is no such pet!\n\r", ch);
      return(TRUE);
    }
    for(k=0,j=ch->followers;(j)&&(k<5);++k){
      j=j->next;
    }
    if(k>=3){
      send_to_char("You already have a full load of pets.\n\r",ch);
      return(TRUE);
    }
    if (GET_GOLD(ch) < (GET_LEVEL(pet)*1000)) {
      send_to_char("You don't have enough gold!\n\r", ch);
      return(TRUE);
    }
    GET_GOLD(ch) -= GET_LEVEL(pet)*1000;
    pet = read_mobile(pet->nr, REAL);
    GET_EXP(pet) = 0;
    SET_BIT(pet->specials.affected_by, AFF_CHARM);
    if (*pet_name) {
      sprintf(buf,"%s %s", pet->player.name, pet_name);
      free(pet->player.name);
      pet->player.name = strdup(buf);    
    }
    char_to_room(pet, ch->in_room);
    add_follower(pet, ch);
    send_to_char("May you enjoy your pet.\n\r", ch);
    act("$n bought $N as a pet.",FALSE,ch,0,pet,TO_ROOM);
    return(TRUE);
  }

  /* All commands except list and buy */
  return(FALSE);
}
int hospital(struct char_data *ch, int cmd, char *arg)
{
  char buf[MAX_STRING_LENGTH];
  int opt,lev,cost;

  lev = GET_LEVEL(ch);
  cost = 30 * lev * lev;
  if (cmd==59) { /* List */
    send_to_char("1 - Hit points restoration\n\r", ch);
    send_to_char("2 - Mana restoration\n\r",ch);
    send_to_char("3 - Poison cured\n\r",ch);
    sprintf(buf,"Your cost for any of these services is %d coins.\n\r",cost);
    send_to_char(buf,ch);
    return(TRUE);
  } else if (cmd==56) { /* Buy */
    arg = one_argument(arg, buf);
    opt = atoi(buf);
    if(cost > GET_GOLD(ch)){
       send_to_char("Get a few more coins and come on back.\n\r",ch);
       return(TRUE);
    }
    switch(opt){
      case 1:
        GET_HIT(ch) = number(GET_HIT(ch),GET_MAX_HIT(ch));
        GET_GOLD(ch) -= cost;
        send_to_char("You feel magnificent!\n\r",ch);
        return TRUE;
      case 2:
        GET_MANA(ch) = number(GET_MANA(ch),GET_MAX_MANA(ch));
        GET_GOLD(ch) -= cost;
        send_to_char("You feel marvelous!\n\r",ch);
        return TRUE;
      case 3:
        if(affected_by_spell(ch,SPELL_POISON)){
           affect_from_char(ch,SPELL_POISON);
           GET_GOLD(ch) -= cost;
           send_to_char("You feel stupendous!\n\r",ch);
        } else {
           send_to_char("Nothing wrong with you.\n\r",ch);
        }
        return TRUE;
      default:
        send_to_char("Huh?\n\r",ch);
        return(TRUE);
    }
  }
  return(FALSE);
}

#define COST 1000000

char *metasign[]={
    "@>->->--",
    "<,,,,,,,>",
    "<>-<>-<>",
    "Your stomach rumbles.",
    "You just wasted one meta point.",
    "<-------->"
};
void do_meta(struct char_data *ch, struct char_data *meta, int opt)
{
  int k;

  switch(opt){
   case  1: k=ch->points.max_hit;
            ch->points.max_hit+=1+GET_META_BONUS(ch)+
              (k<1000)+(k<10000)+(k<100000);
            break;
   case  2: k=ch->points.max_mana;
            ch->points.max_mana+=1+GET_META_BONUS(ch)+
              (k<1000)+(k<10000)+(k<100000);
            break;
   case  3: k=ch->points.max_move;
            ch->points.max_move+=1+GET_META_BONUS(ch)+
              (k<1000)+(k<10000)+(k<100000);
            break;
   case  4: GET_GUT(ch)++;
            break;
   case  5: 
            break;
   case  6: ch->specials.spells_to_learn+=number(1,3);
            break;
   case  7: ch->specials.conditions[1]=0;
            break;
   case  8: ch->specials.conditions[2]=0;
            break;
  }
}
int metaphysician(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *meta, *vict;
  struct obj_data *tok;
  char buf[MAX_STRING_LENGTH],rem[MAX_STRING_LENGTH];
  int i,k,opt,cost,reps;

  if(!cmd){
    if(vict=ch->specials.fighting)
      vict->specials.recall_room=ch->in_room;
    return(FALSE);
  }
  if (IS_NPC(ch))
    return(FALSE);
  for(meta=world[ch->in_room].people;meta;meta=meta->next_in_room){
    if(!IS_NPC(meta))
      continue;
    if(mob_index[meta->nr].func==metaphysician)
      break;
  }
  if(!meta)
    return(FALSE);;
  if (cmd==59) { /* List */
    send_to_char(" 1 - Hit points inflator\n\r",ch);
    send_to_char(" 2 - Mana increase\n\r",ch);
    send_to_char(" 3 - More mobility\n\r",ch);
    send_to_char(" 4 - A stomach enlargment\n\r",ch);
    send_to_char(" 5 - Nothing at all\n\r",ch);
    send_to_char(" 6 - More practices\n\r",ch);
    send_to_char("Any of these will cost 1,000,000 experience points.\n\r",ch);
    return(TRUE);
  } else if (cmd==56) { /* Buy */
    half_chop(arg, buf, rem);
    opt = atoi(buf);
    if((opt < 1) || (opt > 6)){
      act("$n tells you 'What?'",TRUE,meta,0,ch,TO_VICT);
      return(TRUE);
    }
    reps = atoi(rem);
    if(reps > 100000){
      send_to_char("Try a smaller amount please.\n\r",ch);
      return(TRUE);
    }
    if(reps <= 0)
      reps = 1;
    for(i=0;i<reps;i++){
      if((GET_META(ch) < 1) && (COST > GET_EXP(ch))){
         if(i==0){
           act("$n tells you 'Come back when you're more experienced.'",
             TRUE,meta,0,ch,TO_VICT);
         } else {
           sprintf(buf,"$n tells you 'You managed only %d meta(s).'",i);
           act(buf,TRUE,meta,0,ch,TO_VICT);
           break;
         }
         return(TRUE);
      }
      if(GET_META(ch) > 0)
        GET_META(ch)--;
      else
        GET_EXP(ch)-=COST;
      do_meta(ch,meta,opt);
    }
    sprintf(buf,"$n tells you '%s - %d times.'",metasign[opt-1],i);
    act(buf,TRUE,meta,0,ch,TO_VICT);
    act("$N buys something from $n.",TRUE,meta,0,ch,TO_NOTVICT);
    return(TRUE);
  } else if(cmd==279){ /* SPEND */
    half_chop(arg, buf, rem);
    tok = get_obj_in_list_vis(ch, buf, ch->carrying);
    if(!tok){
      send_to_char("You have no such item.\n\r",ch);
      return(TRUE);
    }
    if(GET_ITEM_TYPE(tok) == ITEM_TOKEN){
      opt=atoi(rem);
      if((opt < 1) || (opt > 6)){
        act("$n tells you 'Huh?'",TRUE,meta,0,ch,TO_VICT);
        return(TRUE);
      }
      reps=tok->obj_flags.value[0];
      obj_from_char(tok);
      for(i=0;i<reps;i++)
        do_meta(ch,meta,opt);
      sprintf(buf,"$n tells you '%s - %d times.'",metasign[opt-1],reps);
      act(buf,TRUE,meta,0,ch,TO_VICT);
      act("$N spends $o.",TRUE,meta,tok,ch,TO_NOTVICT);
      extract_obj(tok);
      return(TRUE);
    } else {
      send_to_char("You can't spend that.\n\r",ch);
    }
    return(TRUE);
  }
  return(FALSE);
}
int xpstore(struct char_data *ch, int cmd, char *arg)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *mob;
  struct obj_data *obj;
  int val;

  if(cmd==156){
    do_flee(ch,0,0);
    return(TRUE);
  }
  if (IS_NPC(ch))
    return(FALSE);
  if((cmd==57)||(cmd==58)){ /* Sell and Value */
    for(mob=world[ch->in_room].people ; mob ; mob=mob->next_in_room)
      if(mob_index[mob->nr].virtual == 1530)
        break;
    arg = one_argument(arg, buf);
    obj = get_obj_in_list(buf,ch->carrying);
    if(!obj){
      act("$n says 'You don't have one.'",FALSE,mob,0,ch,TO_VICT);
      return(TRUE);
    }
    if(!IS_SET(obj->obj_flags.extra_flags,ITEM_PRIME)){
      act("$n says 'I don't buy trash.'",FALSE,mob,0,ch,TO_VICT);
      return(TRUE);
    }
    val = obj->obj_flags.cost;
    if(cmd==58){
      sprintf(buf,"$n says 'That would get you %d in experience.'",val);
      act(buf,FALSE,mob,0,ch,TO_VICT);
      return(TRUE);
    }
    if(val > 0)
      GET_EXP(ch) += val;
    obj_from_char(obj);
    sprintf(buf,"$n says 'That gets you %d in experience.'",val);
    act(buf,FALSE,mob,0,ch,TO_VICT);
    act("$n sells $p.",FALSE,ch,obj,0,TO_ROOM);
    extract_obj(obj);
    return(TRUE);
  }
  return(FALSE);
}
int tree_gate(struct char_data *ch, int cmd, char *arg)
{
  if((cmd!=3)||(GET_LEVEL(ch) >= IMO)||IS_NPC(ch))
    return(FALSE);
  send_to_char("The Tree blocks your way.\n\r",ch);
  act("The Tree blocks $n's way south.\n\r",TRUE,ch,0,0,TO_ROOM);
  return(TRUE);
}
int level_gate(struct char_data *ch, int cmd, char *arg)
{
  int f,r;
  
  if (cmd != 3)  /* 3 = south (as a command) */
    return FALSE;
  r=world[ch->in_room].number;
  if(r==1453)
    f=(GET_LEVEL(ch) < 12);
  else if(r==2346)
    f=(GET_LEVEL(ch) >  8);
  else if(r==3300)
    f=(GET_LEVEL(ch) >  3);
  else if(r==5298)
    f=(GET_LEVEL(ch) > 20);
  else if(r==6001)
    f=(GET_LEVEL(ch) > 12);
  if((f)&&(GET_LEVEL(ch) < IMO)){
    act("$n attempts go to where $e is not welcome.",FALSE,ch,0,0,TO_ROOM);
    send_to_char("People of your level may not enter.\n\r",ch);
    return TRUE;
  }
  return FALSE;
}
int size_gate(struct char_data *ch, int cmd, char *arg)
{
  int f,r;
  
  if (cmd != 1)  /* 1 = north (as a command) */
    return FALSE;
  f=(GET_MAX_HIT(ch) > 10000);
  if((f)&&(GET_LEVEL(ch) < IMO)){
    act("$n tries to squeeze through a small door.",FALSE,ch,0,0,TO_ROOM);
    send_to_char("You're too big to fit through the door.\n\r",ch);
    return TRUE;
  }
  return FALSE;
}

int cloner(struct char_data *ch, int cmd, char *arg)
{
  int num;
  char buf[128];
  struct char_data *tmp;
  
  if(cmd)
    return(FALSE);
  if(GET_POS(ch) < POSITION_STANDING)
    return(FALSE);
  for(tmp=world[ch->in_room].people;tmp;tmp=tmp->next_in_room){
    if(IS_NPC(tmp)||(GET_LEVEL(tmp)>=IMO))
      continue;
    if(number(0,2)) break;
  }
  if(!tmp)
    return(FALSE);
  if(ch->specials.affected_by == 8)
    return(FALSE);
  if(number(1,10) < 8)
    return(FALSE);
  ch->specials.affected_by = 8;
  GET_STR(ch) = GET_STR(tmp);
  GET_WIS(ch) = GET_WIS(tmp);
  GET_INT(ch) = GET_INT(tmp);
  GET_DEX(ch) = GET_DEX(tmp);
  GET_CON(ch) = GET_CON(tmp);
  GET_SEX(ch) = GET_SEX(tmp);
  GET_LEVEL(ch) = GET_LEVEL(tmp);
  ch->points.hit = ch->points.max_hit = GET_MAX_HIT(tmp);
  ch->points.hitroll = tmp->points.hitroll;
  ch->points.damroll = tmp->points.damroll;
  ch->points.armor   = tmp->points.armor;
  free(GET_NAME(ch));
  GET_NAME(ch) = strdup(GET_NAME(tmp));
  free(ch->player.short_descr);
  ch->player.short_descr = strdup(GET_NAME(tmp));
  sprintf(buf,"%s is standing here, smiling.\n\r",GET_NAME(ch));
  free(ch->player.long_descr);
  ch->player.long_descr  = strdup(buf);
  act("$n smiles.",TRUE,ch,0,0,TO_ROOM);
  return FALSE;
}
int shooter(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *tch,*tmp;
  struct obj_data *obj;
  int f,num;

  if(cmd == 156){
    send_to_char("Seriously?\n\r",ch);
    return(TRUE);
  }
  if(cmd) return(FALSE);
  if(GET_POS(ch) < POSITION_RESTING) return(FALSE);
  if(!(obj=ch->equipment[HOLD]))
    return(FALSE);
  if(GET_ITEM_TYPE(obj)!=ITEM_FIREWEAPON)
    return(FALSE);
  if(IS_SET(ch->specials.act,ACT_AGGRESSIVE)||
    (GET_POS(ch)==POSITION_FIGHTING)){
    for (tch=world[ch->in_room].people; tch; tch = tch->next_in_room){
      if((!IS_NPC(tch))&&(GET_LEVEL(tch) < IMO)&&(ch==tch->specials.fighting)){
        if(GET_POS(tch) <= POSITION_DEAD) continue;
        if(number(1,17) < 9){
          shoot(ch, tch, TYPE_SHOOT);
          if(GET_LEVEL(tch) > (IMO>>1))
            shoot(ch, tch, TYPE_SHOOT);
        } 
      }
    }
  }
  return(FALSE);
}
int bank(struct char_data *ch, int cmd, char *arg)
{
  char buf[MAX_STRING_LENGTH];
  char bufshort[32];
  int k,amt,amtk;

  if (IS_NPC(ch))
    return(FALSE);
  if (cmd==59) { /* List */
    send_to_char("At the bank you may:\n\r\n\r", ch);
    send_to_char("list - get this list\n\r",ch);
    send_to_char("balance - check your balance\n\r",ch);
    send_to_char("deposit <amount> or\n\r",ch);
    send_to_char("withdraw <amount>\n\r\n\r",ch);
    return(TRUE);
  } else if (cmd==227) { /* Balance */
    to_long_long(bufshort,ch->points.bank);
    sprintf(buf,"You have %s coins in the bank.\n\r",bufshort);
    send_to_char(buf,ch);
    return(TRUE);
  } else if ((cmd==228)||(cmd==229)) {  /* deposit or withdraw */
    if(! *arg){
      send_to_char("The banker says 'Specify an amount.'\n\r",ch);
      return(TRUE);
    }
    arg=one_argument(arg, buf);
    amt=atoi(buf);
    if(amt <= 0){
      send_to_char("The banker says 'Amount must be positive.'\n\r",ch);
      return(TRUE);
    }
    if(cmd==228){
      if(amt > GET_GOLD(ch)){
        send_to_char("The banker says 'You don't have that much.'\n\r",ch);
        return(TRUE);
      }
      GET_GOLD(ch)-=amt;
      (ch->points.bank) += (long long)amt;
    } else {
      if((long long)amt > ch->points.bank){
        send_to_char("The banker says 'We don't make loans.'\n\r",ch);
        return(TRUE);
      }
      GET_GOLD(ch)+=amt;
      (ch->points.bank) -= (long long)amt;
    }
    send_to_char("The banker says 'Have a nice day.'\n\r",ch);
    return(TRUE);
  }
  return(FALSE);
}
int kickbasher(struct char_data *ch, int cmd, char *arg)
{
  char buf[128],name[128];
  struct char_data *vict, *v2;
  int i,n,dir,vno;
  struct obj_data *o,*oo,*nexto,*unequip_char(struct char_data *ch, int pos);

  if(cmd)
    return(FALSE);
  if(vict=ch->specials.fighting){
    if(GET_POS(ch) < POSITION_FIGHTING){
      do_stand(ch,"",0);
      if(number(1,100) < 25)
        return(FALSE);
    }
    if((GET_LEVEL(ch) >= IMO)&&(GET_LEVEL(vict) < IMO)){
      for(v2=world[ch->in_room].people;v2;v2=v2->next_in_room){
        if(v2->specials.fighting==ch){
          damage(ch,v2,GET_MAX_HIT(v2)>>4,SKILL_BASH);
          GET_POS(v2)=POSITION_SITTING;
        }
      }
    }
    if(!IS_NPC(vict)){
      n=number(0,4);
      if(n==0){
        if((o=vict->equipment[WIELD])&&
           (!IS_SET(o->obj_flags.extra_flags,ITEM_NOKICK))){
          obj_to_room(o=unequip_char(vict,WIELD),vict->in_room);
          act("$n kicks $p from $N's grasp.",
            TRUE,ch,o,vict,TO_NOTVICT);
          act("$n kicked $p from your grasp.",
            TRUE,ch,o,vict,TO_VICT);
          if(IS_SET(ch->specials.act,ACT_SMART)){
            for(oo=ch->carrying;oo;oo=nexto) {
              nexto = oo->next_content;
              if(IS_SET(oo->obj_flags.extra_flags,ITEM_PRIME))continue;
              act("$n drops $p.", 1, ch, oo, 0, TO_ROOM);
              obj_from_char(oo);
              obj_to_room(oo,ch->in_room);
            }
            sprintf(buf,"%s",o->name);
            for(i=0;buf[i] && (buf[i]!=' ');++i);
            buf[i]=0;
            do_get(ch,buf,0);
            if(o = get_obj_in_list_vis(ch,buf,ch->carrying)){
              vno=number(1,2);
              if(o->obj_flags.value[vno] > 1)
                o->obj_flags.value[vno]--;
            }
          }
        }
      } else if (n!=1) {
        damage_equips(ch,vict);
      } else {
        damage(ch,vict,GET_LEVEL(ch)<<2,SKILL_KICK);
        if(!(vict=ch->specials.fighting)) return(FALSE);
        if(number(1,7) < 2){
          dir = number(0, 5);
          if(CAN_GO(vict,dir)){
            if(do_simple_move(vict,dir,FALSE)==1){ 
              act("$n's powerful kick sends $N into the next room!",
                TRUE,ch,0,vict,TO_NOTVICT);
              act("$n comes tumbling into the room!",
                TRUE,vict,0,0,TO_NOTVICT);
              act("$n kicks you into the next room!",
                TRUE,ch,0,vict,TO_VICT);
              if(vict->specials.fighting)
               if(vict->specials.fighting->specials.fighting == vict)
                stop_fighting(vict->specials.fighting);
              stop_fighting(vict);
              GET_POS(vict)=POSITION_SITTING;
              WAIT_STATE(vict,2*PULSE_VIOLENCE);
              do_look(vict,"",15);
            }
          }
        }
      }
    }
    return(TRUE);
  }
  return(FALSE);
}

int loader(struct char_data *ch, int cmd, char *arg)
{
  if(cmd)
    return(FALSE);
  if(ch->specials.fighting){
    if(number(1,13) < 4)
      loadhelper(ch,-1);
    else
      spell_cure_critic(GET_LEVEL(ch),ch,ch,0);
    return(TRUE);
  }
  return(FALSE);
}
int portal(struct char_data *ch, int cmd, char *arg)
{
  int location,ok;
  extern int top_of_world;

  if(cmd != 3)   /* specific to Room 2176 */
    return(FALSE);
  location = number(1,top_of_world-1000);
  ok=TRUE;
  if (IS_SET(world[location].room_flags,OFF_LIMITS))
    ok=FALSE;
  else if (IS_SET(world[location].room_flags,PRIVATE))
    ok=FALSE;
  if(!ok){
    send_to_char("You bump into something, and go nowhere.\n\r",ch);
    act("$n seems to bump into nothing??",FALSE,ch,0,0,TO_NOTVICT);
  } else {
    act("$n seems to have left??",FALSE,ch,0,0,TO_NOTVICT);
    send_to_char("You are momentarily disoriented.\n\r",ch);
    char_from_room(ch);
    char_to_room(ch,location);
    do_look(ch,"",15);
  }
  return(TRUE);
}
int drinkmachine(struct char_data *ch, int cmd, char *arg)
{
  extern char *drinks[];
  extern int drink_aff[][3];
  char buf[KBYTE];
  struct obj_data *temp;
  int amount;

  if(cmd != 11)
    return FALSE;
  one_argument(arg,buf);
  if(!*buf){
    send_to_char("Drink from what?\n\r",ch);
    return TRUE;
  }
  temp = get_obj_in_list_vis(ch,buf,world[ch->in_room].contents);
  if(!temp){
    send_to_char("Drink from what??\n\r",ch);
    return FALSE;
  }
  if(GET_ITEM_TYPE(temp)!=ITEM_DISPENSER){
    send_to_char("You can't drink from that.\n\r",ch);
    return TRUE;
  }
  if((number(-2,GET_COND(ch,DRUNK))>10)&&(GET_COND(ch,THIRST)>0)) {
    act("You simply fail to reach your mouth!", FALSE, ch, 0, 0, TO_CHAR);
    act("$n tried to drink but missed $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
  if((GET_COND(ch,FULL)>((95*GET_GUT(ch))/100))&&(GET_COND(ch,THIRST)>0)) {
    act("Your stomach can't contain anymore!",FALSE,ch,0,0,TO_CHAR);
    return;
  }
  if(temp->obj_flags.value[1]>0) {  
    sprintf(buf,"$n drinks %s from $p",drinks[temp->obj_flags.value[2]]);
    act(buf, TRUE, ch, temp, 0, TO_ROOM);
    sprintf(buf,"You drink the %s.\n\r",drinks[temp->obj_flags.value[2]]);
    send_to_char(buf,ch);
    amount = number(1,(GET_GUT(ch)-GET_COND(ch,FULL)-GET_COND(ch,THIRST)));
    amount = MIN(amount,temp->obj_flags.value[1]);
    gain_condition(ch,DRUNK,(int)((int)drink_aff
      [temp->obj_flags.value[2]][DRUNK]*amount)/4);
    gain_condition(ch,FULL,(int)((int)drink_aff
      [temp->obj_flags.value[2]][FULL]*amount));
    gain_condition(ch,THIRST,(int)((int)drink_aff
      [temp->obj_flags.value[2]][THIRST]*amount));
    if(GET_COND(ch,DRUNK)>(GET_GUT(ch)/5))
      act("You feel drunk.",FALSE,ch,0,0,TO_CHAR);
    if(GET_COND(ch,THIRST)>(GET_GUT(ch)/2))
      act("You do not feel thirsty.",FALSE,ch,0,0,TO_CHAR);
    if(GET_COND(ch,FULL)>((95*GET_GUT(ch))/100))
      act("You are full.",FALSE,ch,0,0,TO_CHAR);
    temp->obj_flags.value[1] -= amount;; 
  } else {
    act("It's empty already.",FALSE,ch,0,0,TO_CHAR);
  }
  return(TRUE);
}
int grandpa(struct char_data *ch, int cmd, char *arg)
{
  struct obj_data *o,*unequip_char(struct char_data *ch, int pos);
  struct char_data *vict,*tmp,*pal;
  static int teleflag=0;
  int lev;
  char name[128];

  if(cmd == 156){
    act("$n sticks $s thumb up $s his ass.",FALSE,ch,0,0,TO_ROOM);
    act("You sticks your thumb up your ass.",FALSE,ch,0,0,TO_ROOM);
    return(TRUE);
  }
  if(cmd == 9){
    if(ch->equipment[WIELD]){
      one_argument(arg,name);
      vict=get_char_room_vis(ch,name);
      if(!vict)
        return(FALSE);
      if(mob_index[vict->nr].func!=grandpa)
        return(FALSE);
      obj_to_room(o=unequip_char(ch,WIELD),ch->in_room);
      act("$n kicks the weapon from $N's grasp.",
        TRUE,vict,0,ch,TO_NOTVICT);
      act("$n kicked your weapon from your grasp.",
        TRUE,vict,0,ch,TO_VICT);
    }
    return(FALSE);
  }
  if(cmd)
    return(FALSE);
  vict=ch->specials.fighting;
  if(vict && !number(0,4)){
    do_flee(vict,0,0);
    return(FALSE);
  }
  if(GET_HIT(ch) < GET_MAX_HIT(ch)/2){
    if(teleflag < 3){
      teleport(ch,0);
      teleflag++;
      return(TRUE);
    }
  }
  if(vict=ch->specials.fighting){
    for(tmp=world[ch->in_room].people; tmp; tmp = tmp->next_in_room){
      if((ch == tmp->specials.fighting) && (!IS_NPC(tmp)))
        damage(ch,tmp,GET_LEVEL(tmp)>>5,TYPE_BLAST);
    }
    return TRUE;
  } else {
    if(teleflag > 0){
      cast_burlyheal(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,ch,0);
      --teleflag;
    }
  }
  return(FALSE);
}
int plasterman(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *tmp;
  char buf[KBYTE];
  unsigned f,n;

  if(cmd) return(0);
  for(n=0,tmp=world[ch->in_room].people;tmp;tmp=tmp->next_in_room)
    n++;
  if((n > 4) || ch->specials.fighting || (GET_HIT(ch) < GET_MAX_HIT(ch))){
    f=1;
    teleport(ch,0);
  } else
    f=0;
  switch (number(0,f ? 47 : 619)){
    case 0:
      for(tmp=world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
        if((tmp!=ch) && !IS_NPC(tmp)){
          sprintf(buf,"I'm here with %s, having a swell time!",
            tmp->player.name);
          do_shout(ch,buf,0);
          return(TRUE);
        }
    case 1:
      for(tmp=world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
        if((tmp!=ch) && IS_NPC(tmp)){
          sprintf(buf,"I'm here with %s, having a swell time!",
            tmp->player.short_descr);
          do_shout(ch,buf,0);
          return(TRUE);
        }
    case 29:
      sprintf(buf,"Never fear, %s is here!",ch->player.short_descr);
      do_shout(ch,buf,0);
      return(1);
    default:
      return(0);
  }
}

#define NINSULTS 23

static char *insult[]={
  "Scram %s, you twit.",
  "I guess %s's brain isn't getting any oxygen.",
  "You're under arrest %s, for impersonating a MUD player.",
  "Yo %s, catch any fish today?",
  "You're all worthless, especially %s!",
  "Did I ask for your opinion, %s?",
  "Time to clean out the riff-raff, let's start with %s.",
  "Well, look what the cat dragged in - %s.",
  "Go home %s, and don't come back.",
  "Who asked for your opinion, %s?",
  "Say hi to your mother for me, %s.",
  "You don't amuse me, %s.",
  "Kiss my shorts, %s!",
  "%s proves you don't need brains to MUD.",
  "What's %s up to now?",
  "You're ugly and you stink, %s.",
  "Weren't you just leaving, %s?",
  "Don't forget to kiss an idiot today!  Hey, where's %s?",
  "I'm here with %s, having a miserable time.",
  "Yeah, I got a match, %s, my ass and your face.",
  "Eat celery and die, %s!",
  "Well if it isn't %s the Loser.",
  "Where's that wimp %s today?"
};

#define NBA 31

static char *badadj[]={
  "toadsucking",
  "flannelmouthed",
  "buttkissing",
  "frogfaced",
  "rotten",
  "prevaricating",
  "insipid",
  "stinking",
  "disgusting",
  "revolting",
  "snotnosed",
  "threetoed",
  "fourlegged",
  "liceinfested",
  "spineless",
  "perverted",
  "drooling",
  "babbling",
  "ranting",
  "raving",
  "roachinfested",
  "blubbering",
  "yammering",
  "pissbrained",
  "lowlife",
  "whining",
  "whimpering",
  "underhanded",
  "flatulent",
  "traitorous",
  "self-righteous"
};

#define NBN 22

static char *badnoun[]={
  "moron",
  "idiot",
  "jackass",
  "liar",
  "deviant",
  "walrus",
  "protosimian",
  "reptile",
  "pervert",
  "macuser",
  "fool",
  "sot",
  "waterbuffalo",
  "bacterium",
  "virus",
  "snake",
  "swine",
  "mongrel",
  "monkey",
  "slimeball",
  "sicko",
  "pissant"
};

#define NSOCS 15
static unsigned short soc_list[]={
 26,30,75,106,109,111,114,119,133,134,137,140,171,182,190};

#define NSOCS2 14
static unsigned short soc_list2[]={
  27,30,96,96,130,137,139,148,190,191,194,212,261,264
};

#define AUNT_KATE "twick"

int unclefred(struct char_data *ch, int cmd, char *arg)
{
  static char fred_path[] =
"xso90z00g233z22d010h23000y22y3x322v13yv31xv0w0y0x21z100x0y213w22y1122xo1cr.";
  static bool moving = FALSE;
  static int index=0,hosp=0;
  int a1,a2,n;
  char buf[256];
  struct char_data *p,*q,*r,*tmp,*v1,*v2;
  struct descriptor_data *i;

  if(!hosp){
    hosp=(-1);
    for(n=0;n<=top_of_world;n++)
      if(world[n].number == 3060) {   /* 3060 = Hospital */
        hosp=n;
        break;
      }
  }
  if((cmd==94) && (GET_LEVEL(ch) > (IMO+1))){
    if(!moving){
      moving=TRUE; index=0; return(FALSE);
    }
  }
  if(cmd) return(FALSE);
  bombtosser(ch);
  if(GET_HIT(ch) < GET_MAX_HIT(ch))
    cast_extremelyburlyheal(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0);
  if((GET_HIT(ch) < GET_MAX_HIT(ch))&&(GET_POS(ch) >= POSITION_FIGHTING)){
    cast_extremelyburlyheal(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0);
    if(!IS_SET(world[ch->in_room].room_flags,NO_MAGIC)){
      spell_earthquake(GET_LEVEL(ch),ch,0,0);
      spell_radiation (GET_LEVEL(ch),ch,0,0);
      spell_earthquake(GET_LEVEL(ch),ch,0,0);
    }
  }
  if(tmp=ch->specials.fighting){
    p=get_char_room_vis(ch,AUNT_KATE);
    if(p)
      hit(p,tmp,TYPE_UNDEFINED);
    for(q=world[ch->in_room].people;q;q=tmp){
      tmp=q->next_in_room;
      if(!q->specials.fighting) continue;
      if(!IS_NPC(q) && q->affected){
        affect_remove(q,q->affected);
        act("$n mumbles something about $N...",FALSE,ch,0,q,TO_ROOM);
      }
      if(!IS_NPC(q) && (r=q->specials.fighting) && (r!=ch) && (r!=p))
        spell_nova(IMO-1,ch,q,0);
      else
        spellcast(ch,q,GET_LEVEL(ch));
    }
    return(FALSE);
  } else {
    tmp=get_char_room_vis(ch,AUNT_KATE);
    if(tmp && (tmp->specials.fighting)){
      hit(ch,tmp->specials.fighting,TYPE_UNDEFINED);
      return(TRUE);
    }
  }
  if (!moving) {
    if (time_info.hours == 9) {
      do_shout(ch,"Time to get up, you lazy swine!",0);
      moving = TRUE;
      index = 0;
      sprintf(buf,"My damroll is now %d.\n\r",ch->points.damroll);
      do_say(ch,buf,0);
    }
  }
  if(!moving || (GET_POS(ch) < POSITION_SLEEPING))
    return FALSE;
  switch (n=fred_path[index]) {
    case '9':
      do_open(ch,"door",0);
      n-=6;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
      if(CAN_GO(ch,n-'0'))
        do_move(ch,"",n-'0'+1);
      else {
        tmp=get_char_vis(ch,AUNT_KATE);
        if(tmp){
          spell_relocate(IMO+1,ch,tmp,0);
          moving=FALSE;
          index=0;
        }
      }
      break;
    case 'c':
      do_close(ch,"door",0);
      break;
    case 'd':
      do_drop(ch,"all",0);
      break;
    case 'g':
      do_get(ch,"all",0);
      break;
    case 'h':
      if(ch->in_room == hosp){
        GET_HIT(ch) = GET_MAX_HIT(ch);
        GET_GOLD(ch) >>= 1;
        act("$n buys a full heal.",FALSE,ch,0,0,TO_ROOM);
      }
      break;
    case 'o':
      do_open(ch,"door",0);
      break;
    case 'r':
      GET_POS(ch) = POSITION_RESTING;
      act("$n takes a load off his feet.",FALSE,ch,0,0,TO_ROOM);
      break;
    case 's':
      GET_POS(ch) = POSITION_STANDING;
      act("$n stands up.",FALSE,ch,0,0,TO_ROOM);
      break;
    case 'v':
      if(tmp=gettarget(ch,1))
        do_action(ch,GET_NAME(tmp),soc_list2[number(0,NSOCS2-1)]);
      break;
    case 'w':
      if(tmp=gettarget(ch,1)){
        sprintf(buf,insult[number(0,NINSULTS-1)],GET_NAME(tmp));
        do_say(ch,buf,0);
      }
      break;
    case 'x':
      do_action(ch,"",soc_list[number(0,NSOCS-1)]);
      break;
    case 'y':
      if(tmp=gettarget(ch,number(0,1))){
        a1=number(0,NBA-1);
        do a2=number(0,NBA-1); while(a1==a2);
        sprintf(buf,"Hey %s, you %s%s%s!",GET_NAME(tmp),
         badadj[a1],badadj[a2],
         badnoun[number(0,NBN-1)]);
        do_shout(ch,buf,0);
      }
      break;
    case 'z':
      if(tmp=gettarget(ch,number(0,1))){
        sprintf(buf,insult[number(0,NINSULTS-1)],GET_NAME(tmp));
        do_shout(ch,buf,0);
      }
      break;
    case '.':
      moving = FALSE;
      index=0;
      break;
  }
  index++;
  return TRUE;
}

int rusher(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *tch,*vict;
  int i, newroom, found, loc_nr;
  static int tinksroom=0;

  if(cmd || !AWAKE(ch))
    return(FALSE);
  if(!tinksroom){
    found=0;
    for(tinksroom=0;(!found) && (tinksroom<=top_of_world);tinksroom++)
      if(world[tinksroom].number == 2804){
        found=TRUE;
        break;
      }
  }
  if(vict=ch->specials.fighting){
    if(!IS_NPC(vict) && vict->affected){
      affect_remove(vict,vict->affected);
      act("$n utters the words 'zolpidem tartrate'",FALSE,ch,0,0,TO_ROOM);
    }
    return(snake(ch,cmd,arg) && magic_user(ch,cmd,arg));
  }
  do_get(ch,"all",0);
  for(i=0;i<6;++i){
    if(CAN_GO(ch,i)){
      newroom=world[ch->in_room].dir_option[i]->to_room;
      for(tch=world[newroom].people;tch;tch=tch->next_in_room){
        if(!IS_NPC(tch)){
          if(GET_POS(tch) == POSITION_FIGHTING){
            do_simple_move(ch,i,FALSE);
            if(!IS_NPC(tch))
              hit(ch,tch,TYPE_UNDEFINED);
            return(TRUE);
          }
        }
      }
    }
  }
  if((ch->in_room != tinksroom) && (number(1,5)==3)){
    char_from_room(ch);
    char_to_room(ch,tinksroom);
  }
  return(FALSE);
}
int highwayman(struct char_data *ch, int cmd, char *arg)
{
  if(cmd)
    return(FALSE);

  if(ch->specials.fighting){
    switch(number(0,2)){
      case 0:
      case 1: return(kickbasher(ch,cmd,arg));
      case 2: return(magic_user(ch,cmd,arg));
    }
  } else {
    return(thief(ch,cmd,arg));
  }
}
int buddy(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *tmp;
  char *s;

  if(cmd == 132){   /* snap */
   if(IS_NPC(ch))
     return(FALSE);
   s=GET_NAME(ch);
   for(tmp=world[ch->in_room].people;tmp;tmp=tmp->next_in_room){
    if(IS_NPC(tmp) && tmp->player.description){
     if(strncmp(tmp->player.description,s,strlen(s))==0){
      act("$n snaps $s fingers.",FALSE,ch,0,0,TO_ROOM);
      act("*SNAP*",FALSE,ch,0,0,TO_CHAR);
      if(!tmp->master){
        do_follow(tmp,s,0);
        SET_BIT(tmp->specials.affected_by,AFF_CHARM);
      } else {
        REMOVE_BIT(tmp->specials.affected_by,AFF_CHARM);
        do_follow(tmp,GET_NAME(tmp),0);
      }
      return(TRUE);
     }
    }
   }
  }
  return(FALSE);
}

#define NDA 16

static char *dad_action[]={
  "say Damn, I hope Mom doesn't see me like this.",
  "say That damn woman has all my money.",
  "say You better do what your mother tells you.",
  "say Let's go have a brew?",
  "say Some day that boss of mine is going to go too far!",
  "say That Johnnie Walker hits the spot, doesn't it?",
  "say I'm not a drinking man myself, but if you got a bottle on you..",
  "say Could you be so kind as to lead me to Filthy's?",
  "say I believe I'll pay a visit to the Grubby Inn.",
  "say Don't tell Mom, but that Suzy is one fine lady!",
  "say I ought to take you over my knee and paddle you right now!",
  "say Damn kids!",
  "say I suppose I ought to go find Grandpa.",
  "spit",
  "bird",
  "bellow"
};
int dad(struct char_data *ch, int cmd, char *arg)
{
  int n;
  struct obj_data *obj;
  struct char_data *vict,*tmp;
  char buf[128];

  if(cmd)
    return(0);
  if(!ch->specials.fighting){
    n=number(0,500);
    if(n < NDA){
      strcpy(buf,dad_action[n]);
      command_interpreter(ch,buf);
    }
    if(GET_HIT(ch) < GET_MAX_HIT(ch))
      cast_cure_critic(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0);
    return(0);
  }
  vict=ch->specials.fighting;
  if(IS_NPC(vict) && (GET_LEVEL(vict) < IMO)){
    act("$n brutally slays $N", FALSE, ch, 0, vict, TO_NOTVICT);
    raw_kill(vict);
    return(1);
  }
  switch(number(0,5)){
    case 0: loadhelper(ch,-1); break;
    case 1:
    case 2: damage_equips(ch,vict); break;
    case 3:
    case 4:
      for(tmp=world[ch->in_room].people; tmp;tmp=tmp->next_in_room)
        if(tmp->specials.fighting == ch){
          if(number(0,12))
            cast_sunburst(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,tmp,0);
          else
            cast_nova(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,tmp,0);
        }
      break;
  }
  return(1);
}
int druid(struct char_data *ch, int cmd, char *arg)
{
  char *s;
  struct char_data *vict;

  if(cmd)
    return(0);
  if(vict=ch->specials.fighting){
    if(vict && IS_NPC(vict) && (vict->master)){
      cast_nova(IMO,ch,"",SPELL_TYPE_SPELL,vict,0);
      cast_nova(IMO,ch,"",SPELL_TYPE_SPELL,vict->master,0);
      return(FALSE);
    }
    if(!IS_NPC(vict) && vict->affected){
      affect_remove(vict,vict->affected);
      act("$n utters the words 'zolpidem tartrate'",FALSE,ch,0,0,TO_ROOM);
    }
    return(kickbasher(ch,cmd,arg));
  }
  if(GET_HIT(ch) < (GET_MAX_HIT(ch)>>2))
    cast_cure_critic(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,ch,0);
  if(s=ch->player.title){
    vict=get_char_vis(ch,s);
    if(vict){
      spell_summon(900,ch,vict,0);
      if(vict->in_room == ch->in_room)
        hit(ch,vict,TYPE_UNDEFINED);
    }
  }
  return(0);
}
int vampire(struct char_data *ch, int cmd, char *arg)
{
  char *s;
  struct char_data *vict;

  if(cmd)
    return(0);
  vict=ch->specials.fighting;
  if(vict && IS_NPC(vict) && (vict->master)){
    cast_nova(IMO,ch,"",SPELL_TYPE_SPELL,vict,0);
    return(FALSE);
  }
  if(vict=ch->specials.fighting){
    cast_cure_critic(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,ch,0);
    return(kickbasher(ch,cmd,arg));
  }
  return(0);
}
int teleporter(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *vict;

  if(cmd)
    return(FALSE);
  if(vict=ch->specials.fighting){
    switch(number(0,9)){
      case 0:
      case 1:
      case 2:
        teleport(vict,0);
        loadhelper(ch,vict->in_room);
        return(TRUE);
        break;
      case 3:
        spell_heal(150,ch,ch,0);
        break;
    }
  }
  return(FALSE);
}
int viper(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *vict;
  int i,n;

  if(cmd==156){
    command_interpreter(ch,"fart");
    return(TRUE); 
  }
  if(cmd)
    return(FALSE);
  if(vict=ch->specials.fighting){
    switch(number(0,12)){
      case 0:
        druid(ch,0,0);
        return(TRUE);
      case 1:
        vampire(ch,0,0);
        return(TRUE);
      case 2:
        sleeper(ch,0,0);
        return(TRUE);
      case 3:
        n=(450-GET_LEVEL(vict))/25;
        for(i=0;i<n;i++)
          cast_slime(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,vict,0);
        return(TRUE);
    }
  }
  return(FALSE);
}
int samhill(struct char_data *ch, int cmd, char *arg)
{
  static struct char_data *ptr;
  struct char_data *tch;

  if(cmd){
    if((!IS_NPC(ch)) && (GET_LEVEL(ch) < IMO) &&
       (GET_POS(ch) == POSITION_STANDING) && !ch->specials.fighting)
      ptr = ch;
    return(0);
  }
  if(!ptr)
    return(0);
  for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room){
    if(ptr == tch){
      act("$n slaps $N hard, knocking $M to the ground!.",
        TRUE,ch,0,tch,TO_NOTVICT);
      act("$n slaps you very hard, knocking you to the ground!.",
        TRUE,ch,0,tch,TO_VICT);
      GET_POS(tch) = POSITION_SITTING;
      ptr = 0;
      break;
    }
  }
  ptr = 0;
  return(0);
}
int adaptor(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *v;
  int dir;

  if(cmd){
    if(cmd == 84) { /* cast */
      for(v=world[ch->in_room].people;v;v=v->next_in_room)
        if(IS_NPC(v) && (mob_index[v->nr].func==adaptor)){
          dir=number(0,5);
          if(CAN_GO(v,dir) && number(0,2)){
            do_simple_move(v,dir,FALSE);
            cast_cure_critic(GET_LEVEL(v),v,"",SPELL_TYPE_SPELL,v,0);
          } else if(ch==v->specials.fighting){
            cast_veryburlyheal(GET_LEVEL(v),v,"",SPELL_TYPE_SPELL,v,0);
            cast_slime(GET_LEVEL(v),v,"",SPELL_TYPE_SPELL,ch,0);
          }
          return(0);
        }
    }
    return(0);
  }
  v = ch->specials.fighting;
  if(!v){
    ch->points.damroll = 1;
  } else {
    ch->points.damroll = GET_HIT(v)>>2;
  }
  return(0);
}

int defuser(struct char_data *ch, int cmd, char *arg)
{
  struct obj_data *k;

  if(cmd)
    return(FALSE);
  for(k=world[ch->in_room].contents; k ; k = k->next_content)
    if(GET_ITEM_TYPE(k) == ITEM_BOMB)
      if(IS_SET(k->obj_flags.extra_flags,ITEM_POOFSOON)){
        REMOVE_BIT(k->obj_flags.extra_flags,ITEM_POOFSOON);
        act("$n defuses $o.",TRUE,ch,k,0,TO_ROOM);
        return(TRUE);
      }
  return(FALSE);
}
int builder(struct char_data *ch, int cmd, char *arg)
{
  int loc,dir,srm,drm;
  static int baseroom=0;

  if(cmd)
    return(0);
  if(!baseroom)
    for(loc=0;loc<=top_of_world;loc++)
      if(world[loc].number==10000){
        baseroom=loc;
        break;
      }
  if(ch->specials.fighting)
    return(adaptor(ch,cmd,arg));
  if(number(0,22))
    return(0);
  dir=number(0,3);
  if(CAN_GO(ch,dir)){
    srm=ch->in_room;
    drm=world[srm].dir_option[dir]->to_room;
    free(world[srm].dir_option[dir]);
    world[srm].dir_option[dir] = 0;
    free(world[srm].dir_option[dir]);
    world[drm].dir_option[revdir[dir]] = 0;
    act("$n puts up a wall.",TRUE,ch,0,0,TO_ROOM);
  }
  return(0);
}
int sneaker(struct char_data *ch, int cmd, char *arg)
{
  if(cmd)
    return(FALSE);
  hunter(ch,0,0);
  check4enemies(ch,ch->equipment[WIELD] ? 2 : 1);
}
int wilbur(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *vict, *tch;
  struct obj_data *o;
  int i,n,newroom;
  static int t=0;

  if(cmd)
    return(FALSE);
  if(vict=ch->specials.fighting){
    if(GET_HIT(ch) > (GET_MAX_HIT(ch)-1000))
      return(FALSE);
    for(i=0;i<6;++i){
      if(CAN_GO(ch,i)){
        act("$n flees!",TRUE,ch,0,0,TO_ROOM);
        do_simple_move(ch,i,FALSE);
        cast_veryburlyheal(GET_LEVEL(ch),ch,"",SPELL_TYPE_SPELL,ch,0);
        return(TRUE);
      }
    }
    teleport(ch,0);
  } else {
    sneaker(ch,0,0);
    if(++t > 5){
      t=0;
      for(tch=world[ch->in_room].people;tch;tch=tch->next_in_room){
        if(!IS_NPC(tch) && (GET_LEVEL(tch) < IMO)){
          if(o=tch->equipment[WIELD]){
            if(isname("wd",o->name)){
              hit(ch,tch,SKILL_BACKSTAB);
              return(1);
            }
          }
          if(GET_GOLD(tch) > 1000){
            n=number(2,GET_LEVEL(tch));
            GET_GOLD(tch)-=n; GET_GOLD(ch)+=n;
            act("$n steals some coins from $N.",TRUE,ch,0,tch,TO_NOTVICT);
            act("$n steals some coins from you.",TRUE,ch,0,tch,TO_VICT);
            do_action(ch,"",soc_list[number(0,NSOCS-1)]);
            return(FALSE);
          }
        }
      }
    }
  }
  return(FALSE);
}

