/* 
	rank.c for FredMUD
	by Slug
	November 1996

*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "limits.h"

/* extern vars */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct index_data *mob_index;
extern int bytetotal[];
extern int reboottime;
extern struct player_index_element *player_table;


/* extern functions */

struct time_info_data age(struct char_data *ch);


#ifdef NEEDS_STRDUP
char *strdup(char *source);
#endif


#define GET_PKILLS(ch)   ((ch)->points.pkills)
#define GET_PDEATHS(ch)   ((ch)->points.pdeaths)
#define MAX_RANKED 128
#define GET_KILLS(ch)    ((ch)->points.kills)
#define GET_DEATHS(ch)    ((ch)->points.deaths)
#define GET_KEY(cc,tk) ((*((tk)->function))(cc))

typedef char *ranktype;

struct key_data {
	char *keystring;
	char *outstring;
	ranktype (*function)(struct char_data *ch);
	int usage;
	struct key_data *next;
};

struct rank_data {
	struct char_data *ch;
	char key[80];
};
ranktype rank_gut(struct char_data *ch);
ranktype rank_food(struct char_data *ch);
ranktype rank_drink(struct char_data *ch);
ranktype rank_drunk(struct char_data *ch);
ranktype rank_hp(struct char_data *ch);
ranktype rank_mana(struct char_data *ch);
ranktype rank_moves(struct char_data *ch);
 ranktype rank_hpr(struct char_data *ch);
 ranktype rank_manar(struct char_data *ch);
 ranktype rank_movesr(struct char_data *ch);
 ranktype rank_str(struct char_data *ch);
 ranktype rank_int(struct char_data *ch);
 ranktype rank_wis(struct char_data *ch);
 ranktype rank_dex(struct char_data *ch);
 ranktype rank_con(struct char_data *ch);
 ranktype rank_fitness(struct char_data *ch);
 ranktype rank_hitroll(struct char_data *ch);
 ranktype rank_damroll(struct char_data *ch);
 ranktype rank_armor(struct char_data *ch);
 ranktype rank_mr(struct char_data *ch);
 ranktype rank_meta(struct char_data *ch);
 ranktype rank_mb(struct char_data *ch);
 ranktype rank_xp(struct char_data *ch);
 ranktype rank_height(struct char_data *ch);
 ranktype rank_weight(struct char_data *ch);
 ranktype rank_fatness(struct char_data *ch);
 ranktype rank_coolness(struct char_data *ch);
 ranktype rank_input(struct char_data *ch);
 ranktype rank_output(struct char_data *ch);
 ranktype rank_ipm(struct char_data *ch);
 ranktype rank_opm(struct char_data *ch);
 ranktype rank_motion(struct char_data *ch);
 ranktype rank_speed(struct char_data *ch);
 ranktype rank_gold(struct char_data *ch);
 ranktype rank_bank(struct char_data *ch);
 ranktype rank_age(struct char_data *ch);
 ranktype rank_days(struct char_data *ch);
 ranktype rank_kills(struct char_data *ch);
 ranktype rank_deaths(struct char_data *ch);
 ranktype rank_kd(struct char_data *ch);
 ranktype rank_kh(struct char_data *ch);
 ranktype rank_practices(struct char_data *ch);
 ranktype rank_boner(struct char_data *ch);
 ranktype rank_pow(struct char_data *ch);
 ranktype rank_pkills(struct char_data *ch);
 ranktype rank_pdeaths(struct char_data *ch);
 ranktype rank_pkd(struct char_data *ch);
 ranktype rank_xpt(struct char_data *ch); 
 ranktype rank_xptm(struct char_data *ch);
 ranktype rank_bonk(struct char_data *ch);
 ranktype rank_argle(struct char_data *ch);
 ranktype rank_time(struct char_data *ch);
 ranktype rank_sickness(struct char_data *ch);
 ranktype rank_commands(struct char_data *ch);
 ranktype rank_cph(struct char_data *ch);

