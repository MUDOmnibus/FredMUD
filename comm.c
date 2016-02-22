
#ifdef SYSV
#include <sys/file.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"

#define DFLT_PORT 2150        /* default port */
#define MAX_NAME_LENGTH 12
#define MAX_HOSTNAME 256
#define OPT_USEC 100000
#define MAXFDALLOWED 200
#define MAXOCLOCK 500

extern int errno;

extern struct room_data *world;
extern int top_of_world;      
extern struct time_info_data time_info; 
extern char help[];
extern int reboottime;

/* local globals */

struct descriptor_data *descriptor_list, *next_to_process;
jmp_buf env;
int s;
int slow_death = 0; 
int shutdowngame = 0;
int shutdownwarn = 0;
int maxdesc, avail_descs;
int tics = 0;
int newsflashflag = 0;
int nonewplayers = 0;
int nonewconnect = 0;
int nostealflag = 1;
int nokillflag = 1;
int noshoutflag = 0;
int mischance = 20;
int hitchance = 80;
int bobflag=0;
int jokecount=10;
int timeoflastconnect = 0;
int pcdeaths=0,mobdeaths=0;
int grand_total = 0;
int melsnumber = -1;
int bytetotal[256];
struct char_data *out_char;	/* SLUG_CHANGE 11-17-96 */

int baddoms;
char baddomain[BADDOMS][BADSTRLEN];
int newbeedoms;
char newbeedomain[BADDOMS][BADSTRLEN];

char *bobname;

struct descriptor_data *xo;
int freq_ct[300];

void logsig(int sig);
void hupsig(int sig);
void shutdown_request(int sig);
void checkpointing(int sig);

int get_from_q(struct txt_q *queue, char *dest);
/* write_to_q is in comm.h for the macro */
int run_the_game(int port);
int game_loop(int s);
int init_socket(int port);
int new_connection(int s);
int new_descriptor(int s);
int process_output(struct descriptor_data *t);
int process_input(struct descriptor_data *t);
void close_sockets(int s);
void close_socket(struct descriptor_data *d);
struct timeval timediff(struct timeval *a, struct timeval *b);
void flush_queues(struct descriptor_data *d);
void nonblock(int s);
void parse_name(struct descriptor_data *desc, char *arg);
void freaky(struct descriptor_data *d);


/* extern fcnts */

struct char_data *make_char(char *name, struct descriptor_data *desc);
void boot_db(void);
void zone_update(void);
void affect_update( void ); /* In spells.c */
void point_update( void );  /* In limits.c */
void free_char(struct char_data *ch);
void log(char *str);
void mobile_activity(void);
void string_add(struct descriptor_data *d, char *str);
void perform_violence(void);
void stop_fighting(struct char_data *ch);
void show_string(struct descriptor_data *d, char *input);
void save_char(struct char_data *ch, sh_int load_room);

/* *********************************************************************
*  main game loop and related stuff               *
********************************************************************* */

int sigct=0;

int main(int argc, char **argv)
{
  int i,port;
  char buf[512];
  char *dir;

  port = DFLT_PORT;
  dir = DFLT_DIR;
  newbeedoms=0;
  baddoms=1;
  strcpy(baddomain[0],"141.109.20.61");
  if (argc > 1) {
    if (!isdigit(*argv[1])) {
      fprintf(stderr, "Usage: %s [ port # ]\n", 
        argv[0]);
      exit(0);
    } else if ((port = atoi(argv[1])) <= 1024) {
      fprintf(stderr,"Illegal port #\n");
      exit(0);
    }
  }
  bobname=(char *)strdup("Bob");
  if (chdir(dir) < 0) {
    perror("chdir");
    exit(0);
  }
  for(i=0;i<256;i++)
    bytetotal[i]=0;
  reboottime=time(0);
  srand48(reboottime);
  sprintf(buf,"Boottime = %d",reboottime);
  log(buf);
  run_the_game(port);
  return(0);
}

