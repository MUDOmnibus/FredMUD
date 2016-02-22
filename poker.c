
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

/*   external vars  */

extern int melsnumber;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct index_data *mob_index;

/* extern procedures */

void do_flee(struct char_data *ch, char *argument, int cmd);
void hit(struct char_data *ch, struct char_data *victim, int type);

#ifdef NEEDS_STRDUP
char *strdup(char *source);
#endif

/*   CASINO   */

#define BET_SYNTAX "Bet like this: bet <amount> on <number>.\n\r"
#define CMD_BET 235
#define MAXBETS 32
#define LASTCALL 13
#define LASTSTATE 15

struct bet_data {
   char name[16];
   int amt,num,hit,flag;
} b[MAXBETS];

int barker(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *dude,*tmp,*vict;
  static int ticker=0,state=0,nbets=0;
  int i,amt,num,winner;
  char buf[128],amts[128],nums[128];

  if(cmd==156) return(TRUE);
  if(!cmd){
    ticker=1-ticker;
    if(ticker)
      return(FALSE);
    switch(state){
      case  0:
      case  1: break;
      case  2:
        act("$n barks 'Step right up and place your bets, folks.'",
          TRUE,ch,0,0,TO_ROOM);
        break;
      case  3:
      case  4:
      case  5: break;
      case  6:
        act("$n barks 'Don't be shy folks, nothing ventured nothing gained.'",
          TRUE,ch,0,0,TO_ROOM);
        break;
      case  7: break;
      case  8:
        act("$n barks 'Step right up and place your bets, folks.'",
          TRUE,ch,0,0,TO_ROOM);
        break;
      case  9:
      case 10: break;
      case 11:
        act("$n barks 'Last call for bets on the wheel.'",
          TRUE,ch,0,0,TO_ROOM);
        break;
      case 12:
        act("$n barks 'Betting is now closed, sorry folks.'",
          TRUE,ch,0,0,TO_ROOM);
        break;
      case 13:
        act("$n spins the wheel...", TRUE,ch,0,0,TO_ROOM);
        break;
      case 14:
        act("The wheel begins to slow down...", TRUE,ch,0,0,TO_ROOM);
        break;
      case 15:
        if(melsnumber < 0){
          winner = number(1,101);
        } else {
          winner = melsnumber;
          melsnumber = -1;
        }
        sprintf(buf,"The wheel comes to a stop on the number %d.",winner);
        act(buf,TRUE,ch,0,0,TO_ROOM);
        for(i=0;i<nbets;++i){
          if(b[i].num == winner){
            if(tmp = get_char_room_vis(ch,b[i].name)){
              amt = 100*b[i].amt;
              sprintf(buf,"$n says 'Well well, %s wins %d coins!",
                GET_NAME(tmp),amt);
              act(buf,TRUE,ch,0,tmp,TO_ROOM);
              GET_GOLD(tmp) += amt;
              GET_GOLD(ch) -= amt;
            } else {
              sprintf(buf,"$n says 'Hmm, %s isn't here, too bad.'",b[i].name);
              act(buf,TRUE,ch,0,0,TO_ROOM);
            }
          }
        }
        nbets=0;
        act("$n barks 'You can now place your bets for the next spin.'",
          TRUE,ch,0,0,TO_ROOM);
        break;
    }
    ++state;
    if(state > LASTSTATE) state=0;
    return(FALSE);
  }
  if(cmd == CMD_BET){
    for(tmp=world[ch->in_room].people; tmp; tmp=tmp->next_in_room)
      if(mob_index[tmp->nr].func == barker)
        break;
    if(!tmp){
      send_to_char("Sorry, the barker seems to have left.\n\r",ch);
      return(TRUE);
    }
    if(!CAN_SEE(tmp,ch)){
      act("$n mumbles something about invisible players...",
        TRUE,tmp,0,0,TO_ROOM);
      return(TRUE);
    }
    if(state >= LASTCALL){
      act("$n politely informs you that betting is now closed.",
        TRUE,tmp,0,ch,TO_VICT);
      return(TRUE);
    }
    arg=one_argument(arg,amts);
    if(! amts[0]){ send_to_char(BET_SYNTAX,ch); return(TRUE);}
    arg=one_argument(arg,nums);
    if(! nums[0]){ send_to_char(BET_SYNTAX,ch); return(TRUE);}
    if(!isdigit(nums[0])){
      if(strncasecmp(nums,"on",2)==0){
        arg=one_argument(arg,nums);
        if(! nums[0]){ send_to_char(BET_SYNTAX,ch); return(TRUE);}
      } else {
        send_to_char(BET_SYNTAX,ch);
        return(TRUE);
      }
    }
    amt=atoi(amts);
    num=atoi(nums);
    if((num <= 0) || (num > 101)){
      act("$n says '$N, you can't bet on that number here.'",
        TRUE,tmp,0,ch,TO_ROOM);
      return(TRUE);
    }
    if((amt <= 0) || (amt > 1000000)){
      act("$n says '$N, bets must be between 1 and 1,000,000 coins.'",
        TRUE,tmp,0,ch,TO_ROOM);
      return(TRUE);
    }
    if(amt > GET_GOLD(ch)){
      act("$n says '$N seems to want to bet more than $E has.'",
        TRUE,tmp,0,ch,TO_ROOM);
      return(TRUE);
    }
    sprintf(buf,"$n says '$N bets %d coins on number %d.'",amt,num);
    act(buf,TRUE,tmp,0,ch,TO_ROOM);
    strcpy(b[nbets].name,GET_NAME(ch));
    b[nbets].amt=amt; b[nbets].num=num;
    if((++nbets) >= MAXBETS){
      state=LASTCALL;
      act("$n barks 'Betting is now closed, sorry folks.'",
        TRUE,tmp,0,0,TO_ROOM);
    }
    GET_GOLD(ch) -= amt;
    GET_GOLD(tmp) += amt;
    WAIT_STATE(ch,10);
    return(TRUE);
  }
  return(FALSE);
}