int rank_compare_top(const void *n1, const void *n2);
int rank_compare_bot(const void *n1, const void *n2);
int char_compare(const void *n1, const void *n2);

void eat_spaces(char **source);

void init_keys(void);

void add_key(char *key,char *out,ranktype (*f)(struct char_data *ch));
struct key_data *search_key(char *key);

struct key_data *key_list = NULL;
struct key_data *key_list_tail = NULL;
struct key_data *the_key = NULL;

int maxkeylength = 5;	/* used for formatting in rankhelp */
int maxoutlength = 5;

int keys_inited = FALSE;
int KEY_PRINTING = FALSE;	/* used by rank functions sometimes */
int KEY_SPAMMING = FALSE;

char kbuf[80];		/* used by rank functions */
char xbuf[50]; /* used for to_long_long output */
char *kbp;
int mud_birth;

struct rank_data tt[MAX_RANKED];
void do_slug_rank(struct char_data *ch, char *arg, int cmd)
{
    int i,j,k;
	int rk;
    int toprank;
    char buffer[MAX_STRING_LENGTH];
    char keybuf[MAX_STRING_LENGTH], nbuf[MAX_STRING_LENGTH];
    char nbufnp[MAX_STRING_LENGTH];
    struct char_data *c, *tc;
    struct descriptor_data *d;
    char *nbufp;
	struct key_data *tk;
    struct char_file_u tmp_store;

 int load_char(char *name, struct char_file_u *char_element);

if (!keys_inited) {
	init_keys();
	keys_inited = TRUE;

/*
i= load_char(player_table->name,&tmp_store);
mud_birth = tmp_store.birth;
*/
	
}
    if (IS_NPC(ch))
	return;

    while (arg[0] == ' ')
	arg++;
    half_chop(arg, keybuf, nbuf);
    nbufp = nbuf;

if(!strlen(keybuf)) strcpy(keybuf,"help");
the_key = search_key(keybuf);
if (the_key == NULL) {
send_to_char("Please specify a valid key. Type \"rank help\" for instructions.\n\r",ch);

return;
}
the_key->usage++;

if (!strcmp(the_key->keystring,"frequency")) {
send_to_char("Unimplemented Rank Key\n\r",ch);
}
else
if (!strcmp(the_key->keystring,"usage")) {
buffer[0] = '\0';
j = 80 / (maxkeylength+9);
i=0;
send_to_char("Key Usage Statistics.\n\r", ch);
        for(tk=key_list;tk;tk=tk->next) {
		sprintf(kbuf,"%*s: %6d ",maxkeylength,tk->keystring,tk->usage);
                strcat(buffer,kbuf);
		if (++i % j == 0) strcat(buffer,"\n\r");        
	}
strcat(buffer,"\n\r");
send_to_char(buffer, ch);

}
else
if (!strcmp(the_key->keystring,"debug")) {
strcpy(buffer,"Experience data for each tick.\n\r");
for(i=0;i<MAX_META_SAMPLES;i++) {
to_long_long(xbuf,(long long)ch->specials.metadata[i]);
sprintf(kbuf,"%3d: %20s",i,xbuf);
	/*
sprintf(kbuf,"%3d: %20d",i,ch->specials.metadata[i]);
*/
	strcat(buffer,kbuf);
	if (i%3 == 2) strcat(buffer,"\n\r");
}
strcat(buffer,"\n\r");
send_to_char(buffer,ch);
to_long_long(xbuf,(long long)ch->specials.metadata[MAX_META_SAMPLES]);
sprintf(buffer,"Currently on tick #%s\n\r",xbuf);
send_to_char(buffer,ch);
} /* end debug */
else
if (!strcmp(the_key->keystring,"spam")) {

	send_to_char("Valid keys and your values.\n\r", ch);
	KEY_SPAMMING = KEY_PRINTING = TRUE;
	k=FALSE;	/* found first hidden key yet? */
	for(tk=key_list;(tk && !k);tk=tk->next) {
	if(!strcmp(tk->keystring,"boner")) k=1;
	else
 	if (tk->function != NULL) {
		strcpy(kbuf,GET_KEY(ch,tk));
		for(kbp=kbuf;*kbp == ' ';kbp++);

		sprintf(buffer, "%*s = %*s = %*s\n\r",maxkeylength,tk->keystring,		maxoutlength,tk->outstring,
		18,kbp);
		send_to_char(buffer, ch);
	}
	}
	KEY_SPAMMING = KEY_PRINTING = FALSE;
}
else
if (!strcmp(the_key->keystring,"help")) {
send_to_char("Valid keys:\n\r", ch);
j = 80 / (maxkeylength+1);
i = 0;
buffer[0] = '\0';
k=FALSE;	/* reached first hidden key */

for(tk=key_list;(tk && !k);tk=tk->next) {
if(!strcmp(tk->keystring,"boner")) k=1;
else {
sprintf(kbuf,"%*s ",maxkeylength,tk->keystring);
strcat(buffer,kbuf);
if (++i % j == 0) strcat(buffer,"\n\r");
}
}
strcat(buffer,"\n\r");
strcat(buffer,"Usage: rank <key> [+-][max]\n\r");
send_to_char(buffer,ch);
}
else
{
    k = 0;
    for (d = descriptor_list; d; d = d->next) {
        if (!d->connected) {
            c = d->original ? d->original : d->character;
            if (CAN_SEE(ch, c)) {
                if (k) {
                    if (k < MAX_RANKED) {
                        tt[k].ch = c;
                        k++;
                    } else {    /*list is full */
                        tt[k].ch = c;
                    }
                } else {        /* first player in list */
                    tt[k].ch = c;
                    k++;
                }
            }
        }
    }
	toprank = TRUE;
	if (!strcmp(the_key->keystring,"deaths"))
	    toprank = FALSE;
	if (*nbufp == '-') {
	    toprank = FALSE;
	    nbufp++;
	} else if (*nbufp == '+') {
	    toprank = TRUE;
	    nbufp++;
	}
	rk = atoi(nbufp);
	if (rk == 0)
	    rk = 20;
	if (k < rk)
	    rk = k;
for(i=0;i<k;i++)
strcpy(tt[i].key,GET_KEY(tt[i].ch,the_key));

/* sort */
	if (toprank)
	    qsort(tt, k, sizeof(struct rank_data), rank_compare_top);
	else
	    qsort(tt, k, sizeof(struct rank_data), rank_compare_bot);

/*print out */
KEY_PRINTING = TRUE;
	sprintf(buffer, "%s %d players ranked by %s.\n\r", toprank ? "Top" : "Bottom", rk, the_key->outstring);
	send_to_char(buffer, ch);
	for (i = 0; i < rk; i++) {
	
	strcpy(kbuf,GET_KEY(tt[i].ch,the_key));
	for(kbp=kbuf;*kbp == ' ';kbp++);
	sprintf(buffer, "#%2d: %15s %*s\n\r", i + 1, GET_NAME(tt[i].ch),18, kbp);
	    send_to_char(buffer, ch);
	}
	send_to_char("OK.\n\r", ch);
    }

}