int run_the_game(int port)
{
  struct descriptor_data *point,*next_point;
  void signal_setup(void);
  void saveallplayers();

  descriptor_list = NULL;
  signal_setup();
  do {
    s = init_socket(port);
    if(s < 0) exit(0);
  } while(s < 0);
  boot_db();
  log("Entering game loop.");
  game_loop(s);
  saveallplayers();
  close_sockets(s);
  log("Normal termination of game.");
}
void saveallplayers()
{
  struct descriptor_data *pt, *npt;

  for (pt = descriptor_list; pt; pt = npt) {
    npt = pt->next;
    if(pt->connected == CON_PLYNG) {
      if(pt->character)
        stash_char(pt->character,0);
    } else {
      if(pt->character){
        if(pt->character->specials.timer > 20){
          log("Tried auto-closing a non-playing descriptor.");
          close_socket(pt);
        }
      }
    }
  }
}

fd_set input_set, output_set, exc_set;

int game_loop(int s)
{
  int tmp_room, old_len;
  struct timeval last_time, now, timespent, timeout, null_time;
  static struct timeval opt_time;
  char comm[MAX_INPUT_LENGTH];
  struct descriptor_data *t, *point, *next_point;
  int pulse = 0, testmask, mask, xoclock = 0, flag;
  char prmpt[64];

  null_time.tv_sec = 0;
  null_time.tv_usec = 0;
  opt_time.tv_usec = OPT_USEC;  /* Init time values */
  opt_time.tv_sec = 0;
  gettimeofday(&last_time, (struct timezone *) 0);
  maxdesc = s;
  avail_descs = MAXFDALLOWED;
/*
  mask = sigmask(SIGUSR1) | sigmask(SIGUSR2) | sigmask(SIGINT)  |
         sigmask(SIGBUS ) | sigmask(SIGSEGV) | sigmask(SIGTRAP) |
         sigmask(SIGPIPE) | sigmask(SIGALRM) | sigmask(SIGTERM) |
         sigmask(SIGURG ) | sigmask(SIGXCPU) | sigmask(SIGHUP);

  testmask = sigmask(SIGBUS) | sigmask(SIGSEGV);
*/
  /* Main loop */
  while(!shutdowngame) {
    setjmp(env);
    if(shutdowngame) continue;
    if(shutdownwarn){
      if(shutdownwarn==1)
         shutdowngame=1;
      else
         --shutdownwarn;
    }
    /* Check what's happening out there */
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exc_set);
    FD_SET(s, &input_set);
    for (point = descriptor_list; point; point = point->next) {
      FD_SET(point->descriptor, &input_set);
      FD_SET(point->descriptor, &exc_set);
      FD_SET(point->descriptor, &output_set);
    }
    /* check out the time */
    gettimeofday(&now, (struct timezone *) 0);
    timespent = timediff(&now, &last_time);
    timeout = timediff(&opt_time, &timespent);
    last_time.tv_sec = now.tv_sec + timeout.tv_sec;
    last_time.tv_usec = now.tv_usec + timeout.tv_usec;
    if (last_time.tv_usec >= 1000000) {
      last_time.tv_usec -= 1000000;
      last_time.tv_sec++;
    }
    extern int sigsetmask (int __mask) __THROW __attribute_deprecated__;
    if(timeout.tv_usec)
    if(select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout) < 0) {
      perror("Select sleep");
      exit(1);
    }
    if(select(maxdesc+1, &input_set, &output_set, &exc_set, &null_time) < 0) {
      perror("Select poll");
      exit(0);
    }
    extern int sigsetmask (int __mask) __THROW __attribute_deprecated__;
    if(!nonewconnect)
     if(FD_ISSET(s, &input_set))
      new_descriptor(s);
    for(point = descriptor_list; point; point = next_point) {
      next_point = point->next;   
      if(FD_ISSET(point->descriptor, &exc_set)) {
        freaky(point);
        FD_CLR(point->descriptor, &input_set);
        FD_CLR(point->descriptor, &output_set);
      }
    }
    for (point = descriptor_list; point; point = next_point) {
      next_point = point->next;
      if (FD_ISSET(point->descriptor, &input_set))
        if (process_input(point) < 0) {
           FD_CLR(point->descriptor, &input_set);
           FD_CLR(point->descriptor, &output_set);
           if (point->connected == CON_PLYNG) {
             stash_char(point->character,0);
           }
           close_socket(point);
        }
    }
    /* process_commands; */
    if(sigct < 10)
    for (point = descriptor_list; point; point = next_to_process) {
      next_to_process = point->next;
      if ((--(point->wait) <= 0) && get_from_q(&point->input, comm)) {
        if(point->character && (point->connected == CON_PLYNG) &&
          (point->character->specials.was_in_room !=  NOWHERE)) {
          if (point->character->in_room != NOWHERE)
            char_from_room(point->character);
          char_to_room(point->character, 
            point->character->specials.was_in_room);
          point->character->specials.was_in_room = NOWHERE;
          act("$n has returned.",  TRUE, point->character, 0, 0, TO_ROOM);
        }
        point->wait = 1;
        if (point->character)
          point->character->specials.timer = 0;
        point->prompt_mode = 1;
        if (point->str)
          string_add(point, comm);
        else if (!point->connected) 
          if (point->showstr_point)
            show_string(point, comm);
          else {
            point->wait += command_interpreter(point->character, comm);
            ++point->ncmds;
          }
        else
          nanny(point, comm); 
      }
    }
    for (point = descriptor_list; point; point = next_point) {
      next_point = point->next;   
      if (FD_ISSET(point->descriptor, &exc_set)) {
        freaky(point);
        FD_CLR(point->descriptor, &input_set);
        FD_CLR(point->descriptor, &output_set);
      }
    }
    for (point = descriptor_list; point; point = next_point) {
      next_point = point->next;
      xo=point;
      if (FD_ISSET(point->descriptor, &output_set) && point->output.head)
        if((flag=process_output(point)) < 0){
          FD_CLR(point->descriptor, &input_set);
          FD_CLR(point->descriptor, &output_set);
          if (point->connected == CON_PLYNG) {
             stash_char(point->character,0);
          }
          close_socket(point);
        } else
          point->prompt_mode = flag;
     }
    /* give the people some prompts */
    ++xoclock;
    if (xoclock == MAXOCLOCK) {
      xoclock=0;
    }
    for (point = descriptor_list; point; point = point->next){
      if((!point->connected) && (!point->original) &&
         (GET_LEVEL(point->character)<IMO) && (point->descriptor==xoclock)){
        save_char(point->character,NOWHERE);
        stash_char(point->character,0);
      }
      if (point->prompt_mode) {
	out_char = point->character; /* SLUG_CHANGE 11-17-96 */
        if (point->str)
          write_to_descriptor(point->descriptor, "] ");
        else if (!point->connected)
          if (point->showstr_point)
            write_to_descriptor(point->descriptor,
              "--<< Press return >>--");
          else {
            if(GET_LEVEL(point->character) < IMO){
              if(point->character->specials.fighting){
                sprintf(prmpt, "\[%d,%d,%d](%d) ",
                  GET_HIT(point->character),
                  GET_MANA(point->character),
                  GET_MOVE(point->character),
                  GET_HIT(point->character->specials.fighting));
              } else {
                sprintf(prmpt, "\[%d,%d,%d] ",
                  GET_HIT(point->character),GET_MANA(point->character),
                  GET_MOVE(point->character));
              }
            } else {
               sprintf(prmpt, "\[%d] ",
                  world[point->character->in_room].number);
            }
            write_to_descriptor(point->descriptor, prmpt);
          }
        point->prompt_mode = 0;
      }
    }
    pulse++;
    if (!(pulse % PULSE_ZONE))
      zone_update();
    if (!(pulse % PULSE_MOBILE))
      mobile_activity();
    if (!(pulse % PULSE_VIOLENCE))
      perform_violence();
    if (!(pulse % SECS_PER_MUD_HOUR)){
      weather_and_time(1);
      affect_update();
      point_update();
    }
    if (pulse >= 2400) {
      pulse = 0;
      /* zapper(); */
    }
    tics++;
  }
}
/*

zapper shuts down the game on Mon-Fri at 7 am EST

*/
zapper()
{
   int dow,tod,t;

   t=time(0)-18000;
   dow=((t/86400)+5)%7;
   tod=(t%86400)/60;
   if(dow >= 2){
      if(tod > 900) return;
      if(tod > 390) send_to_all("The game will shut down soon.\n\r");
      if(tod >= 420){
         shutdowngame=1;
         if(getppid() > 1)
            kill(getppid(),9);
      }
   }
}
/* ******************************************************************
*  general utility stuff (for local use)                   *
****************************************************************** */