static char *cardsym="A23456789TJQK";
static int cardval[]={11,2,3,4,5,6,7,8,9,10,10,10,10};

#define CLOSESTATE 5
#define HITSTATE 7
#define BJLASTSTATE 10
#define CMD_CARD 247
#define BJMAX 16
struct bet_data bj[BJMAX];

int dealer(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *tmp,*vict;
  static int state=0,nbets=0,ticker=0,dc,dct,dflag;
  int i,j,k,amt,num;
  char buf[128],amts[128],ha[8];

  if(cmd==156) return(TRUE);
  if(!cmd){
    ticker=1-ticker;
    if(ticker)
      return(FALSE);
    if(ch->specials.fighting){
      for(tmp=world[ch->in_room].people; tmp; tmp=tmp->next_in_room)
        if(IS_NPC(tmp))
           damage(tmp,ch->specials.fighting,1000,SKILL_KICK);
    }
    switch(state){
      case  0:
      case  1:
      case  2:
      case  3:
        if(!(state & 1))
          act("$n says 'Place your bets now for the next round.'",
            TRUE,ch,0,0,TO_ROOM);
        break;
      case  4:
        act("$n says 'Ok folks, the table is closed.'",
          TRUE,ch,0,0,TO_ROOM);
        break;
      case  5:
        dc=number(0,12);
        dct=cardval[dc];
        dflag=(dc==0);
        sprintf(buf,"$n says 'I deal my up card first, the <%c>.'",cardsym[dc]);
        act(buf,TRUE,ch,0,0,TO_ROOM);
        break;
      case  6:
        for(i=0;i<nbets;++i){
          for(j=0;j<2;++j){
            k=number(0,12);
            ha[j+j]=cardsym[k];
            ha[j+j+1]=(j==0) ? '-' : 0;
            bj[i].num+=cardval[k];
            if(k==0) bj[i].flag++;
            if((bj[i].num > 21) && (bj[i].flag > 0)){
              bj[i].num-=10; bj[i].flag--;
            }
          }
          sprintf(buf,"$n says '%s is dealt %s for a total of %d.'",
            bj[i].name,ha,bj[i].num);
          act(buf,TRUE,ch,0,0,TO_ROOM);
          if(bj[i].num == 21){
            sprintf(buf,"$n says '%s has blackjack, and wins %d.\n\r",
              bj[i].name,2*bj[i].amt);
            act(buf,TRUE,ch,0,0,TO_ROOM);
            if(tmp=get_char_room_vis(ch,bj[i].name)){
              amt=3*bj[i].amt;
              GET_GOLD(tmp) += amt;
              GET_GOLD(ch ) -= amt;
              bj[i].amt=0;
              bj[i].name[0]=0;
            }
          }
        }
        act("$n says 'Just type CARD if you want another card.'",
          TRUE,ch,0,0,TO_ROOM);
        break;
      case  7:
        for(i=0;i<nbets;++i){
          if(bj[i].hit){
            bj[i].hit=0;
            k=number(0,12);
            bj[i].num+=cardval[k];
            if(k==0) bj[i].flag++;
            if((bj[i].num > 21) && (bj[i].flag > 0)){
              bj[i].num-=10; bj[i].flag--;
            }
            sprintf(buf,"$n says '%s gets the <%c> for a total of %d.'",
              bj[i].name,cardsym[k],bj[i].num);
            act(buf,TRUE,ch,0,0,TO_ROOM);
            if(bj[i].num > 21){
              sprintf(buf,"$n says '%s is busted.'",bj[i].name);
              act(buf,TRUE,ch,0,0,TO_ROOM);
            }
          }
        }
        break;
      case  8:   /* Wait for more hits */
        break;
      case  9:   /* Complete dealer hand */
        do {
          dc=number(0,12);
          dct+=cardval[dc];
          if(dc==0) ++dflag;
          if((dct > 21) && (dflag > 0)){
            dct-=10; --dflag;
          }
          sprintf(buf,"$n gets the <%c> for a total of %d.",cardsym[dc],dct);
          act(buf,TRUE,ch,0,0,TO_ROOM);
        } while(dct < 17);
        if(dct > 21){
          act("$n grumbles 'Everybody not busted wins.'",
            TRUE,ch,0,0,TO_ROOM);
          for(i=0;i<nbets;++i){
            if(bj[i].num <= 21){
              if(!(tmp=get_char_room_vis(ch,bj[i].name)))
                continue;
              amt = bj[i].amt;
              sprintf(buf,"$n says '$N wins %d.'",amt);
              act(buf,TRUE,ch,0,tmp,TO_ROOM);
              amt <<= 1;
              GET_GOLD(tmp) += amt;
              GET_GOLD(ch ) -= amt;
            }
          }
        } else {
          for(i=0;i<nbets;++i){
            if((bj[i].num <= 21)&&(bj[i].num > dct)){
              if(!(tmp=get_char_room_vis(ch,bj[i].name)))
                continue;
              amt = bj[i].amt;
              sprintf(buf,"$n says '$N wins %d.'",amt);
              act(buf,TRUE,ch,0,tmp,TO_ROOM);
              amt <<= 1;
              GET_GOLD(tmp) += amt; GET_GOLD(ch ) -= amt;
            } else if(bj[i].num == dct){
              if(!(tmp=get_char_room_vis(ch,bj[i].name)))
                continue;
              amt = bj[i].amt;
              act("$n says '$N ties me.'",TRUE,ch,0,tmp,TO_ROOM);
              GET_GOLD(tmp) += amt; GET_GOLD(ch ) -= amt;
            } else {
              if(!(tmp=get_char_room_vis(ch,bj[i].name)))
                continue;
              amt = bj[i].amt;
              sprintf(buf,"$n says '$N loses %d.'",amt);
              act(buf,TRUE,ch,0,tmp,TO_ROOM);
            }
          }
        }
        nbets=0;
        break;
      case 10:
          act("$n shuffles the cards.",TRUE,ch,0,0,TO_ROOM);
          break;
    }    
    ++state;
    if(state > BJLASTSTATE) state=0;
    return(FALSE);
  }
  if(cmd == CMD_BET){
    for(tmp=world[ch->in_room].people; tmp; tmp=tmp->next_in_room)
      if(mob_index[tmp->nr].func == dealer)
        break;
    if(!tmp){
      send_to_char("Sorry, the dealer seems to have left.\n\r",ch);
      return(TRUE);
    }
    if(!CAN_SEE(tmp,ch)){
      act("$n mumbles something about invisible players...",
        TRUE,tmp,0,0,TO_ROOM);
      return(TRUE);
    }
    if(state >= CLOSESTATE){
      act("$n politely informs you that the table is closed.",
        TRUE,tmp,0,ch,TO_VICT);
      return(TRUE);
    }
    for(i=0;i<nbets;++i)
      if(!strcmp(bj[i].name,GET_NAME(ch))){
        act("$n reminds you that one hand per round is enough.",
          TRUE,tmp,0,ch,TO_VICT);
        return(TRUE);
      }
    arg=one_argument(arg,amts);
    if(! amts[0]){ send_to_char("Bet how much?\n\r",ch); return(TRUE);}
    amt=atoi(amts);
    if((amt <= 0) || (amt > 1000000)){
      act("$n says '$N, bets must be between 1 and 1 million coins.'",
        TRUE,tmp,0,ch,TO_ROOM);
      return(TRUE);
    }
    if(amt > GET_GOLD(ch)){
      act("$n says '$N seems to want to bet more that $E has.'",
        TRUE,tmp,0,ch,TO_ROOM);
      return(TRUE);
    }
    sprintf(buf,"$n says '$N bets %d coins.'",amt);
    act(buf,TRUE,tmp,0,ch,TO_ROOM);
    strcpy(bj[nbets].name,GET_NAME(ch));
    bj[nbets].amt=amt; bj[nbets].num=0; bj[nbets].flag=0; bj[nbets].hit=0;
    if((++nbets) >= BJMAX){
      state=CLOSESTATE;
      act("$n says 'The table is now closed, sorry folks.'",
        TRUE,tmp,0,0,TO_ROOM);
    }
    GET_GOLD(ch) -= amt;
    GET_GOLD(tmp) += amt;
    WAIT_STATE(ch,PULSE_VIOLENCE);
    return(TRUE);
  }
  if(cmd == CMD_CARD){
    if((state < HITSTATE) || (state > (HITSTATE+1))){
      send_to_char("Not now, friend.\n\r",ch);
      return(TRUE);
    }
    for(tmp=world[ch->in_room].people; tmp; tmp=tmp->next_in_room)
      if(mob_index[tmp->nr].func == dealer)
        break;
    if(!tmp){
      send_to_char("Sorry, the dealer seems to have left.\n\r",ch);
      return(TRUE);
    }
    if(!CAN_SEE(tmp,ch)){
      act("$n mumbles something about invisible players...",
        TRUE,tmp,0,0,TO_ROOM);
      return(TRUE);
    }
    for(i=0;i<nbets;++i){
      if(strcmp(GET_NAME(ch),bj[i].name)==0)
        break;
    }
    if(i==nbets){
      send_to_char("The dealer reminds you that you're not in this hand.\n\r",
        ch);
      return(TRUE);
    }
    if(bj[i].num > 21){
      send_to_char("The dealer informs you that you're already busted.\n\r",
        ch);
      return(TRUE);
    }
    if(bj[i].num == 21){
      send_to_char("The dealer reminds you that you already have 21.\n\r",
        ch);
      return(TRUE);
    }
    bj[i].hit=1;
    state=HITSTATE;
    return(TRUE);
  }
  return(FALSE);
}
/*   POKER   */