void init_keys(void) {

/* put the add_keys in the order you want them displayed in help */

 add_key("hp","hit points",rank_hp);
 add_key("mana","mana points",rank_mana);
 add_key("moves","move points",rank_moves);
 add_key("powerpoints","powerpoints",rank_pow);
 add_key("hpr","hit point regen rate",rank_hpr);
 add_key("manar","mana regen rate",rank_manar);
 add_key("movesr","move point regen rate",rank_movesr);
 add_key("strength", "strength",rank_str);
 add_key("intel","intelligence",rank_int);
 add_key("wisdom", "wisdom",rank_wis);
 add_key("dexterity","dexterity",rank_dex);
 add_key("const","constitution",rank_con);
 add_key("fitness", "sum of stats",rank_fitness);
 add_key("hitroll","hit bonus",rank_hitroll);
 add_key("damroll", "damage bonus",rank_damroll);
 add_key("armor","armor",rank_armor);
 add_key("mr", "magic resistance",rank_mr);
 add_key("meta","meta points",rank_meta);
 add_key("mbonus","meta bonus",rank_mb);
 add_key("xp","experience points",rank_xp);
 add_key("xpt","average xp/tick last 100 ticks",rank_xpt);
 add_key("xptmax","max xp/tick last 100 ticks",rank_xptm);
 add_key("height","height",rank_height);
 add_key("weight","weight",rank_weight);
 add_key("fatness","fatness",rank_fatness);
 add_key("coolness","coolness",rank_coolness);
 add_key("input","bytes sent by player",rank_input);
 add_key("output","bytes sent to player",rank_output);
 add_key("ipm","input bytes per minute",rank_ipm);
 add_key("opm","output bytes per minute",rank_opm);
 add_key("motion","rooms passed through",rank_motion);
 add_key("speed","rooms per minute",rank_speed);
 add_key("bonk","walls impacted",rank_bonk);
 add_key("argle","typos",rank_argle);
 add_key("commands","successful commands",rank_commands);
 add_key("cph","commands per hour",rank_cph); 
 add_key("gold","gold carried",rank_gold);
 add_key("bank","bank balance",rank_bank);
 add_key("age","age",rank_age);
 add_key("days","days played",rank_days);
 add_key("time","connect time this boot",rank_time);
 add_key("sickness","addiction factor",rank_sickness);
 add_key("kills","kills",rank_kills);
 add_key("deaths","deaths",rank_deaths);
 add_key("kd","kills to deaths ratio",rank_kd);
 add_key("kph","kills per hour",rank_kh);
 add_key("practices","practices",rank_practices);
 add_key("gut","stomach capacity",rank_gut);
 add_key("food","units of food in gut",rank_food);
 add_key("drink","units of liquid in gut",rank_drink);
 add_key("drunk","alcohol content",rank_drunk);
 /* NOT IN UNTIL PFILE CAN BE REVISED
 add_key("pkills","player kills",rank_pkills);
 add_key("pdeaths","player deaths",rank_pdeaths);
 add_key("pkd","player kd ratio",rank_pkd);
*/
 
 add_key("help","help",NULL);
 add_key("spam","spam",NULL);

/* any keys including and after "boner" will not show up in help or spam */ 
 add_key("boner","length in millimeters",rank_boner); 
 add_key("debug","debug",NULL);
 add_key("usage","usage",NULL);
 add_key("frequency","frequency",NULL);

} /* end init_keys */

