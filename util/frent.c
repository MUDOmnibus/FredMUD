#include <stdio.h>

#define K 1024

main(int argc, char *argv[])
{
  char s[K];
  FILE *fd;
  int n;

  srandom(getpid());
  fd=fopen(argv[1],"r");
  if(fd==NULL){
    perror("fopen");
    exit(0);
  }
  while(fgets(s,K-1,fd)){
    n=strlen(s);
    s[--n]=0;
    sprintf(s+n," %d %d\n",0,random());
    printf("%s",s);
  }
}