int get_from_q(struct txt_q *queue, char *dest)
{
  struct txt_block *tmp;

   /* Q empty? */
  if (!queue->head)
    return(0);

  tmp = queue->head;
  strcpy(dest, queue->head->text);
  queue->head = queue->head->next;

  free(tmp->text);
  free(tmp);

  return(1);
}

void write_to_q(char *txt, struct txt_q *queue)
{
  struct txt_block *new;

  CREATE(new, struct txt_block, 1);
  CREATE(new->text, char, strlen(txt) + 1);
  strcpy(new->text, txt);
  if (!queue->head) {
    new->next = NULL;
    queue->head = queue->tail = new;
  } else {
    queue->tail->next = new;
    queue->tail = new;
    new->next = NULL;
  }
}

struct timeval timediff(struct timeval *a, struct timeval *b)
{
  struct timeval rslt, tmp;

  tmp = *a;

  if ((rslt.tv_usec = tmp.tv_usec - b->tv_usec) < 0)
  {
    rslt.tv_usec += 1000000;
    --(tmp.tv_sec);
  }
  if ((rslt.tv_sec = tmp.tv_sec - b->tv_sec) < 0)
  {
    rslt.tv_usec = 0;
    rslt.tv_sec = 0;
  }
  return(rslt);
}