struct key_data *search_key(char *key) {

struct key_data *tk;

if (strlen(key))
for(tk=key_list;tk;tk=tk->next)
if (!strncmp(tk->keystring,key,strlen(key))) return tk;

return NULL;

} /* end search_key */

void add_key(char *key,char *out,ranktype (*f)(struct char_data *ch)) {

struct key_data *tmp_key;

CREATE(tmp_key,struct key_data,1);
tmp_key->keystring = strdup(key);
tmp_key->outstring = strdup(out);
tmp_key->function = f;
tmp_key->usage = 0;
tmp_key->next = NULL;

maxkeylength = MAX(maxkeylength,strlen(key));
maxoutlength = MAX(maxoutlength,strlen(out));

/* append to list */
if (!key_list) key_list = key_list_tail = tmp_key;
else {
key_list_tail->next = tmp_key;
key_list_tail = tmp_key;
}

} /* end add_key */


void eat_spaces(char **source) {
while (**source == ' ') (*source)++;
} /* end eat_spaces */

char k1[80];
char k2[80];

int rank_compare_top(const void *n1, const void *n2)
{

strcpy(k1,(*((struct rank_data *) n1)).key);
strcpy(k2,(*((struct rank_data *) n2)).key);

return(strcmp(k2,k1));
}
int char_compare(const void *n1, const void *n2)
{
    return ((*((char *) n1)) - (*((char *) n2)));
}

int rank_compare_bot(const void *n1, const void *n2)
{

strcpy(k1,(*((struct rank_data *) n1)).key);
strcpy(k2,(*((struct rank_data *) n2)).key);

return(strcmp(k1,k2));
}


