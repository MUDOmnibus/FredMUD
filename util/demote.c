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

void del(char *filename, int name)
{
  char confirm[80];
  FILE *fl;
  struct char_file_u player;
  int pos, num, i;
  long end;

  if (!(fl = fopen(filename, "r+b")))
  {
    perror("list");
    exit();
  }

  puts("Searching for player:");

  for (num = 1, pos = 0;; pos++, num++)
  {
    fread(&player, sizeof(player), 1, fl);
    if (feof(fl)) {
      fprintf(stderr, "delplay: could not locate %d.\n", name);
      exit();
    }
    if (num == name) {
      player.level=0;
      memset(player,0,sizeof(player));
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
  if (argc != 2)
    puts("Usage: delplay <Player Number>");
  else {
    if (atoi(argv[1]) < 1)
      puts("Illegal player number, must be >= 1");
    else
      del("../lib/players",atoi(argv[1]));
  }
}