/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data *d)
{
  char dummy[MAX_STRING_LENGTH];

  while (get_from_q(&d->output, dummy));
  while (get_from_q(&d->input, dummy));
}

/* ******************************************************************
*  socket handling               *
****************************************************************** */

int init_socket(int port)
{
  int s;
  char *opt;
  char hostname[MAX_HOSTNAME+1];
  struct sockaddr_in sa;
  struct hostent *hp;
  struct linger ld;

  bzero((char *)&sa, sizeof(struct sockaddr_in));
  gethostname(hostname, MAX_HOSTNAME);
  log(hostname);
  hp = gethostbyname(hostname);
  if (hp == NULL) {
    perror("gethostbyname");
    exit(1);
  }
  sa.sin_family = hp->h_addrtype;
  sa.sin_port  = htons(port);
  s = socket(AF_INET,SOCK_STREAM,0);
  if(s < 0) {
    perror("Init-socket");
    exit(1);
  }
  if(setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char *) &opt, sizeof(opt)) < 0){
    perror ("setsockopt REUSEADDR");
    exit (1);
  }
/*
  ld.l_onoff = 0; ld.l_linger = 0;
  if (setsockopt(s, SOL_SOCKET, SO_LINGER, &ld, sizeof(ld)) < 0) {
    perror("setsockopt LINGER");
    exit(1);
  }
*/
  if (bind(s,(struct sockaddr *) &sa, sizeof(sa)) < 0) {
    perror("bind");
    close(s);
    return(-1);
  }
  listen(s, 3);
  return(s);
}

int new_connection(int s)
{
  struct sockaddr_in isa;
  /* struct sockaddr peer; */
  int i;
  int t;
  char buf[100];

  i = sizeof(isa);
  getsockname(s, (struct sockaddr *) &isa, &i);
  if ((t = accept(s, (struct sockaddr *) &isa, &i)) < 0) {
    return(-1);
  }
  nonblock(t);
  return(t);
}

