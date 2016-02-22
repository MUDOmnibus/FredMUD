#include <stdio.h>
#include <ctype.h>
#include "../structs.h"

#define TOLOWER(c)  (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))

int str_cmp(char *str1, char *str2)
{
  for (; *str1 || *str2; str1++, str2++)
    if (TOLOWER(*str1) != TOLOWER(*str2))
      return(1);

  return(0);
}

void del(char *filename, int name, char *newpass)
{
  char confirm[80];
  FILE *fl;
  struct char_file_u player;
  int pos, num;
  long end;

  if (!(fl = fopen(filename, "r+b"))) {
    perror("list");
    exit();
  }

  for (num = 1, pos = 0;; pos++, num++) {
    fread(&player, sizeof(player), 1, fl);
    if (feof(fl)) {
      fprintf(stderr, "delplay: could not locate %d.\n", name);
      exit();
    }
    if (num == name) {
      printf("%s\n",player.name);
      strcpy(player.name,newpass);
      fseek(fl, -sizeof(player), 1);
      fwrite(&player, sizeof(player), 1, fl);
      break;
    }
  }
  fseek(fl, 0, 2);
  fclose(fl);
}
main(int argc, char **argv)
{
      del("../lib/players",atoi(argv[1]),argv[2]);
}