/* RANK FUNCTIONS */

ranktype rank_commands(struct char_data *ch) {
 sprintf(kbuf,"%20d",(ch)->specials.commands);
return(kbuf);
} /* end rank_commands */

ranktype rank_cph(struct char_data *ch) {
long secs;
float per;
secs = time(0) - ch->specials.connect_time;

  per = 3600*ch->specials.commands;
  per /= secs;
sprintf(kbuf,"%20.2f",per);
return(kbuf);
} /* end rank_cph */

ranktype rank_time(struct char_data *ch) {
long secs;
float per;
secs = time(0)  - ch->specials.connect_time;

if (KEY_SPAMMING) {
  sprintf(kbuf,"%3d:%02d:%02d",secs / 3600,
        (secs %3600) / 60, (secs % 3600) % 60);
} else
if (KEY_PRINTING) {
  per = secs;
  per = per/(time(0) - reboottime);

  sprintf(kbuf,"%3d:%02d:%02d = %4.2f %% of Running Time",secs / 3600,
	(secs %3600) / 60, (secs % 3600) % 60,   per*100);
}
else
        sprintf(kbuf,"%d",secs);
return(kbuf);
} /* end rank_time */

ranktype rank_sickness(struct char_data *ch) {
long secs;
float per;
secs = ch->player.time.played + time(0) - ch->player.time.logon;

  per = secs;
  per = per/(time(0) - ch->player.time.birth); /*percent real time */
  per *= per*secs/3600; /* times hours played */ 
  per *=100; 
  sprintf(kbuf,"%20.2f",per);
return(kbuf);
} /* end rank_sickness */

ranktype rank_bonk(struct char_data *ch) {
 sprintf(kbuf,"%20d",(ch)->specials.bonks);
return(kbuf);
} /* end rank_bonk */

ranktype rank_argle(struct char_data *ch) {
 sprintf(kbuf,"%20d",(ch)->specials.argles);
return(kbuf);
} /* end rank_argle */

ranktype rank_xpt(struct char_data *ch) {

long long sum=0;
int i;

for(i=0;i<MAX_META_SAMPLES;i++) {
sum += (long long)(ch->specials.metadata[i]);
}
sum /= MIN(MAX_META_SAMPLES,1 + ch->specials.connect_tics);
to_long_long(xbuf,sum);
sprintf(kbuf,"%27s",xbuf);
return(kbuf);

} /* end rank_xpt */

ranktype rank_xptm(struct char_data *ch) {

long long max;
int i;
max = 0;
for(i=0;i<MAX_META_SAMPLES;i++) {
if( (ch->specials.metadata[i]) > max) 
	max = (ch->specials.metadata[i]);
}
to_long_long(xbuf,max);
sprintf(kbuf,"%27s",xbuf);
return(kbuf);
} /* end rank_xptm */

ranktype rank_mb(struct char_data *ch) {
 sprintf(kbuf,"%20d",(ch)->specials.metabonus);
return(kbuf);
} /* end rank_mb */

ranktype rank_pow(struct char_data *ch){
long long sum;
sum = (long long)GET_MAX_HIT(ch) + 
	(long long)GET_MAX_MANA(ch) + 
	(long long)GET_MAX_MOVE(ch);
to_long_long(xbuf,sum);
sprintf(kbuf,"%27s",xbuf);
return(kbuf);
} /* end rank_pow */

/*
ranktype rank_pkills(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_PKILLS(ch));
return(kbuf);
}

ranktype rank_pdeaths(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_PDEATHS(ch));
return(kbuf);
}

 ranktype rank_pkd(struct char_data *ch) {
float rf;
rf = GET_PKILLS(ch);
sprintf(kbuf,"%20.2f",GET_PDEATHS(ch) ? rf / GET_PDEATHS(ch) : 
rf);
return(kbuf);
}
*/