int new_descriptor(int s)
{
  int desc;
  struct descriptor_data *newd;
  int i,size;
  struct sockaddr_in sock;
  struct hostent *from;
  char buf[10];

  if ((desc = new_connection(s)) < 0)
    return (-1);
  bytetotal[desc]=0;
  timeoflastconnect=time(0);
  size = sizeof(sock);
  if(getpeername(desc,(struct sockaddr *)&sock,&size) < 0){
    *newd->host = 0;
    close(desc);
    return(-1);
  } else {
    if(unfriendly_domain((char *)inet_ntoa(sock.sin_addr),baddomain,baddoms)){
      close(desc);
      return(-1);
    }
  }
  CREATE(newd, struct descriptor_data, 1);
  strncpy(newd->host,(char *)inet_ntoa(sock.sin_addr),16);
  if((maxdesc+1) >= avail_descs){
    close(desc);
    flush_queues(newd);
    free((char *)newd);
    return(0);
  } else
    if (desc > maxdesc)
      maxdesc = desc;
  newd->ncmds = 0; newd->contime = time(0);
  newd->descriptor = desc;
  newd->connected = 1;
  newd->wait = 1;
  newd->prompt_mode = 0;
  *newd->buf = '\0';
  newd->str = 0;
  newd->showstr_head = 0;
  newd->showstr_point = 0;
  *newd->last_input= '\0';
  newd->output.head = NULL;
  newd->input.head = NULL;
  newd->next = descriptor_list;
  newd->character = 0;
  newd->original = 0;
  newd->snoop.snooping = 0;
  newd->snoop.snoop_by = 0;

  /* prepend to list */

  descriptor_list = newd;

  SEND_TO_Q(GREETINGS, newd);
  if(nonewplayers){
    SEND_TO_Q("WARNING:\n\r",newd);
    SEND_TO_Q("No NEW characters are being accepted right now.\n\r\n\r",newd);
  }
  SEND_TO_Q("By what name do you wish to be known? ", newd);

  return(0);
}

unfriendly_domain(h,list,listlen)
char *h;
char list[BADDOMS][BADSTRLEN];
int listlen;
{
   int i;

   for(i=0;i<listlen;++i){
      if(strncmp(h,list[i],strlen(list[i]))==0){
         return(1);
      }
   }
   return(0);
}

int process_output(struct descriptor_data *t)
{
  char i[MAX_STRING_LENGTH + 1];
  int iter=0;

out_char = t->character;	/* SLUG_CHANGE 11-17-96 */

  if(!t->prompt_mode && !t->connected)
    if (write_to_descriptor(t->descriptor, "\n\r") < 0)
      return(-1);
  while(get_from_q(&t->output, i)){
    if(t->snoop.snoop_by) {
      write_to_q("> ",&t->snoop.snoop_by->desc->output);
      write_to_q(i,&t->snoop.snoop_by->desc->output);
    }
    if (write_to_descriptor(t->descriptor, i))
      return(-1);
  }
  if(!t->connected && !(t->character && !IS_NPC(t->character) && 
                  IS_SET(t->character->specials.act, PLR_COMPACT)))
    if(write_to_descriptor(t->descriptor, "\n\r") < 0)
      return(-1);
  return(1);
}

#include <sys/ioctl.h>

