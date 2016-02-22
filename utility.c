/* ************************************************************************
*  file: utility.c, Utility module.                       Part of DIKUMUD *
*  Usage: Utility procedures                                              *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
************************************************************************* */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "structs.h"
#include <time.h>
#include "utils.h"

extern struct time_data time_info;


int MIN(int a, int b)
{
  return a < b ? a:b;
}


int MAX(int a, int b)
{
  return a > b ? a:b;
}

/* creates a random number in interval [from;to] */
int number(int from, int to) 
{
  int d;

  d=to-from+1;
  if(d < 2) return(from);
  return((lrand48() % d) + from);
}


/* simulates dice roll */
int dice(int number, int size) 
{
  int r;
  int sum = 0;

  if(size <= 1){
    if(size < 1) return(1);
    return(number);
  }
  for (r = 1; r <= number; r++) sum += ((lrand48() % size)+1);
  return(sum);
}



#ifdef NEEDS_STRDUP
/* Create a duplicate of a string */
char *strdup(char *source)
{
  char *new;

  CREATE(new, char, strlen(source)+1);
  return(strcpy(new, source));
}
#endif


/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different or end of both                 */
int str_cmp(char *arg1, char *arg2)
{
  int chk, i;

  if(!arg1)
    return(!arg2 ? 0 : -1);
  else if(! arg2)
     return(1);
  for (i = 0; *(arg1 + i) || *(arg2 + i); i++)
    if (chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i)))
      if (chk < 0)
        return (-1);
      else 
        return (1);
  return(0);
}



/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different, end of both, or n reached     */
int strn_cmp(char *arg1, char *arg2, int n)
{
  int chk, i;

  for (i = 0; (*(arg1 + i) || *(arg2 + i)) && (n>0); i++, n--)
    if (chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i)))
      if (chk < 0)
        return (-1);
      else 
        return (1);

  return(0);
}



/* writes a string to the log */
void log(char *str)
{
  long ct;
  char *tmstr;

  ct = time(0);
  tmstr = asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  fprintf(stderr, "%s :: %s\n", tmstr, str);
}
  


void sprintbit(long vektor, char *names[], char *result)
{
  long nr;

  *result = '\0';

  for(nr=0; vektor; vektor>>=1)
  {
    if (IS_SET(1, vektor))
      if (*names[nr] != '\n') {
        strcat(result,names[nr]);
        strcat(result," ");
      } else {
        return;
      }
    if (*names[nr] != '\n')
      nr++;
  }
  if (!*result)
    strcat(result,"NOTHING");
}



void sprinttype(int type, char *names[], char *result)
{
  int nr;

  for(nr=0;(*names[nr]!='\n');nr++);
  if(type < nr)
    strcpy(result,names[type]);
  else
    strcpy(result,"UNDEFINED");
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data real_time_passed(time_t t2, time_t t1)
{
  long secs;
  struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs/SECS_PER_REAL_HOUR) % 24;  /* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR*now.hours;

  now.day = (secs/SECS_PER_REAL_DAY);          /* 0..34 days  */
  secs -= SECS_PER_REAL_DAY*now.day;

  now.month = -1;
  now.year  = -1;

  return now;
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs/SECS_PER_MUD_HOUR) % 24;  /* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR*now.hours;

  now.day = (secs/SECS_PER_MUD_DAY) % 35;     /* 0..34 days  */
  secs -= SECS_PER_MUD_DAY*now.day;

  now.month = (secs/SECS_PER_MUD_MONTH) % 17; /* 0..16 months */
  secs -= SECS_PER_MUD_MONTH*now.month;

  now.year = (secs/SECS_PER_MUD_YEAR);        /* 0..XX? years */

  return now;
}

struct time_info_data age(struct char_data *ch)
{
  long secs;
  struct time_info_data player_age;

  player_age = mud_time_passed(time(0),ch->player.time.birth);
/* SLUG_CHANGE 11-17-96 added just next line, age = real_days +17 */
  player_age.year = (time(0) - ch->player.time.birth)/86400;
  player_age.year += 17;   /* All players start at 17 */
  return player_age;
}

#define LLL 30

void to_long_long(char *s, long long n)
{
  int i,j,k;
  int neg;

  if (neg = (n < 0)) n*=-1;

  i=LLL;
  s[i]=0;
  k=0;
  for(;;){
    s[--i]= '0' + (n % 10);
    n /= 10;
    if(n){
      k++;
      if(k==3){
        s[--i]=',';
        k=0;
      }
    } else
      break;
  }
  if(neg) s[--i]='-';
  if(i){
    for(j=0;j<=(LLL-i);j++)
      s[j]=s[i+j];
  }
}