ranktype rank_hp(struct char_data *ch) {
long long sum;
sum = (long long)GET_MAX_HIT(ch); 
to_long_long(xbuf,sum);
sprintf(kbuf,"%27s",xbuf);
return(kbuf);
} /* end rank_hp */

ranktype rank_mana(struct char_data *ch) {
long long sum;
sum = (long long)GET_MAX_MANA(ch);
to_long_long(xbuf,sum);
sprintf(kbuf,"%27s",xbuf);
return(kbuf);
} /* end rank_mana */

ranktype rank_moves(struct char_data *ch) {
long long sum;
sum = (long long)GET_MAX_MOVE(ch);
to_long_long(xbuf,sum);
sprintf(kbuf,"%27s",xbuf);
return(kbuf);
} /* end rank_move */

 ranktype rank_hpr(struct char_data *ch) {
sprintf(kbuf,"%20d",hit_gain(ch));
return(kbuf);
} /* end rank_hpr */

 ranktype rank_manar(struct char_data *ch) {
sprintf(kbuf,"%20d",mana_gain(ch));
return(kbuf);
} /* end rank_manar */

 ranktype rank_movesr(struct char_data *ch) {
sprintf(kbuf,"%20d",move_gain(ch));
return(kbuf);
} /* end rank_movesr */


ranktype rank_str(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_STR(ch));
return(kbuf);
} /* end rank_str */

 ranktype rank_int(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_INT(ch));
return(kbuf);
} /* end rank_int */

 ranktype rank_wis(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_WIS(ch));
return(kbuf);
} /* end rank_wis */

 ranktype rank_dex(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_DEX(ch));
return(kbuf);
} /* end rank_dex */

 ranktype rank_con(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_CON(ch));
return(kbuf);
} /* end rank_con */

 ranktype rank_fitness(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_STR(ch) + GET_INT(ch) + GET_WIS(ch) + GET_DEX(ch) + 
GET_CON(ch));
return(kbuf);
} /* end rank_fitness */

 ranktype rank_hitroll(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_HITROLL(ch));
return(kbuf);
} /* end rank_hitroll */

 ranktype rank_damroll(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_DAMROLL(ch));
return(kbuf);
} /* end rank_damroll */

 ranktype rank_armor(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_AC(ch));
return(kbuf);
} /* end rank_armor */

 ranktype rank_mr(struct char_data *ch) {
sprintf(kbuf,"%20d",(ch)->specials.magres);
return(kbuf);
} /* end rank_mr */

 ranktype rank_meta(struct char_data *ch) {
long long sum;
sum = (long long)GET_META(ch);
to_long_long(xbuf,sum);
sprintf(kbuf,"%27s",xbuf);
return(kbuf);
} /* end rank_meta */

 ranktype rank_xp(struct char_data *ch) {
long long sum;
sum = (long long)GET_EXP(ch);
to_long_long(xbuf,sum);
sprintf(kbuf,"%27s",xbuf);
return(kbuf);
} /* end rank_xp */

 ranktype rank_height(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_HEIGHT(ch));
return(kbuf);
} /* end rank_height */

 ranktype rank_weight(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_WEIGHT(ch));
return(kbuf);
} /* end rank_weight */

 ranktype rank_fatness(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_WEIGHT(ch) * 100 / GET_HEIGHT(ch));
return(kbuf);
} /* end rank_fatness */

 ranktype rank_coolness(struct char_data *ch) {
sprintf(kbuf,"%20d",(GET_LEVEL(ch)==9999) ? 100 : 
((GET_GOLD(ch)+GET_EXP(ch))%100));
return(kbuf);
} /* end rank_coolness */

 ranktype rank_input(struct char_data *ch) {
to_long_long(xbuf,(long long)(ch->specials.inbytes));
sprintf(kbuf,"%27s",xbuf);
return(kbuf);
} /* end rank_input */

 ranktype rank_output(struct char_data *ch) {
to_long_long(xbuf,(long long)((ch)->specials.outbytes));
sprintf(kbuf,"%27s",xbuf);
return(kbuf);
} /* end rank_output */