int write_to_descriptor(int desc, char *txt)
{
  int sofar, thisround, total;
  int w,x,y,z;
  char buf[256];
  static int max=0;
  
  total = strlen(txt);
/*
  if(total > max){
    sprintf(buf,"Output Max = %d.",max=total);
    log(buf);
  }
*/
  sofar = 0;
  do {
/*
    ioctl(desc,SIOCGPGRP,&w);
    ioctl(desc,SIOCATMARK,&x);
    ioctl(desc,SIOCGHIWAT,&y);
    ioctl(desc,SIOCGLOWAT,&z);
    sprintf(buf,"IOCTL: %d %d %x %x",w,x,y,z);
    log(buf);
*/
    thisround = write(desc, txt + sofar, total - sofar);
    if (thisround < 0) {
      perror("Write to socket");
      return(-1);
    } else if(thisround==0) {
    } else {
      sofar += thisround;
    }
  } while (sofar < total);
  grand_total += sofar;
  bytetotal[desc] += sofar;
  
  if(out_char)
  out_char->specials.outbytes += sofar; /* SLUG_CHANGE 11-17-96 */
  return(0);
}
int process_input(struct descriptor_data *t)
{
  int sofar, thisround, begin, squelch, i, k, flag;
  char tmp[MAX_STRING_LENGTH],buffer[MAX_STRING_LENGTH];

  sofar = 0;
  flag = 0;
  begin = strlen(t->buf);
  /* Read in some stuff */
  do {
    if ((thisround = read(t->descriptor, t->buf + begin + sofar, 
      MAX_INPUT_LENGTH - (begin + sofar) - 1)) > 0)
      sofar += thisround;    
    else
      if (thisround < 0)
        if(errno != EWOULDBLOCK) {
          perror("Read1 - ERROR");
          return(-1);
        } else
          break;
      else {
        return(-1);
      }
  }
  while (!ISNEWL(*(t->buf + begin + sofar - 1)));  
  *(t->buf + begin + sofar) = 0;

  /* if no newline is contained in input, return without proc'ing */
  for (i = begin; !ISNEWL(*(t->buf + i)); i++)
    if (!*(t->buf + i))
      return(0);

if(t->character) /* SLUG_CHANGE 11-17-96 */ 
t->character->specials.inbytes += sofar;   

  /* input contains 1 or more newlines; process the stuff */
  for (i = 0, k = 0; *(t->buf + i);) {
    if (!ISNEWL(*(t->buf + i)) && !(flag = (k >= (MAX_INPUT_LENGTH - 2))))
      if(*(t->buf + i) == '\b')   /* backspace */
        if (k)  /* more than one char ? */
        {
          if (*(tmp + --k) == '$')
            k--;
          i++;
          }
        else
           i++;  /* no or just one char.. Skip backsp */
      else
        if (isascii(*(t->buf + i)) && isprint(*(t->buf + i)))
        {
          if ((*(tmp + k) = *(t->buf + i)) == '$')
            *(tmp + ++k) = '$';
          k++;
          i++;
          }
        else
           i++;
    else {
      *(tmp + k) = 0;
      if(*tmp == '!')
        strcpy(tmp,t->last_input);
      else
        strcpy(t->last_input,tmp);
      write_to_q(tmp, &t->input);
      if(t->snoop.snoop_by) {
          write_to_q("% ",&t->snoop.snoop_by->desc->output);
          write_to_q(tmp,&t->snoop.snoop_by->desc->output);
          write_to_q("\n\r",&t->snoop.snoop_by->desc->output);
        }
      if (flag) {
	out_char = t->character; /* SLUG_CHANGE 11-17-96 */
        sprintf(buffer, "Line too long. Truncated to:\n\r%s\n\r", tmp);
        if (write_to_descriptor(t->descriptor, buffer) < 0)
          return(-1);
        /* skip the rest of the line */
        for (; !ISNEWL(*(t->buf + i)); i++);
      }
      /* find end of entry */
      for (; ISNEWL(*(t->buf + i)); i++);
      /* squelch the entry from the buffer */
      for (squelch = 0;; squelch++)
        if ((*(t->buf + squelch) = 
          *(t->buf + i + squelch)) == '\0')
            break;
      k = 0;
      i = 0;
    }
  }
  return(1);
}

void close_sockets(int s)
{
  log("Closing all sockets.");
  shutdown(s,2);
  while(descriptor_list){
    close_socket(descriptor_list);
  }
  close(s);
}

