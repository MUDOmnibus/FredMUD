
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "structs.h"
#include "utils.h"

struct alias_data *parse_alias(char *s);

void read_alias(struct char_data *ch)
{
  FILE *fd;
  char buf[MAX_STRING_LENGTH];
  struct alias_data *p;

  sprintf(buf,"alias/%s",GET_NAME(ch));
  fd=fopen(buf,"r");
  if(!fd)
    return;
  while(fgets(buf,MAX_STRING_LENGTH,fd)){
    p=parse_alias(buf);
    if(!p) break;
    p->next = ch->alp;
    ch->alp = p;
  }
  fclose(fd);
}
void save_alias(struct char_data *ch)
{
  FILE *fd;
  char buf[MAX_STRING_LENGTH];
  struct alias_data *p;

  sprintf(buf,"alias/%s",GET_NAME(ch));
  fd=fopen(buf,"w");
  if(!fd)
    return;
  for(p=ch->alp;p;p=p->next)
    fprintf(fd,"%s=%s\n",p->in,p->out);
  fclose(fd);
}
struct alias_data *parse_alias(char *s)
{
  char *t,*u;
  struct alias_data *p;
  char buf[MAX_STRING_LENGTH];

  t = (char *)index(s,'=');
  if(!t)
    return 0;
  *t = 0;
  t++;
  p=(struct alias_data *)malloc(sizeof(struct alias_data));
  p->in = strdup(s);
  u = (char *)rindex(t,'\n');
  if(u)
    *u = 0;
  p->out = strdup(t);
  p->next = 0;
  return p;
}
void do_unalias(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  struct alias_data *p,*q;

  one_argument(argument,buf);
  if(!*buf){
    return;
  }
  q=0;
  for(p=ch->alp;p;p=p->next)
    if(!strcmp(buf,p->in)){
      q=p;
      break;
    }
  if(!q)
    return;
  if(q->in)
    free(q->in);
  if(q->out)
    free(q->out);
  if(q==ch->alp){
    ch->alp=q->next;
    free(q);
  } else {
    for(p=ch->alp;p;p=p->next){
      if(p->next==q){
        p->next=q->next;
        free(q);
        break;
      }
    }
  }
  save_alias(ch);
}
void do_alias(struct char_data *ch, char *argument, int cmd)
{
  char buf1[MAX_STRING_LENGTH],buf2[MAX_STRING_LENGTH];
  struct alias_data *p;

  half_chop(argument,buf1,buf2);
  if(!*buf1){
    for(p=ch->alp;p;p=p->next){
      sprintf(buf1,"%s->%s\n\r",p->in,p->out);
      send_to_char(buf1,ch);
    }
    return;
  }
  if(!*buf2){
    for(p=ch->alp;p;p=p->next)
      if(strcmp(p->in,buf1)==0){
        sprintf(buf2,"%s is aliased to %s\n\r",buf1,p->out);
        send_to_char(buf2,ch);
        return;
      }
    sprintf(buf2,"%s isn't aliased to anything\n\r",buf1);
    send_to_char(buf2,ch);
    return;
  }
  for(p=ch->alp;p;p=p->next)
    if(strcmp(p->in,buf1)==0){
      sprintf(buf1,"New alias for %s is %s replacing %s\n\r",
        p->in,buf2,p->out);
      send_to_char(buf1,ch);
      free(p->out);
      p->out=strdup(buf2);
      return;
    }
  p=(struct alias_data *)malloc(sizeof(struct alias_data));
  p->in  = strdup(buf1);
  p->out = strdup(buf2);
  p->next = ch->alp;
  ch->alp = p;
  save_alias(ch);
}
int expand_alias(struct char_data *ch, char *inbuf, char *outbuf)
{
  struct alias_data *p;
  int n;

  while(*inbuf == ' ')
    inbuf++;
  for(p=ch->alp;p;p=p->next){
    n = strlen(p->in);
    if(strncmp(p->in,inbuf,n)==0){
      if(!inbuf[n] || (inbuf[n]==' ')){
        strcpy(outbuf,p->out);
        strcat(outbuf,inbuf+n);
        return 1;
      }
    }
  }
  return 0;
}