ranktype rank_ipm(struct char_data *ch) {
long secs;
float ipm;
secs = (long)(time(0)-ch->specials.connect_time);
ipm = ch->specials.inbytes;
ipm *= 60;
ipm /= (secs);
sprintf(kbuf,"%20.2f",ipm);
return(kbuf);

} /* end rank_ipm */

ranktype rank_opm(struct char_data *ch) {
long secs;
float ipm;
secs = (long)(time(0)-ch->specials.connect_time);
ipm = ch->specials.outbytes;
ipm *= 60;
ipm /= (secs);
sprintf(kbuf,"%20.2f",ipm);
return(kbuf);
} /* end rank_opm */
ranktype rank_motion(struct char_data *ch) {
to_long_long(xbuf,(long long)ch->specials.rooms);
sprintf(kbuf,"%27s",xbuf);
return(kbuf);

} /* end rank_motion */

 ranktype rank_speed(struct char_data *ch) {
long secs;
float rpm;
secs = (long)(time(0)-ch->specials.connect_time);
rpm = ch->specials.rooms;
rpm *= 60;
rpm /= (secs);
sprintf(kbuf,"%20.2f",rpm);
return(kbuf);
} /* end rank_speed */


 ranktype rank_gold(struct char_data *ch) {
to_long_long(xbuf,(long long)GET_GOLD(ch));
sprintf(kbuf,"%27s",xbuf);
return(kbuf);
} /* end rank_gold */

 ranktype rank_bank(struct char_data *ch) {
to_long_long(xbuf,ch->points.bank);
sprintf(kbuf,"%27s",xbuf);
return(kbuf);
} /* end rank_bank */

 ranktype rank_age(struct char_data *ch) {
if (KEY_PRINTING)
	sprintf(kbuf,"%3d",(age(ch).year));
else
	sprintf(kbuf,"%12d",-(ch)->player.time.birth);

return(kbuf);
} /* end rank_age */

 ranktype rank_days(struct char_data *ch) {
int secs;
secs = ch->player.time.played + time(0) - ch->player.time.logon;

if (KEY_PRINTING)
sprintf(kbuf,"%3d days, %2d hours",secs / 3600 / 24, 	
(secs / 3600) % 24);
else
	sprintf(kbuf,"%d",secs);
return(kbuf);
} /* end rank_days */

 ranktype rank_kills(struct char_data *ch) {
sprintf(kbuf,"%20d",(ch)->points.kills);
return(kbuf);
} /* end rank_kills */

 ranktype rank_deaths(struct char_data *ch) {
sprintf(kbuf,"%20d",(ch)->points.deaths);
return(kbuf);
} /* end rank_deaths */

 ranktype rank_kd(struct char_data *ch) {
float rf;
rf = GET_KILLS(ch);
sprintf(kbuf,"%20.2f",GET_DEATHS(ch) ? rf / GET_DEATHS(ch) : rf);
return(kbuf);
} /* end rank_kd */

 ranktype rank_kh(struct char_data *ch) {
int secs;
float rf;
secs = ch->player.time.played +time(0) - ch->player.time.logon;
rf = GET_KILLS(ch);
rf *= 3600;
rf /= secs;
sprintf(kbuf,"%20.2f",rf);
return(kbuf);
} /* end rank_kh */

ranktype rank_practices(struct char_data *ch) {
sprintf(kbuf,"%20d",ch->specials.spells_to_learn);
return(kbuf);
} /* end rank_practices */


/* Four stomach ranks */

ranktype rank_gut(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_GUT(ch));
return(kbuf);
}
ranktype rank_food(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_COND(ch,FULL));
return(kbuf);
}
ranktype rank_drink(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_COND(ch,THIRST));
return(kbuf);
}
ranktype rank_drunk(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_COND(ch,DRUNK));
return(kbuf);
}


 ranktype rank_boner(struct char_data *ch) {
sprintf(kbuf,"%20d",GET_STR(ch) * 100 / (GET_INT(ch) ? GET_INT(ch) : 1));
return(kbuf);
} /* end rank_boner */