void close_socket(struct descriptor_data *d)
{
  struct affected_type *af;
  struct descriptor_data *tmp;
  char buf[100];

  close(d->descriptor);
  flush_queues(d);
  if (d->descriptor == maxdesc)
    --maxdesc;
  /* Forget snooping */
  if (d->snoop.snooping)
    d->snoop.snooping->desc->snoop.snoop_by = 0;
  if (d->snoop.snoop_by) {
      send_to_char("Your victim is no longer among us.\n\r",d->snoop.snoop_by);
      d->snoop.snoop_by->desc->snoop.snooping = 0;
    }
  if (d->character){
    if (d->connected == CON_PLYNG) {
      save_char(d->character, NOWHERE);
      stash_char(d->character,0);
      act("$n has lost $s link.", TRUE, d->character, 0, 0, TO_ROOM);
      sprintf(buf, "Closing link to: %s.", GET_NAME(d->character));
      log(buf);
      d->character->desc = 0;
    } else {
/*
      sprintf(buf, "Losing player: %s.", GET_NAME(d->character));
      log(buf);
*/
      free_char(d->character);
    }
  } else
/*
    log("Losing descriptor without char.");
*/
  if (next_to_process == d)    /* to avoid crashing the process loop */
    next_to_process = next_to_process->next;   
  if (d == descriptor_list) /* this is the head of the list */
    descriptor_list = descriptor_list->next;
  else  /* This is somewhere inside the list */ {
    /* Locate the previous element */
    for (tmp = descriptor_list; (tmp->next != d) && tmp; 
      tmp = tmp->next);
    tmp->next = d->next;
  }
  if (d->showstr_head)
    free(d->showstr_head);
  free(d);
}

void nonblock(int s)
{
#ifndef NO_FNDELAY
  if (fcntl(s, F_SETFL, FNDELAY) == -1)
  {
    perror("Noblock");
    exit(1);
  }
#endif
}



/* ****************************************************************
*  Public routines for system-to-player-communication        *
**************************************************************** */



void send_to_char(char *messg, struct char_data *ch)
{
  if(ch->desc && messg)
    write_to_q(messg, &ch->desc->output);
}

void send_to_all(char *messg)
{
  struct descriptor_data *i;

  if (messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected)
        write_to_q(messg, &i->output);
}

void send_to_outdoor(char *messg)
{
  struct descriptor_data *i;

  if (messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected)
        if (OUTSIDE(i->character))
          write_to_q(messg, &i->output);
}

void send_to_except(char *messg, struct char_data *ch)
{
  struct descriptor_data *i;

  if (messg)
    for (i = descriptor_list; i; i = i->next)
      if (ch->desc != i && !i->connected)
        write_to_q(messg, &i->output);
}

void send_to_room(char *messg, int room)
{
  struct char_data *i;

  if (messg)
    for (i = world[room].people; i; i = i->next_in_room)
      if (i->desc)
        write_to_q(messg, &i->desc->output);
}




void send_to_room_except(char *messg, int room, struct char_data *ch)
{
  struct char_data *i;

  if (messg)
    for (i = world[room].people; i; i = i->next_in_room)
      if (i != ch && i->desc)
        write_to_q(messg, &i->desc->output);
}

void send_to_room_except_two
  (char *messg, int room, struct char_data *ch1, struct char_data *ch2)
{
      struct char_data *i;

      if (messg)
        for (i = world[room].people; i; i = i->next_in_room)
          if (i != ch1 && i != ch2 && i->desc)
            write_to_q(messg, &i->desc->output);
}

/* higher-level communication */

void act(char *str, int hide_invisible, struct char_data *ch,
  struct obj_data *obj, void *vict_obj, int type)
{
  char *strp, *point, *i;
  struct char_data *to;
  char buf[MAX_STRING_LENGTH];

  if (!str)
    return;
  if (!*str)
    return;

  if (type == TO_VICT)
    to = (struct char_data *) vict_obj;
  else if (type == TO_CHAR)
    to = ch;
  else
    to = world[ch->in_room].people;