#ifndef CMD_BET
#define CMD_BET  235
#endif
#define CMD_CARD 247
#define CMD_FOLD 249

#define POK_NOBETTOR   (-1)
#define POK_SOLOBETTOR (-2)
#define POK_ROUNDOVER  (-3)

#define POK_OPENBET 100
#define POK_MAXBET 100000

int maxpokerbet=POK_MAXBET;

struct poker_data {
   char name[32];
   char hand[5],rank[5],suit[5];
   int bet,flag;
};

#define POKERMAX 10
struct poker_data pok[POKERMAX];

char pokerdeck[52];

static char *suitname[]={
   "clubs","diamonds","hearts","spades"
};
static char *rankname[]={
   "2","3","4","5","6","7","8","9","10","jack","queen","king","ace"
};
static char *suitchar="CDHS";
static char *rankchar="23456789TJQKA";

int hand_evaluator(struct poker_data *p, char *mess)
{
  char tr[13],ts[4];
  int i,j,k,rr,rmax=1,wrmax,hirk=0,lork=12,ndr=0,flush=1;

  for(i=0;i<13;++i) tr[i]=0;
  for(i=0;i< 4;++i) ts[i]=0;
  for(i=0;i<5;++i){
    j=p->rank[i];
    if(j > hirk) hirk=j;
    if(j < lork) lork=j;
    if(tr[j]==0) ++ndr;
    ++tr[j];
    if(tr[j] > rmax){
      rmax=tr[j]; wrmax=j;
    }
    if(flush){
      j=p->suit[i];
      if(i==0){
        ++ts[j];
      } else {
        if(ts[j])
          ++ts[j];
        else
          flush=0;
      }
    }
  }
  rr=hirk-lork;
  if(flush){
    if(rr==4){
      strcpy(mess,"a straight Flush");
      return(2000000+hirk);
    } else {
      strcpy(mess,"Flush");
      return(50000+hirk);
    }
  } else {
    if(ndr==5){
      if(rr==4){
        strcpy(mess,"a straight");
        return(25000+hirk);
      } else {
        strcpy(mess,"Nothing");
        k=0; j=1;
        for(i=0;i<13;++i){
           if(tr[i])
              k+=j;
           j>>=1;
        }
        return(2+k+13*hirk);
      }
    }
    if(rmax==4){
      sprintf(mess,"Four %ss",rankname[wrmax]);
      return(500000+wrmax);
    }
    if(rmax==3){
      if(ndr==2){
        strcpy(mess,"Full House");
        return(100000+wrmax);
      } else {
        sprintf(mess,"Three %ss",rankname[wrmax]);
        return(15000+wrmax);
      }
    }
    if(rmax==2){
      if(ndr==3){
        for(i=j=k=0;i<13;++i)
          if(tr[i]==2){
            j=i; k+=i;
          }
        for(i=12;tr[i]!=1;--i);
        sprintf(mess,"Two pairs, %ss and %ss",rankname[j],rankname[k-j]);
        return(5000+169*j+13*(k-j)+i);
      } else {
        for(i=12;tr[i]!=1;--i);
        sprintf(mess,"A pair of %ss",rankname[wrmax]);
        return(1000+13*wrmax+i);
      }
    }
    strcpy(mess,"Ooops");
    return(1);
  }
  return(number(1,10));
}
int poker(struct char_data *ch, int cmd, char *arg)
{
  struct char_data *tmp,*vict;
  static int ticker=0,state=0,nbets=0,deckptr=0,ncards=0;
  static int rdswaited=0,whobets=0,pot=0,totbet=0;
  int i,j,k,l,amt,besthand,winner,amtbet,needstobet;
  char buf[128],buf2[128];

  if(cmd==156) return(TRUE);
  if(!cmd){
    if(ch->specials.fighting){
      for(tmp=world[ch->in_room].people; tmp; tmp=tmp->next_in_room)
        if(IS_NPC(tmp) && !tmp->specials.fighting){
          damage(tmp,ch->specials.fighting,1000,SKILL_KICK);
          break;
        }
    }
    ticker=1-ticker;
    if(ticker)
      return(FALSE);
    switch(state){
      case  0:
      case  1:
      case  2:
      case  3:
        if(!(state & 1))
          act("$n says 'Place your bets now for the next hand.'",
            TRUE,ch,0,0,TO_ROOM);
        ++state; return;
      case  4:
        if(!nbets){    /* Nobody playing, go back to beginning. */
          state=0;
          return;
        }
        if(nbets==1){
          act("$n says 'Still waiting for some competition.'",
            TRUE,ch,0,0,TO_ROOM);
          return(TRUE);
        }
        act("$n says 'Well folks, the table is closed.'",
          TRUE,ch,0,0,TO_ROOM);
        for(i=0;i<52;++i) pokerdeck[i]=i;    /* Shuffle */
        for(i=0;i<52;++i){
          j=number(0,51);
          k=pokerdeck[i]; pokerdeck[i]=pokerdeck[j]; pokerdeck[j]=k;
        }
        deckptr=0; ++state;
        return(TRUE);
      case  5:
        for(i=0;i<nbets;++i){
          k=pokerdeck[deckptr++];
          pok[i].hand[0]=k;
          pok[i].rank[0]=(k>>2);
          pok[i].suit[0]=(k&3);
          pok[i].flag=1;
        }
        act("$n deals one card face down to everyone.",TRUE,ch,0,0,TO_ROOM);
        act("$n says 'You can BET, when it's your turn to bet.'",
          TRUE,ch,0,0,TO_ROOM);
        act("$n says 'You can CARD, to see your cards.'",
          TRUE,ch,0,0,TO_ROOM);
        whobets=POK_ROUNDOVER; ncards=1; ++state;
      case 6:
        if(whobets >= 0){
          ++rdswaited;
          if(rdswaited == 6){
            sprintf(buf,"$n says 'Looks like %s is folding.'",
              pok[whobets].name);
            act(buf,TRUE,ch,0,0,TO_ROOM);
            pok[whobets].flag=0;
            rdswaited=0;
            whobets=nextbettor(pok,totbet,whobets,nbets);
            return(TRUE);
          } else {
            sprintf(buf,"$n says 'OK %s, %d coins to you.'",
              pok[whobets].name,totbet-pok[whobets].bet);
            act(buf,TRUE,ch,0,0,TO_ROOM);
            return(TRUE);
          }
        }
        if(whobets == POK_NOBETTOR){
          act("$n says 'Well, everybody's out.'",TRUE,ch,0,0,TO_ROOM);
          state=0; pot=0; nbets=0;
          return(TRUE);
        }
        if((whobets != POK_SOLOBETTOR) && (ncards < 5)){
          for(i=0;i<nbets;++i){
            pok[i].bet=0;
            if(!pok[i].flag) continue;
            j=pokerdeck[deckptr++];
            pok[i].hand[ncards]=j;
            pok[i].rank[ncards]=k=(j>>2);
            pok[i].suit[ncards]=l=(j&3);
            sprintf(buf,"$n says '%s gets the %s of %s.'",
              pok[i].name,rankname[k],suitname[l]);
            act(buf,TRUE,ch,0,0,TO_ROOM);
          }
          totbet=POK_OPENBET;
          ++ncards; whobets=0; rdswaited=0;
          if(!pok[whobets].flag)
            whobets=nextbettor(pok,totbet,whobets,nbets);
          return(TRUE);
        }
        besthand=0;
        for(i=0;i<nbets;++i){
          if(!pok[i].flag) continue;
          j=(whobets == POK_SOLOBETTOR) ? 1 : hand_evaluator(&pok[i],buf2);
          if(j > besthand){
            besthand=j;
            winner=i;
          }
          if(whobets != POK_SOLOBETTOR){
            sprintf(buf,"$n says '%s has %s.'",pok[i].name,buf2);
            act(buf,TRUE,ch,0,0,TO_ROOM);
          }
        }
        if(besthand==0){
          act("$n says 'Hmmm, nobody left to claim the pot.'",
            TRUE,ch,0,0,TO_ROOM);
        } else {
          for(tmp=world[ch->in_room].people; tmp; tmp=tmp->next_in_room)
            if(!strcmp(pok[winner].name,GET_NAME(tmp)))
              break;
          if(!tmp){
            act("$n says 'Hmmm, nobody wins, very confusing.'",
              TRUE,ch,0,0,TO_ROOM);
          } else {
            sprintf(buf,"$n says '%s wins the %d coin pot.'",
              pok[winner].name,pot);
            act(buf,TRUE,ch,0,0,TO_ROOM);
            sprintf(buf,"The winning hand:");
            for(i=0;i<ncards;++i){
              sprintf(buf2," %c%c",
                rankchar[pok[winner].rank[i]],suitchar[pok[winner].suit[i]]);
              strcat(buf,buf2);
            }
            strcat(buf,".\n\r");
            act(buf,TRUE,ch,0,0,TO_ROOM);
            GET_GOLD(tmp) += pot;
          }
        }
        state=0; pot=0; nbets=0; return(TRUE);
    }
  }
  if(cmd == CMD_BET){
    if(state==5) {
      send_to_char("You remember that it's not time to bet.\n\r",ch);
      return(TRUE);
    }
    for(tmp=world[ch->in_room].people; tmp; tmp=tmp->next_in_room)
      if(mob_index[tmp->nr].func == poker)
        break;
    if(!tmp){
      send_to_char("Sorry, the dealer seems to have left.\n\r",ch);
      return(TRUE);
    }
    if(!CAN_SEE(tmp,ch)){
      act("$n mumbles something about invisible players...",
        TRUE,tmp,0,0,TO_ROOM);
      return(TRUE);
    }
    if(state <= 4){
      for(i=0;i<nbets;++i)
        if(!strcmp(pok[i].name,GET_NAME(ch))){
          act("$n reminds you that one hand per round is enough.",
            TRUE,tmp,0,ch,TO_VICT);
          return(TRUE);
        }
      if(nbets >= POKERMAX){
        act("$n says 'Sorry, the table is full.'",TRUE,tmp,0,0,TO_ROOM);
        return(TRUE);
      }
      if(POK_OPENBET > GET_GOLD(ch)){
        act("$n says '$N doesn't have enough cash to play.'",
          TRUE,tmp,0,ch,TO_ROOM);
        return(TRUE);
      }
      sprintf(buf,"$n says '$N bets %d coins.'",POK_OPENBET);
      act(buf,TRUE,tmp,0,ch,TO_ROOM);
      strcpy(pok[nbets].name,GET_NAME(ch));
      ++nbets;
      if(nbets >= POKERMAX){
        state=4;
        act("$n says 'The table is now closed, sorry folks.'",
          TRUE,tmp,0,0,TO_ROOM);
      }
      GET_GOLD(ch) -= POK_OPENBET;
      pot += POK_OPENBET;
      return(TRUE);
    } else {
      if((whobets < 0) || strcmp(pok[whobets].name,GET_NAME(ch))){
        send_to_char("It's not your turn to bet.\n\r",ch);
        return(TRUE);
      }
      needstobet=totbet-pok[whobets].bet;
      arg=one_argument(arg,buf);
      if(!isdigit(buf[0]))
        amtbet=needstobet;
      else {
        amtbet=atoi(buf);
        if(amtbet < needstobet){
          sprintf(buf,"$n says '%s needs to bet %d coins.'",
            GET_NAME(ch),needstobet);
          act(buf,TRUE,tmp,0,0,TO_ROOM);
          return(TRUE);
        }
        if((amtbet == 0) || (amtbet > (needstobet+maxpokerbet))){
          sprintf(buf,"$n says 'This table has a %d coin limit.'",
            maxpokerbet);
          act(buf,TRUE,tmp,0,0,TO_ROOM);
          return(TRUE);
        }
      }
      if(amtbet > GET_GOLD(ch)){
        act("$n says '$N doesn't have enough cash to continue.'",
          TRUE,tmp,0,ch,TO_ROOM);
        pok[whobets].flag=0;
        rdswaited=0;
        whobets=nextbettor(pok,totbet,whobets,nbets);
        return(TRUE);
      }
      pok[whobets].bet+=amtbet;
      GET_GOLD(ch) -= amtbet; pot += amtbet;
      if(amtbet > needstobet){
        totbet+=(amtbet-needstobet);
        sprintf(buf,"$n says '%s bets %d and raises %d.'",
          GET_NAME(ch),needstobet,amtbet-needstobet);
      } else
        sprintf(buf,"$n says '%s bets %d.'",GET_NAME(ch),amtbet);
      act(buf,TRUE,tmp,0,0,TO_ROOM);
      rdswaited=0;
      whobets=nextbettor(pok,totbet,whobets,nbets);
      return(TRUE);
    }
  }
  if(cmd == CMD_CARD){
    if(state < 6){
      send_to_char("Not yet.\n\r",ch);
      return(TRUE);
    }
    for(k=0;k<nbets;++k)
      if(strcmp(GET_NAME(ch),pok[k].name)==0)
        break;
    send_to_char("The Table:\n\r",ch);
    for(i=0;i<nbets;++i){
      if(!pok[i].flag) continue;
      sprintf(buf,"%-15s: ",pok[i].name);
      if(i==k){
        sprintf(buf2," %c%c",
          rankchar[pok[k].rank[0]],suitchar[pok[k].suit[0]]);
      } else {
        sprintf(buf2," ??");
      }
      strcat(buf,buf2);
      for(j=1;j<ncards;++j){
        sprintf(buf2," %c%c",
          rankchar[pok[i].rank[j]],suitchar[pok[i].suit[j]]);
        strcat(buf,buf2);
      }
      strcat(buf,".\n\r");
      send_to_char(buf,ch);
    }
    if((k < nbets)&&(pok[k].flag)){
      sprintf(buf,"You have bet a total of %d/%d coins this round.\n\r",
        pok[k].bet,totbet);
      send_to_char(buf,ch);
    }
    return(TRUE);
  }
  if(cmd == CMD_FOLD){
    if((state < 6)||(whobets < 0)||strcmp(pok[whobets].name,GET_NAME(ch))){
      send_to_char("Not now.\n\r",ch);
      return(TRUE);
    }
    for(tmp=world[ch->in_room].people; tmp; tmp=tmp->next_in_room)
      if(mob_index[tmp->nr].func == poker)
        break;
    if(!tmp){
      send_to_char("Sorry, the dealer seems to have left.\n\r",ch);
      return(TRUE);
    }
    pok[whobets].flag=0;
    sprintf(buf,"$n says '%s folds.'",pok[whobets].name);
    act(buf,TRUE,tmp,0,0,TO_ROOM);
    whobets=nextbettor(pok,totbet,whobets,nbets);
    rdswaited=0;
    return(TRUE);
  }
  return(FALSE);
}
int nextbettor(struct poker_data pok[], int totbet, int whobets, int nbets)
{
  int i,j,k;

  k=0;
  do {
    ++k;
    whobets=(whobets+1)%nbets;
  } while(!pok[whobets].flag && (k < nbets));
  if(k==nbets)
    return(pok[whobets].flag ? POK_SOLOBETTOR : POK_NOBETTOR);
  if(pok[whobets].bet == totbet)
    return(POK_ROUNDOVER);
  return(whobets);
}