  for (; to; to = to->next_in_room) {
    if (to->desc && ((to != ch) || (type == TO_CHAR)) &&  
      (CAN_SEE(to, ch) || !hide_invisible) && AWAKE(to) &&
      !((type == TO_NOTVICT) && (to == (struct char_data *) vict_obj)))
    {
      for (strp = str, point = buf;;)
        if (*strp == '$') {
          switch (*(++strp)) {
            case 'n': i = PERS(ch, to); break;
            case 'N': i = PERS((struct char_data *) vict_obj, to); break;
            case 'm': i = HMHR(ch); break;
            case 'M': i = HMHR((struct char_data *) vict_obj); break;
            case 's': i = HSHR(ch); break;
            case 'S': i = HSHR((struct char_data *) vict_obj); break;
            case 'e': i = HSSH(ch); break;
            case 'E': i = HSSH((struct char_data *) vict_obj); break;
            case 'o': i = OBJN(obj, to); break;
            case 'O': i = OBJN((struct obj_data *) vict_obj, to); break;
            case 'p': i = OBJS(obj, to); break;
            case 'P': i = OBJS((struct obj_data *) vict_obj, to); break;
            case 'a': i = SANA(obj); break;
            case 'A': i = SANA((struct obj_data *) vict_obj); break;
            case 'T': i = (char *) vict_obj; break;
            case 'F': i = fname((char *) vict_obj); break;
            case '$': i = "$"; break;
            default:
              log("Illegal $-code to act():");
              log(str);
              break;
          }
          while (*point = *(i++))
            ++point;
          ++strp;
        }
        else if (!(*(point++) = *(strp++)))
          break;
      *(--point) = '\n';
      *(++point) = '\r';
      *(++point) = '\0';
      if((type & 1) || !IS_SET(to->specials.act,PLR_VERYBRIEF))
        write_to_q(CAP(buf), &to->desc->output);
    }
    if ((type == TO_VICT) || (type == TO_CHAR))
      return;
  }
}
void freaky(struct descriptor_data *d)
{
  char buf[128];
  struct char_data *vict;
  int i;

  vict=d->original ? d->original : d->character;
  sprintf(buf,"Freaky: %d %d %s",
    d->connected,
    d->descriptor,
    vict->player.name);
  log(buf);
  stash_char(vict,0);
  for(i=0;i<MAX_WEAR;i++)
    if(vict->equipment[i]){
      extract_obj(unequip_char(vict,i));
      vict->equipment[i]=0;
    }
  wipe_obj(vict->carrying);
  vict->carrying=0;
  if(vict->desc)
    close_socket(vict->desc);
  extract_char(vict);
}
void signal_setup(void)
{
  struct itimerval itime;
  struct timeval interval;

  signal(SIGUSR2, shutdown_request);
  signal(SIGALRM, logsig);
  /*siginterrupt(SIGHUP , 1); */ signal(SIGHUP , hupsig);
  /*siginterrupt(SIGINT , 1); */ signal(SIGINT , hupsig);
  /*siginterrupt(SIGTERM, 1); */ signal(SIGTERM, hupsig);
  /*siginterrupt(SIGSEGV, 1); */ signal(SIGSEGV, hupsig);
  /*siginterrupt(SIGBUS , 1); */ signal(SIGBUS , hupsig);
  /*siginterrupt(SIGPIPE, 1); */ signal(SIGPIPE, hupsig);
  /*siginterrupt(SIGTRAP, 1); */ signal(SIGTRAP, hupsig);

  return;

  interval.tv_sec = 900;    /* 15 minutes */
  interval.tv_usec = 0;
  itime.it_interval = interval;
  itime.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &itime, 0);
  signal(SIGVTALRM, checkpointing);
}
void checkpointing(int sig)
{
  extern int tics;
  
  if (!tics) {
    log("CHECKPOINT shutdown: tics not updated");
    abort();
  }
  else
    tics = 0;
  log("checkpointing");
  exit(0);
}
void shutdown_request(int sig)
{
  send_to_all("Shut down signal has been received.\n\r");
  log("Received USR2 - shutdown request");
  shutdowngame = 1;
}
/* kick out players etc */
void hupsig(int sig)
{
  char ss[MAX_STRING_LENGTH];
  struct descriptor_data *tmp, *point;

  signal(sig,hupsig);
  sigct++;
  sprintf(ss,"SIG(%d,%d): %s %s",
    sigct,sig,xo->character->player.name,xo->host);
  --GET_INT(xo->character);
  log(ss);
  if(sigct > 3)
    exit(0);
  if(sigct > 1)
    shutdowngame = 100;
  longjmp(env,sigct);
}
void logsig(int sig)
{
  log("Signal received. Ignoring.");
}


