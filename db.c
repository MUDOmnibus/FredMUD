/**************************************************************************
*  file: db.c , Database module.                          Part of DIKUMUD *
*  Usage: Loading/Saving chars, booting world, resetting etc.             *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "limits.h"

/**************************************************************************
*  declarations of most of the 'global' variables                         *
************************************************************************ */

struct momtalktype **momtalk;
int momtct=0, momsreadflag=0;

int dig_room, dig_vnum;
struct room_data *world;              /* dyn alloc'ed array of rooms     */
int top_of_world = 0;                 /* ref to the top element of world */
struct obj_data  *object_list = 0;    /* the global linked list of obj's */
struct char_data *character_list = 0; /* global l-list of chars          */
struct zone_data *zone_table;         /* table of reset data             */
int top_of_zone_table = 0;
struct message_list fight_messages[MAX_MESSAGES]; /* fighting messages   */
struct player_index_element *player_table = 0; /* index to player file   */
int top_of_p_table = 0;               /* ref to top of table             */
int top_of_p_file = 0;

char news[MAX_STRING_LENGTH];         /* the news                        */
char motd[MAX_STRING_LENGTH];         /* the messages of today           */
char help[MAX_STRING_LENGTH];         /* the main help page              */
char info[MAX_STRING_LENGTH];         /* the info text                   */
char spellinfo[MAX_STRING_LENGTH];    /* the info text                   */

FILE *mob_f,                          /* file containing mob prototypes  */
     *obj_f,                          /* obj prototypes                  */
     *help_fl;                        /* file for help texts (HELP <kwd>)*/

struct index_data *mob_index;         /* index table for mobile file     */
struct index_data *obj_index;         /* index table for object file     */
struct help_index_element *help_index = 0;

int top_of_mobt = 0;                  /* top of mobile index table       */
int top_of_objt = 0;                  /* top of object index table       */
int top_of_helpt;                     /* top of help index table         */

struct time_info_data time_info;  /* the infomation about the time   */
struct weather_data weather_info;  /* the infomation about the weather */

int weaponboost[]={-3,-2,-1,0,0,0,0,1,2,3,4};

#ifdef NEEDS_STRDUP
char *strdup(char *s);
#endif

void boot_zones(void);
void setup_dir(FILE *fl, int room, int dir);
void allocate_room(int new_top);
void boot_world(void);
struct index_data *generate_indices(FILE *fl, int *top);
void build_player_index(void);
void char_to_store(struct char_data *ch, struct char_file_u *st);
void store_to_char(struct char_file_u *st, struct char_data *ch);
int is_empty(int zone_nr);
void reset_zone(int zone);
int file_to_string(char *name, char *buf);
void renum_world(void);
void renum_zone_table(void);
void reset_time(void);
void clear_char(struct char_data *ch);
void quickwear(struct char_data *ch, struct obj_data *obj_object, int keyword);
void quick_equip(struct char_data *ch, struct obj_data *obj, int pos);
void do_loadroom(struct char_data *ch, char *argument, int cmd);

/* external refs */

extern int bobflag;
extern char *bobname;

extern struct descriptor_data *descriptor_list;
void load_messages(void);
void weather_and_time ( int mode );
void assign_command_pointers ( void );
void assign_spell_pointers ( void );
void assign_weapon_spells ( void );
void log(char *str);
int dice(int number, int size);
int number(int from, int to);
void boot_social_messages(void);
struct help_index_element *build_help_index(FILE *fl, int *num);
int where_wear(struct obj_data *obj_object);


/*************************************************************************
*  routines for booting the system                                       *
*********************************************************************** */

void add_random_affects(struct obj_data *obj)
{
  int i,j,tmp;

  for( i = 0 ; i < MAX_OBJ_AFFECT; i++)
    if(!obj->affected[i].location){
      j=obj->obj_flags.cost;
      if(j >= 0){
        switch(i){
          case 0: tmp=number(17,19); break;
          case 1: tmp=number(1,5); break;
          case 2: tmp=number(12,14); break;
        }
        obj->affected[i].modifier = number(1,j);
      } else {
        obj->affected[i].modifier = number(j,0);
        tmp=number(12,14);
      }
      obj->affected[i].location = tmp;
    }
}
void vary_item(struct obj_data *obj)
{
  int i;

  switch(GET_ITEM_TYPE(obj)){
    case ITEM_WORN:
      if(obj->affected[0].location){
        obj->affected[0].modifier =
          number(obj->affected[0].modifier,0); 
      }
      break;
    case ITEM_TOKEN:
      i=obj->obj_flags.value[0];
      obj->obj_flags.value[0]=number(1,i);
      break;
    case ITEM_PILL:
      i=obj->obj_flags.value[1];
      obj->obj_flags.value[1]=number(1,i);
      break;
    case ITEM_ARMOR:
      if(IS_SET(obj->obj_flags.extra_flags,ITEM_CHEAP))
        add_random_affects(obj);
    case ITEM_BOMB:
      i=obj->obj_flags.value[0];
      obj->obj_flags.value[0]=number(MAX(2,i>>1),i);
      break;
    case ITEM_WEAPON:
      obj->obj_flags.value[1]+=weaponboost[number(0,10)];
      obj->obj_flags.value[2]+=weaponboost[number(0,10)];
      break;
    case ITEM_POTION:
      i=obj->obj_flags.value[0];
      obj->obj_flags.value[0]=number(i,i+i);
      break;
  }
}

/* body of the booting system */
void boot_db(void)
{
  int i;

  log("Boot db -- BEGIN.");

  log("Resetting the game time:");
  reset_time();
  log("Reading newsfile, help-page, info and motd.");
  file_to_string(NEWS_FILE, news);
  file_to_string(MOTD_FILE, motd);
  file_to_string(HELP_PAGE_FILE, help);
  file_to_string(INFO_FILE, info);
  file_to_string(SPELL_FILE,spellinfo);
  log("Opening mobile, object and help files.");
  if (!(mob_f = fopen(MOB_FILE, "r"))) {
    perror("boot");
    exit(0);
  }
  if (!(obj_f = fopen(OBJ_FILE, "r"))) {
    perror("boot");
    exit(0);
  }
  if (!(help_fl = fopen(HELP_KWRD_FILE, "r")))
    log("   Could not open help file.");
  else 
    help_index = build_help_index(help_fl, &top_of_helpt);
  log("Loading zone table.");
  boot_zones();
  log("Loading rooms.");
  boot_world();
  log("Renumbering rooms.");
  renum_world();
  log("Generating index tables for mobile and object files.");
  mob_index = generate_indices(mob_f, &top_of_mobt);
  obj_index = generate_indices(obj_f, &top_of_objt);
  log("Renumbering zone table.");
  renum_zone_table();
  log("Generating player index.");
  build_player_index();
  log("Loading fight messages.");
  load_messages();
  log("Loading social messages.");
  boot_social_messages();
  log("Assigning function pointers:");
  log(" Mobiles.");
  assign_mobiles();
  log(" Objects.");
  assign_objects();
  log(" Rooms.");
  assign_rooms();
  log(" Commands.");  
  assign_command_pointers();
  log(" Spells.");
  assign_spell_pointers();
  log(" Weapons Spells.");
  assign_weapon_spells();
  for (i = 0; i <= top_of_zone_table; i++)
    reset_zone(i);
  log("Boot db -- DONE.");
}

/* reset the time in the game from file */
void reset_time(void)
{
  char buf[MAX_STRING_LENGTH];
  struct time_info_data mud_time;

  long beginning_of_time = 650336715;
  struct time_info_data mud_time_passed(time_t t2, time_t t1);
  time_info = mud_time_passed(time(0), beginning_of_time);

  switch(time_info.hours){
    case 0 :
    case 1 :
    case 2 :
    case 3 :
    case 4 : 
    {
      weather_info.sunlight = SUN_DARK;
      break;
    }
    case 5 :
    {
      weather_info.sunlight = SUN_RISE;
      break;
    }
    case 6 :
    case 7 :
    case 8 :
    case 9 :
    case 10 :
    case 11 :
    case 12 :
    case 13 :
    case 14 :
    case 15 :
    case 16 :
    case 17 :
    case 18 :
    case 19 :
    case 20 :
    {
      weather_info.sunlight = SUN_LIGHT;
      break;
    }
    case 21 :
    {
      weather_info.sunlight = SUN_SET;
      break;
    }
    case 22 :
    case 23 :
    default :
    {
      weather_info.sunlight = SUN_DARK;
      break;
    }
  }

  sprintf(buf,"   Current Gametime: %dH %dD %dM %dY.",
          time_info.hours, time_info.day,
          time_info.month, time_info.year);
  log(buf);

  weather_info.pressure = 960;
  if ((time_info.month>=7)&&(time_info.month<=12))
    weather_info.pressure += dice(1,50);
  else
    weather_info.pressure += dice(1,80);

  weather_info.change = 0;

  if (weather_info.pressure<=980)
    weather_info.sky = SKY_LIGHTNING;
  else if (weather_info.pressure<=1000)
    weather_info.sky = SKY_RAINING;
  else if (weather_info.pressure<=1020)
    weather_info.sky = SKY_CLOUDY;
  else weather_info.sky = SKY_CLOUDLESS;
}

/* generate index table for the player file */
void build_player_index(void)
{
  int nr = -1, i;
  struct char_file_u dummy;
  FILE *fl;

  if (!(fl = fopen(PLAYER_FILE, "rb+")))
  {
    perror("build player index");
    exit(0);
  }
  for (; !feof(fl);) {
    fread(&dummy, sizeof(struct char_file_u), 1, fl);
    if (!feof(fl))   /* new record */
    {
      /* Create new entry in the list */
      if (nr == -1) {
        CREATE(player_table, 
                 struct player_index_element, 1);
        nr = 0;
      }  else {
        if (!(player_table = (struct player_index_element *)
            realloc(player_table, (++nr + 1) *
            sizeof(struct player_index_element))))
        {
          perror("generate index");
          exit(0);
        }
      }
      player_table[nr].nr = nr;
      CREATE(player_table[nr].name, char,
         strlen(dummy.name) + 1);
      for (i = 0; *(player_table[nr].name + i) = 
         LOWER(*(dummy.name + i)); i++);
    }
  }
  fclose(fl);
  top_of_p_table = nr;
  top_of_p_file = top_of_p_table;
}
  
/* generate index table for object or monster file */
struct index_data *generate_indices(FILE *fl, int *top)
{
  int i = 0;
  struct index_data *index;
  long pos;
  char buf[82];

  rewind(fl);
  for (;;) {
    if (fgets(buf, 81, fl)) {
      if (*buf == '#') {
        /* allocate new cell */
        if (!i)             /* first cell */
          CREATE(index, struct index_data, 1);
        else
          if (!(index = 
            (struct index_data*) realloc(index, 
            (i + 1) * sizeof(struct index_data))))
          {
            perror("load indices");
            exit(0);
           }
        sscanf(buf, "#%d", &index[i].virtual);
        index[i].pos = ftell(fl);
        index[i].number = 0;
        index[i].total = 0;
        index[i].name = 0;
        index[i].func = 0;
        i++;
      } else if (*buf == '$')  /* EOF */
          break;
    }
    else
    {
      perror("generate indices");
      exit(0);
    }
  }
  *top = i - 2;
  return(index);
}

void boot_world(void)
{
  FILE *fl;
  int i,j,nr,fr,room_nr=0,zone=0,dir_nr,virtual_nr,flag,tmp;
  char *temp, chk[50];
  struct extra_descr_data *new_descr;

  world = 0;
  character_list = 0;
  object_list = 0;
  if (!(fl = fopen(WORLD_FILE, "r"))) {
    perror("fopen");
    log("boot_world: could not open world file.");
    exit(0);
  }
  do {
    fscanf(fl, " #%d\n", &virtual_nr);
    temp = fread_string(fl);
    if (flag = (*temp != '$')) {
      allocate_room(room_nr);
      world[room_nr].number = virtual_nr;
      world[room_nr].name = temp;
      world[room_nr].description = fread_string(fl);
      if (top_of_zone_table >= 0) {
        fscanf(fl, " %*d ");
        /* OBS: Assumes ordering of input rooms */
        if(world[room_nr].number <= (zone ? zone_table[zone-1].top : -1)) {
          fprintf(stderr, "Room nr %d is below zone %d.\n",
            room_nr, zone);
          exit(0);
        }
        while (world[room_nr].number > zone_table[zone].top)
          if (++zone > top_of_zone_table) {
            fprintf(stderr, "Room %d is outside of any zone.\n",
              virtual_nr);
            exit(0);
          }
        world[room_nr].zone = zone;
      }
      fscanf(fl, " %d ", &tmp);
      world[room_nr].room_flags = tmp;
      fscanf(fl, " %d ", &tmp);
      world[room_nr].sector_type = tmp;
      world[room_nr].funct = 0;
      world[room_nr].contents = 0;
      world[room_nr].people = 0;
      world[room_nr].light = 0; /* Zero light sources */
      for (tmp = 0; tmp <= 5; tmp++)
        world[room_nr].dir_option[tmp] = 0;
      world[room_nr].ex_description = 0;
      for (;;) {
        fscanf(fl, " %s \n", chk);
        if (*chk == 'D')  /* direction field */
          setup_dir(fl, room_nr, atoi(chk + 1));
        else if (*chk == 'E') {
          CREATE(new_descr, struct extra_descr_data, 1);
          new_descr->keyword = fread_string(fl);
          new_descr->description = fread_string(fl);
          new_descr->next = world[room_nr].ex_description;
          world[room_nr].ex_description = new_descr;
        } else if (*chk == 'S')  /* end of current room */
          break;
      }
      room_nr++;
    }
  }
  while (flag);
  free(temp);  /* cleanup the area containing the terminal $  */
  fclose(fl);
  dig_room=room_nr;
  dig_vnum=DIG_ROOM;
  for(i=0;i<=LAST_ROOM-DIG_ROOM;++i){
    allocate_room(room_nr);
    world[room_nr].number = DIG_ROOM+i;
    world[room_nr].name = 0;
    world[room_nr].description = 0;
    world[room_nr].zone = 100;
    world[room_nr].room_flags = 28;
    world[room_nr].sector_type = 4;
    world[room_nr].funct = 0;
    world[room_nr].contents = 0;
    world[room_nr].people = 0;
    world[room_nr].light = 0; /* Zero light sources */
    for (tmp = 0; tmp <= 5; tmp++)
      world[room_nr].dir_option[tmp] = 0;
    world[room_nr].ex_description = 0;
    room_nr++;
  }
  top_of_world = --room_nr;
}

void allocate_room(int new_top)
{
  struct room_data *new_world;

  if (new_top) { 
    if (!(new_world = (struct room_data *) 
      realloc(world, (new_top + 1) * sizeof(struct room_data)))) {
      perror("alloc_room");
      exit(0);
    } 
  } else
    CREATE(new_world, struct room_data, 1);
  world = new_world;
}

/* read direction data */
void setup_dir(FILE *fl, int room, int dir)
{
  int tmp;

  CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
  world[room].dir_option[dir]->general_description = fread_string(fl);
  world[room].dir_option[dir]->keyword = fread_string(fl);
  fscanf(fl, " %d ", &tmp);
  if (tmp == 1)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR;
  else if (tmp == 2)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
  else
    world[room].dir_option[dir]->exit_info = 0;
  fscanf(fl, " %d ", &tmp);
  world[room].dir_option[dir]->key = tmp;
  fscanf(fl, " %d ", &tmp);
  world[room].dir_option[dir]->to_room = tmp;
}




void renum_world(void)
{
  register int room, door;

  for (room = 0; room <= top_of_world; room++)
    for (door = 0; door <= 5; door++)
      if (world[room].dir_option[door])
     if (world[room].dir_option[door]->to_room != NOWHERE)
       world[room].dir_option[door]->to_room =
         real_room(world[room].dir_option[door]->to_room);
}

void renum_zone_table(void)
{
  int zone, comm;

  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (comm = 0; zone_table[zone].cmd[comm].command != 'S'; comm++)
      switch(zone_table[zone].cmd[comm].command) {
        case 'M':
          zone_table[zone].cmd[comm].arg1 =
            real_mobile(zone_table[zone].cmd[comm].arg1);
          zone_table[zone].cmd[comm].arg3 = 
            real_room(zone_table[zone].cmd[comm].arg3);
        break;
        case 'O':
          zone_table[zone].cmd[comm].arg1 = 
            real_object(zone_table[zone].cmd[comm].arg1);
          if (zone_table[zone].cmd[comm].arg3 != NOWHERE)
            zone_table[zone].cmd[comm].arg3 =
            real_room(zone_table[zone].cmd[comm].arg3);
        break;
        case 'G':
          zone_table[zone].cmd[comm].arg1 =
            real_object(zone_table[zone].cmd[comm].arg1);
        break;
        case 'E':
          zone_table[zone].cmd[comm].arg1 =
            real_object(zone_table[zone].cmd[comm].arg1);
        break;
        case 'P':
          zone_table[zone].cmd[comm].arg1 =
            real_object(zone_table[zone].cmd[comm].arg1);
          zone_table[zone].cmd[comm].arg3 =
            real_object(zone_table[zone].cmd[comm].arg3);
        break;          
        case 'D':
          zone_table[zone].cmd[comm].arg1 =
            real_room(zone_table[zone].cmd[comm].arg1);
        break;
      }
}


/* load the zone table and command tables */
void boot_zones(void)
{
  FILE *fl;
  int zon = 0, cmd_no = 0, ch, expand, tmp;
  char *check, buf[81];

  if (!(fl = fopen(ZONE_FILE, "r"))){
    perror("boot_zones");
    exit(0);
  }

  for(;;){
    fscanf(fl, " #%*d\n");
    check = fread_string(fl);

    if (*check == '$')
      break;    /* end of file */

    /* alloc a new zone */

    if (!zon)
      CREATE(zone_table, struct zone_data, 1);
    else
      if (!(zone_table = (struct zone_data *) realloc(zone_table,
        (zon + 1) * sizeof(struct zone_data))))
        {
          perror("boot_zones realloc");
          exit(0);
        }

    zone_table[zon].name = check;
    fscanf(fl, " %d ", &zone_table[zon].top);
    fscanf(fl, " %d ", &zone_table[zon].lifespan);
    fscanf(fl, " %d ", &zone_table[zon].reset_mode);

    /* read the command table */

    cmd_no = 0;

    for(expand = 1;;){
      if (expand)
        if (!cmd_no)
          CREATE(zone_table[zon].cmd, struct reset_com, 1);
        else
          if (!(zone_table[zon].cmd =
            (struct reset_com *) realloc(zone_table[zon].cmd, 
            (cmd_no + 1) * sizeof(struct reset_com))))
          {
            perror("reset command load");
            exit(0);
          }

      expand = 1;

      fscanf(fl, " "); /* skip blanks */
      fscanf(fl, "%c", 
        &zone_table[zon].cmd[cmd_no].command);
      
      if (zone_table[zon].cmd[cmd_no].command == 'S')
        break;

      if (zone_table[zon].cmd[cmd_no].command == '*')
      {
        expand = 0;
        fgets(buf, 80, fl); /* skip command */
        continue;
      }

      fscanf(fl, " %d %d %d", 
        &tmp,
        &zone_table[zon].cmd[cmd_no].arg1,
        &zone_table[zon].cmd[cmd_no].arg2);

      zone_table[zon].cmd[cmd_no].if_flag = tmp;

      if (zone_table[zon].cmd[cmd_no].command == 'M' ||
        zone_table[zon].cmd[cmd_no].command == 'O' ||
        zone_table[zon].cmd[cmd_no].command == 'E' ||
        zone_table[zon].cmd[cmd_no].command == 'P' ||
        zone_table[zon].cmd[cmd_no].command == 'D')
        fscanf(fl, " %d", &zone_table[zon].cmd[cmd_no].arg3);

      fgets(buf, 80, fl);  /* read comment */

      cmd_no++;
    }
    zon++;
  }
  top_of_zone_table = --zon;
  free(check);
  fclose(fl);
}

/*************************************************************************
*  procedures for resetting, both play-time and boot-time      *
*********************************************************************** */

/* read a mobile from MOB_FILE */
struct char_data *read_mobile(int nr, int type)
{
  int i, skill_nr;
  long tmp, tmp2, tmp3;
  struct char_data *mob;
  char chk[10], buf[KBYTE];
  char letter;
  double factor = 1.0;
  static double max_factor = 1.0;

  i = nr;
  if (type == VIRTUAL)
    if ((nr = real_mobile(nr)) < 0) {
    sprintf(buf, "Mobile (V) %d does not exist in database.", i);
    return(0);
  }

  fseek(mob_f, mob_index[nr].pos, 0);

  CREATE(mob, struct char_data, 1);
  clear_char(mob);

  for (i=0;i<MAX_SKILLS;i++)
    mob->skills[i].learned = 100;

  /***** String data *** */
   
  mob->player.name = fread_string(mob_f);
  mob->player.short_descr = fread_string(mob_f);
  if(!mob_index[nr].name)
    mob_index[nr].name = strdup(mob->player.short_descr);
  mob->player.long_descr = fread_string(mob_f);
  mob->player.description = fread_string(mob_f);
  mob->player.title = 0;
  if(bobflag){
    free(mob->player.name);
    free(mob->player.short_descr);
    free(mob->player.long_descr);
    if(mob->player.description){
      free(mob->player.description);
      mob->player.description=0;
    }
    mob->player.name = strdup(bobname);
    mob->player.short_descr = strdup(bobname);
    sprintf(buf,"%s is here.\n\r",bobname);
    mob->player.long_descr = strdup(buf);
  }
  /* *** Numeric data *** */

  fscanf(mob_f, "%d ", &tmp);
  mob->specials.act = tmp;
  SET_BIT(mob->specials.act, ACT_ISNPC);

  fscanf(mob_f, " %d ", &tmp);
  mob->specials.affected_by = tmp;
  fscanf(mob_f, " %d ", &tmp);
  mob->specials.alignment = tmp;

  fscanf(mob_f, " %c \n", &letter);

  if (letter == 'S') {
    fscanf(mob_f, " %D ", &tmp);
    GET_LEVEL(mob) = tmp;
    mob->specials.magres = tmp/10;
    mob->abilities.str   = 8+tmp+number(tmp,4+tmp);
    mob->abilities.intel = 8+tmp+number(tmp,4+tmp); 
    mob->abilities.wis   = 8+tmp+number(tmp,4+tmp);
    mob->abilities.dex   = 8+tmp+number(tmp,4+tmp);
    mob->abilities.con   = 8+tmp+number(tmp,4+tmp);
  } else {
    fscanf(mob_f, " %D ", &tmp);
    mob->abilities.str = tmp;
    fscanf(mob_f, " %D ", &tmp);
    mob->abilities.intel = tmp; 
    fscanf(mob_f, " %D ", &tmp);
    mob->abilities.wis = tmp;
    fscanf(mob_f, " %D ", &tmp);
    mob->abilities.dex = tmp;
    fscanf(mob_f, " %D ", &tmp);
    mob->abilities.con = tmp;
    fscanf(mob_f, " %D \n", &tmp);
    mob->specials.magres = tmp;
    fscanf(mob_f, " %D ", &tmp);
    GET_LEVEL(mob) = tmp;
  }
  fscanf(mob_f, " %D ", &tmp);
  mob->points.hitroll = tmp;
  fscanf(mob_f, " %D ", &tmp);
  mob->points.armor = tmp+number(1,tmp>>1);
  mob->specials.xxx = 0;
  fscanf(mob_f, " %Dd%D+%D ", &tmp, &tmp2, &tmp3);
  mob->points.max_hit = dice(tmp, tmp2)+tmp3;
  mob->points.hit = mob->points.max_hit;
  fscanf(mob_f, " %Dd%D+%D \n", &tmp, &tmp2, &tmp3);
  mob->points.damroll = factor*tmp3;
  mob->specials.damnodice = tmp;
  mob->specials.damsizedice = tmp2;
  mob->points.mana = 10;
  mob->points.max_mana = 10;
  mob->points.move = 50;
  mob->points.max_move = 50;
  fscanf(mob_f, " %D ", &tmp);
  mob->points.gold = tmp;
  fscanf(mob_f, " %D \n", &tmp);
  GET_EXP(mob) = factor*tmp;
  fscanf(mob_f, " %D ", &tmp);
  mob->specials.position = tmp;
  fscanf(mob_f, " %D ", &tmp);
  mob->specials.default_pos = tmp;
  fscanf(mob_f, " %D \n", &tmp);
  mob->specials.it  = 0;
  GET_SPELL_LEV_BONUS(mob) = 0;
  mob->player.sex = tmp;
  mob->player.time.birth = time(0);
  mob->player.time.played  = 0;
  mob->player.time.logon  = time(0);
  mob->player.weight = factor*100;
  mob->player.height = 100*factor;
  for (i = 0; i < 3; i++)
    GET_COND(mob, i) = (i > 0) ? 100 : 0;
  for (i = 0; i < 5; i++)
    mob->specials.apply_saving_throw[i] =
      MAX(IMO-GET_LEVEL(mob), 1);
  mob->tmpabilities = mob->abilities;
  for (i = 0; i < MAX_WEAR; i++)
    mob->equipment[i] = 0;
  if(number(1,IMO) < GET_LEVEL(mob))
    mob->specials.affected_by |= AFF_HOLD;
  mob->nr = nr;
  mob->desc = 0;

  /* insert in list */

  mob->next = character_list;
  character_list = mob;
  mob_index[nr].number++;
  mob_index[nr].total++;
  return(mob);
}

/* read an object from OBJ_FILE */
struct obj_data *read_object(int nr, int type)
{
  struct obj_data *obj;
  int tmp, i;
  char chk[50], buf[100];
  struct extra_descr_data *new_descr;

  i = nr;
  if (type == VIRTUAL)
    if ((nr = real_object(nr)) < 0)
  {
    sprintf(buf, "Object (V) %d does not exist in database.", i);
    return(0);
  }

  fseek(obj_f, obj_index[nr].pos, 0);
  CREATE(obj, struct obj_data, 1);
  clear_object(obj);

  /* *** string data *** */

  obj->name = fread_string(obj_f);
  obj->short_description = fread_string(obj_f);
  if(!obj_index[nr].name)
    obj_index[nr].name = strdup(obj->short_description);
  obj->description = fread_string(obj_f);

  /* *** numeric data *** */

  fscanf(obj_f, " %d ", &tmp);
  obj->obj_flags.type_flag = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->obj_flags.extra_flags = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->obj_flags.wear_flags = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->obj_flags.value[0] = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->obj_flags.value[1] = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->obj_flags.value[2] = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->obj_flags.value[3] = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->obj_flags.weight = tmp;
  fscanf(obj_f, " %d ", &tmp);
  obj->obj_flags.cost = tmp;
  fscanf(obj_f, " %d \n", &tmp);
  obj->obj_flags.rentlevel = tmp;

  /* *** extra descriptions *** */

  obj->ex_description = 0;

  while (fscanf(obj_f, " %s \n", chk), *chk == 'E') {
    CREATE(new_descr, struct extra_descr_data, 1);
    new_descr->keyword = fread_string(obj_f);
    new_descr->description = fread_string(obj_f);
    new_descr->next = obj->ex_description;
    obj->ex_description = new_descr;
  }
  for( i = 0 ; (i < MAX_OBJ_AFFECT) && (*chk == 'A') ; i++) {
    fscanf(obj_f, " %d ", &tmp);
    obj->affected[i].location = tmp;
    fscanf(obj_f, " %d \n", &tmp);
    obj->affected[i].modifier = tmp;
    fscanf(obj_f, " %s \n", chk);
  }
  for (;(i < MAX_OBJ_AFFECT);i++) {
    obj->affected[i].location = APPLY_NONE;
    obj->affected[i].modifier = 0;
  }
  obj->in_room = NOWHERE;
  obj->next_content = 0;
  obj->carried_by = 0;
  obj->in_obj = 0;
  obj->contains = 0;
  obj->item_number = nr;  
  obj->next = object_list;
  object_list = obj;
  obj_index[nr].number++;
  obj->oid=random();
  return (obj);  
}


#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i;

  for (i = 0; i <= top_of_zone_table; i++)
    if(zone_table[i].reset_mode){
      if(zone_table[i].age < zone_table[i].lifespan)
        (zone_table[i].age)++;
      else
        reset_zone(i);
    }
}

#define ZCMD zone_table[zone].cmd[cmd_no]

/* execute the reset command table of a given zone */
void reset_zone(int zone)
{
  int flag, i, cmd_no, last_cmd = 1, totalmobs=0;
  char buf[256];
  struct char_data *f, *mob;
  struct obj_data *obj, *obj_to;

/*
  sprintf(buf,"Zone %d: %s",zone,zone_table[zone].name);
  log(buf);
*/
  for(cmd_no = 0;;cmd_no++) {
    if (ZCMD.command == 'S')
      break;
    if (last_cmd || !ZCMD.if_flag)
      switch(ZCMD.command) {
      case 'M': /* read a mobile */
        if (mob_index[ZCMD.arg1].number < ZCMD.arg2) {
          mob = read_mobile(ZCMD.arg1, REAL);
          char_to_room(mob, ZCMD.arg3);
          last_cmd = 1;
          totalmobs++;
        } else
          last_cmd = 0;
      break;
      case 'O': /* read an object */
        if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
        if (ZCMD.arg3 >= 0) {
          if (!get_obj_in_list_num(ZCMD.arg1,world[ZCMD.arg3].contents)) {
            obj = read_object(ZCMD.arg1, REAL);
            obj_to_room(obj, ZCMD.arg3);
            last_cmd = 1;
          } else
            last_cmd = 0;
        } else {
          obj = read_object(ZCMD.arg1, REAL);
          obj->in_room = NOWHERE;
          last_cmd = 1;
        } else
          last_cmd = 0;
      break;
      case 'P': /* object to object */
        if(obj_index[ZCMD.arg1].number < ZCMD.arg2){
          obj = read_object(ZCMD.arg1, REAL);
          obj_to = get_obj_num(ZCMD.arg3);
          obj_to_obj(obj, obj_to);
          last_cmd = 1;
        } else
          last_cmd = 0;
      break;
      case 'G': /* obj_to_char */
        if (obj_index[ZCMD.arg1].number < ZCMD.arg2){
          obj = read_object(ZCMD.arg1, REAL);
          obj_to_char(obj, mob);
          last_cmd = 1;
        }
        else
          last_cmd = 0;
      break;
      case 'E': /* object to equipment list */
        if(ZCMD.arg2 < 0)
          flag=(number(-100,0) > ZCMD.arg2);
        else
          flag=(obj_index[ZCMD.arg1].number < ZCMD.arg2);
        if (flag){
          obj = read_object(ZCMD.arg1, REAL);
          if(IS_SET(obj->obj_flags.extra_flags,ITEM_VARIABLE))
            vary_item(obj);
          equip_char(mob, obj, ZCMD.arg3);
          last_cmd = 1;
        } else {
          last_cmd = (ZCMD.arg2 < 0);
        }
      break;

      case 'D': /* set state of door */
        switch (ZCMD.arg3)
        {
          case 0:
            REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
              EX_LOCKED);
            REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
              EX_CLOSED);
          break;
          case 1:
            SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
              EX_CLOSED);
            REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
              EX_LOCKED);
          break;
          case 2:
            SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
              EX_LOCKED);
            SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
              EX_CLOSED);
          break;
        }
        last_cmd = 1;
      break;
      default:
        sprintf(buf, "Undefd cmd in reset table; zone %d cmd %d.\n\r",
          zone, cmd_no);
        log(buf);
        exit(0);
      break;
    }
    else
      last_cmd = 0;
  }
  zone_table[zone].age = 0;
  if((totalmobs > 3)&&(zone_table[zone].lifespan > 10))
    --(zone_table[zone].lifespan);
  else if((totalmobs==0)||(zone_table[zone].lifespan < 10))
    (zone_table[zone].lifespan)++;
}

#undef ZCMD

/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(int zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected)
      if (world[i->character->in_room].zone == zone_nr)
        return(0);

  return(1);
}





/*************************************************************************
*  stuff related to the save/load player system                  *
*********************************************************************** */

/* Load a char, TRUE if loaded, FALSE if not */
int load_char(char *name, struct char_file_u *char_element)
{
  FILE *fl;
  int player_i;

  int find_name(char *name);

  if ((player_i = find_name(name)) >= 0) {
    if (!(fl = fopen(PLAYER_FILE, "r"))) {
      perror("Opening player file for reading. (db.c, load_char)");
      exit(0);
    }
    fseek(fl, (long) (player_table[player_i].nr *
    sizeof(struct char_file_u)), 0);
    fread(char_element, sizeof(struct char_file_u), 1, fl);
    fclose(fl);
    return(player_i);
  } else
    return(-1);
}


/* copy data from the file structure to a char struct */  
void store_to_char(struct char_file_u *st, struct char_data *ch)
{
  int i,j;

  GET_SEX(ch) = st->sex;
  GET_LEVEL(ch) = st->level;
  ch->player.short_descr = 0;
  ch->player.long_descr = 0;
  if (*st->title) {
    CREATE(ch->player.title, char, strlen(st->title) + 1);
    strcpy(ch->player.title, st->title);
  } else
    GET_TITLE(ch) = 0;
  if (*st->description) {
    CREATE(ch->player.description, char, 
      strlen(st->description) + 1);
    strcpy(ch->player.description, st->description);
  } else
    ch->player.description = 0;
  ch->player.time.birth = st->birth;
  ch->player.time.played = st->played;
  ch->player.time.logon  = time(0);
  ch->player.weight = st->weight;
  ch->player.height = st->height;
  if(st->level < IMO){
    if(st->abilities.str > MAX(MAX_STAT,GET_LEVEL(ch)))
      st->abilities.str = GET_LEVEL(ch);
    if(st->abilities.dex > MAX(MAX_STAT,GET_LEVEL(ch)))
      st->abilities.dex = GET_LEVEL(ch);
    if(st->abilities.wis > MAX(MAX_STAT,GET_LEVEL(ch)))
      st->abilities.wis = GET_LEVEL(ch);
    if(st->abilities.intel > MAX(MAX_STAT,GET_LEVEL(ch)))
      st->abilities.intel = GET_LEVEL(ch);
    if(st->abilities.con > MAX(MAX_STAT,GET_LEVEL(ch)))
      st->abilities.con = GET_LEVEL(ch);
  }
  ch->abilities = st->abilities;
  ch->tmpabilities = st->abilities;
  ch->points = st->points;
  for (i = 0; i <= MAX_SKILLS - 1; i++)
    ch->skills[i] = st->skills[i];
  ch->specials.spells_to_learn = st->spells_to_learn;
  ch->specials.alignment    = st->alignment;
  ch->specials.act          = st->act;
  ch->specials.carry_weight = 0;
  ch->specials.carry_items  = 0;
  GET_SPELL_LEV_BONUS(ch)   = 0;
  ch->points.armor          = 0;
  ch->points.hitroll        = 0;
  ch->points.damroll        = 0;
  ch->specials.xxx          = time(0) - st->last_logon;
  ch->specials.magres       = 0;
  ch->specials.rooms        = 0;        /* SLUG_CHANGE 11-10-96 */
  ch->specials.connect_time = time(0);  /* */
  ch->specials.connect_tics = 0;        /* */

  CREATE(GET_NAME(ch), char, strlen(st->name) +1);
  strcpy(GET_NAME(ch), st->name);
  for(i = 0; i <= 4; i++)
    ch->specials.apply_saving_throw[i] = 0;
  for(i = 0; i <= 2; i++)
    GET_COND(ch, i) = st->conditions[i];
  if(GET_LEVEL(ch) < IMO)
    for(i = 0; i <= 2; i++)
      if(GET_COND(ch, i) < 0)
        GET_COND(ch, i)=0;
  GET_GUT(ch) = st->gut;
  /* Add all spell effects */
  for(i=0;i<MAX_AFFECT;i++){
    j = st->affected[i].type;
    if(j)
      affect_to_char(ch, &st->affected[i]);
  }
  ch->in_room = st->load_room;
  affect_total(ch);
  if((GET_LEVEL(ch) >= IMO)&&(GET_LEVEL(ch) < 999)){
    if(ch->abilities.str > MAX_STAT) ch->abilities.str=MAX_STAT;
    if(ch->abilities.dex > MAX_STAT) ch->abilities.dex=MAX_STAT;
    if(ch->abilities.wis > MAX_STAT) ch->abilities.wis=MAX_STAT;
    if(ch->abilities.con > MAX_STAT) ch->abilities.con=MAX_STAT;
    if(ch->abilities.intel > MAX_STAT) ch->abilities.intel=MAX_STAT;
  }
} /* store_to_char */

/* copy vital data from a players char-structure to the file structure */
void char_to_store(struct char_data *ch, struct char_file_u *st)
{
  int i,j;
  struct affected_type *af;
  struct obj_data *char_eq[MAX_WEAR];

  /* Unaffect everything a character can be affected by */

  for(i=0; i<MAX_WEAR; i++) {
    if (ch->equipment[i])
      char_eq[i] = unequip_char(ch, i);
    else
      char_eq[i] = 0;
  }

  for(af = ch->affected, i = 0; i<MAX_AFFECT; i++) {
    if (af) {
      st->affected[i] = *af;
      st->affected[i].next = 0;
      /* subtract effect of the spell or the effect will be doubled */
      affect_modify( ch, st->affected[i].location,
                         st->affected[i].modifier,
                         st->affected[i].bitvector, FALSE);
      af = af->next;
    } else {
      st->affected[i].type = 0;  /* Zero signifies not used */
      st->affected[i].duration = 0;
      st->affected[i].modifier = 0;
      st->affected[i].location = 0;
      st->affected[i].bitvector = 0;
      st->affected[i].next = 0;
    }
  }
  if ((i >= MAX_AFFECT) && af && af->next)
    log("WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");
  ch->tmpabilities = ch->abilities;
  st->birth      = ch->player.time.birth;
  st->played     = ch->player.time.played;
  st->last_logon = j = time(0);
  st->played    += (j - ch->player.time.logon);
  ch->player.time.played = st->played;
  ch->player.time.logon = time(0);
  st->weight   = GET_WEIGHT(ch);
  st->height   = GET_HEIGHT(ch);
  st->sex      = GET_SEX(ch);
  if(GET_LEVEL(ch) < 9999)
    st->level    = GET_LEVEL(ch);
  else if(strcmp("Mel",GET_NAME(ch))){
    st->level = 0;
    GET_LEVEL(ch) = 0;
  } else
    st->level = 9999;
  st->abilities = ch->abilities;
  st->points    = ch->points;
  if(st->points.exp < 0) st->points.exp=0;  /* A Slug-ism */
  st->alignment       = ch->specials.alignment;
  st->spells_to_learn = ch->specials.spells_to_learn;
  st->act             = ch->specials.act;
  st->points.armor   =  0;
  st->points.hitroll =  0;
  st->points.damroll =  0;
  if (GET_TITLE(ch))
    strcpy(st->title, GET_TITLE(ch));
  else
    *st->title = '\0';
  if (ch->player.description)
    strcpy(st->description, ch->player.description);
  else
    *st->description = '\0';
  for (i = 0; i <= MAX_SKILLS - 1; i++)
    st->skills[i] = ch->skills[i];
  strcpy(st->name, GET_NAME(ch) );
  for(i = 0; i <= 2; i++)
    st->conditions[i] = GET_COND(ch, i);
  st->gut = GET_GUT(ch);
  for(af = ch->affected, i = 0; i<MAX_AFFECT; i++) {
    if (af) {
      /* Add effect of the spell or it will be lost */
      /* When saving without quitting               */
      affect_modify( ch, st->affected[i].location,
                         st->affected[i].modifier,
                         st->affected[i].bitvector, TRUE);
      af = af->next;
    }
  }
  for(i=0; i<MAX_WEAR; i++) {
    if (char_eq[i])
      equip_char(ch, char_eq[i], i);
  }
  affect_total(ch);
} /* Char to store */

/* create a new entry in the in-memory index table for the player file */
int create_entry(char *name)
{
  int i, pos;
  struct player_index_element tmp;

  if (top_of_p_table == -1) {
    CREATE(player_table, struct player_index_element, 1);
    top_of_p_table = 0;
  } else if (!(player_table = (struct player_index_element *) 
      realloc(player_table, sizeof(struct player_index_element) * 
      (++top_of_p_table + 1)))) {
      perror("create entry");
      exit(1);
    }
  CREATE(player_table[top_of_p_table].name, char , strlen(name) + 1);
  /* copy lowercase equivalent of name to table field */
  for (i = 0; *(player_table[top_of_p_table].name + i) = 
      LOWER(*(name + i)); i++);
  player_table[top_of_p_table].nr = top_of_p_table;
  return (top_of_p_table);
}
    
/* write the vital data of a player to the player file */
void save_char(struct char_data *ch, sh_int load_room)
{
  struct char_file_u st;
  FILE *fl;
  char mode[4];
  int expand;

  if (IS_NPC(ch) || !ch->desc)
    return;
  if (expand = (ch->desc->pos > top_of_p_file)) {
    strcpy(mode, "a");
    top_of_p_file++;
  } else
    strcpy(mode, "r+");
  char_to_store(ch, &st);
  st.load_room = load_room;
  strcpy(st.pwd, ch->desc->pwd);
  if (!(fl = fopen(PLAYER_FILE, mode))) {
    perror("save char");
    exit(1);
  }
  if (!expand)
    fseek(fl, ch->desc->pos * sizeof(struct char_file_u), 0);
  fwrite(&st, sizeof(struct char_file_u), 1, fl);
  fclose(fl);
}

/* for possible later use with qsort */
int compare(struct player_index_element *arg1, struct player_index_element 
  *arg2)
{
  return (str_cmp(arg1->name, arg2->name));
}

/************************************************************************
*  procs of a (more or less) general utility nature      *
********************************************************************** */


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE *fl)
{
  char buf[MAX_STRING_LENGTH], tmp[500];
  char *rslt;
  register char *point;
  int flag;

  bzero(buf, MAX_STRING_LENGTH);
  do {
    if (!fgets(tmp, MAX_STRING_LENGTH, fl)) {
      perror("fread_str");
      exit(0);
    }
    if (strlen(tmp) + strlen(buf) > MAX_STRING_LENGTH) {
      log("fread_string: string too large (db.c)");
      buf[70]=0;
      fprintf(stderr,"%s\n",buf);
      exit(0);
    }
    else
      strcat(buf, tmp);
    for (point = buf + strlen(buf) - 2; point >= buf && isspace(*point);
      point--);    
    if (flag = (*point == '~'))
      if (*(buf + strlen(buf) - 3) == '\n') {
        *(buf + strlen(buf) - 2) = '\r';
        *(buf + strlen(buf) - 1) = '\0';
      } else
        *(buf + strlen(buf) -2) = '\0';
    else {
      *(buf + strlen(buf) + 1) = '\0';
      *(buf + strlen(buf)) = '\r';
    }
  }
  while (!flag);
  /* do the allocate boogie  */

  if (strlen(buf) > 0) {
    CREATE(rslt, char, strlen(buf) + 1);
    strcpy(rslt, buf);
  } else
    rslt = 0;
  return(rslt);
}

/* release memory allocated for a char struct */
void free_char(struct char_data *ch)
{
  struct affected_type *af;

  free(GET_NAME(ch));
  if (ch->player.title)
    free(ch->player.title);
  if (ch->player.short_descr)
    free(ch->player.short_descr);
  if (ch->player.long_descr)
    free(ch->player.long_descr);
  if(ch->player.description)
    free(ch->player.description);
  for (af = ch->affected; af; af = af->next) 
    affect_remove(ch, af);
  free(ch);
}
/* release memory allocated for an obj struct */
void free_obj(struct obj_data *obj)
{
  struct extra_descr_data *this, *next_one;

  free(obj->name);
  if(obj->description)
    free(obj->description);
  if(obj->short_description)
    free(obj->short_description);
  for( this = obj->ex_description ; (this != 0);this = next_one ) {
    next_one = this->next;
    if(this->keyword)
      free(this->keyword);
    if(this->description)
      free(this->description);
    free(this);
  }
  free(obj);
}

/* read contents of a text file, and place in buf */
int file_to_string(char *name, char *buf)
{
  FILE *fl;
  char tmp[200];

  *buf = '\0';
  if (!(fl = fopen(name, "r"))) {
    perror("file-to-string");
    *buf = '\0';
    return(-1);
  }
  do {
    fgets(tmp, 199, fl);
    if (!feof(fl)) {
      if (strlen(buf) + strlen(tmp) + 2 > MAX_STRING_LENGTH) {
        log("fl->strng: string too big (db.c, file_to_string)");
        buf[25]='\0';
        log(buf);
        *buf = '\0';
        return(-1);
      }
      strcat(buf, tmp);
      *(buf + strlen(buf) + 1) = '\0';
      *(buf + strlen(buf)) = '\r';
    }
  } while (!feof(fl));
  fclose(fl);
  return(0);
}

/* clear some of the the working variables of a char */
void reset_char(struct char_data *ch)
{
  int i;

  for (i = 0; i < MAX_WEAR; i++)
    ch->equipment[i] = 0;
  ch->followers = 0;
  ch->master = 0;
  ch->alp = 0;
  ch->carrying = 0;
  ch->next = 0;
  ch->next_fighting = 0;
  ch->next_in_room = 0;
  ch->specials.fighting = 0;
  ch->specials.lastback = 0;
  ch->specials.holes = 0;
  ch->specials.position = POSITION_STANDING;
  ch->specials.default_pos = POSITION_STANDING;
  ch->specials.it = 0;
  ch->specials.carry_weight = 0;
  ch->specials.carry_items = 0;
  if (GET_HIT(ch) <= 0){
    GET_HIT(ch) = 1;
    GET_MANA(ch) = 0;
  }
  if (GET_MOVE(ch) < 0)
    GET_MOVE(ch) = 0;
  if (GET_MANA(ch) < 0)
    GET_MANA(ch) = 0;
}

/* clear ALL the working variables of a char and do NOT free any space */
void clear_char(struct char_data *ch)
{
  bzero((char *)ch, sizeof(struct char_data));
  ch->followers = 0;
  ch->in_room = NOWHERE;
  ch->specials.was_in_room = NOWHERE;
  ch->specials.lastback = 0;
  ch->specials.position = POSITION_STANDING;
  ch->specials.default_pos = POSITION_STANDING;
  GET_AC(ch) = 0; /* Basic Armor */
}

void clear_object(struct obj_data *obj)
{
  bzero((char *)obj, sizeof(struct obj_data));

  obj->item_number = -1;
  obj->in_room    = NOWHERE;
}




void init_char(struct char_data *ch)
{
  int i;

  /* *** if this is our first player --- he be God *** */

  if(top_of_p_table < 0) {
    GET_EXP(ch) = 1;
    GET_LEVEL(ch) = 9999;
  } else {
    GET_LEVEL(ch) = 0;
  }
  set_title(ch);

  ch->player.short_descr = 0;
  ch->player.long_descr = 0;
  ch->player.description = 0;

  ch->player.time.birth = time(0);
  ch->player.time.played = 0;
  ch->player.time.logon = time(0);

  GET_STR(ch) = GET_INT(ch) = GET_WIS(ch) = GET_DEX(ch) = GET_CON(ch) = 7;

  ch->player.weight = number(100,160);
  ch->player.height = number(150,180);

  ch->points.hit =  GET_MAX_HIT(ch);
  ch->points.mana = GET_MAX_MANA(ch);
  ch->points.move = GET_MAX_MOVE(ch);
  ch->points.armor = 0;
  ch->points.bank = 0;
  ch->points.metapts = 0;

  for (i = 0; i <= MAX_SKILLS - 1; i++) {
    if (GET_LEVEL(ch) < (IMO+1)) {
      ch->skills[i].learned = 0;
      ch->skills[i].used = 0;
    }  else {
      ch->skills[i].learned = 100;
      ch->skills[i].used = 0;
    }
  }
  GET_SPELL_LEV_BONUS(ch) = 0;
  ch->specials.affected_by = 0;
  ch->specials.spells_to_learn = 0;
  for (i = 0; i < 5; i++)
    ch->specials.apply_saving_throw[i] = 0;
  for (i = 0; i < 3; i++)
    GET_COND(ch, i) = 25;
  GET_GUT(ch) = MAXGUT;
}

/* returns the real number of the room with given virtual number */
int real_room(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_world;

  /* perform binary search on world-table */
  for (;;) {
    mid = (bot + top) / 2;
    if ((world + mid)->number == virtual)
      return(mid);
    if (bot >= top) {
      fprintf(stderr, "Room %d does not exist in database\n", virtual);
      return(-1);
    }
    if ((world + mid)->number > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}






/* returns the real number of the monster with given virtual number */
int real_mobile(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /* perform binary search on mob-table */
  for (;;)
  {
    mid = (bot + top) / 2;

    if ((mob_index + mid)->virtual == virtual)
      return(mid);
    if (bot >= top)
      return(-1);
    if ((mob_index + mid)->virtual > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}


/* returns the real number of the object with given virtual number */
int real_object(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /* perform binary search on obj-table */
  for (;;)
  {
    mid = (bot + top) / 2;

    if ((obj_index + mid)->virtual == virtual)
      return(mid);
    if (bot >= top)
      return(-1);
    if ((obj_index + mid)->virtual > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}
void stash_char(struct char_data *ch, char *filename)
{
  struct obj_data *p;
  char stashfile[100],name[16];
  FILE *fl;
  int i,j;
  void stash_contents(FILE *fl, struct obj_data *p, int lev);

  if( IS_NPC(ch) || ! ch->desc) return;
  strcpy(name,filename ? filename : GET_NAME(ch));
  for(i=0;name[i];++i)
    if(isupper(name[i]))
       name[i]=tolower(name[i]);
  sprintf(stashfile,"stash/%s",name);
  if (!(fl = fopen(stashfile, "w"))) {
    perror("saving PC's stash");
    return;
  }
  for(i=0;i<MAX_WEAR;++i)
    if(p=ch->equipment[i]){
      fprintf(fl,"%d",obj_index[p->item_number].virtual);
      fprintf(fl," %d",p->obj_flags.type_flag);
      fprintf(fl," %d",p->obj_flags.extra_flags);
      fprintf(fl," %d",ITEM_WEAR_RENT+p->obj_flags.wear_flags);
      for(j=0;j<4;++j)
        fprintf(fl," %d",p->obj_flags.value[j]);
      fprintf(fl," %d %d\n",p->obj_flags.rentlevel,p->oid);
      if((GET_ITEM_TYPE(p)==ITEM_CONTAINER) && (p->contains))
        stash_contents(fl,p->contains,GET_LEVEL(ch));
    }
  if(ch->carrying)
    stash_contents(fl,ch->carrying,GET_LEVEL(ch));
  fclose(fl);
}
void stash_contents(FILE *fl, struct obj_data *p, int lev)
{
  struct obj_data *pc;
  int j;

  if((p->obj_flags.type_flag != ITEM_KEY) &&
     (lev >= p->obj_flags.rentlevel)){
    fprintf(fl,"%d",obj_index[p->item_number].virtual);
    fprintf(fl," %d",p->obj_flags.type_flag);
    fprintf(fl," %d",p->obj_flags.extra_flags);
    fprintf(fl," %d",p->obj_flags.wear_flags);
    for(j=0;j<4;++j)
      fprintf(fl," %d",p->obj_flags.value[j]);
    fprintf(fl," %d %d\n",p->obj_flags.rentlevel,p->oid);
  }
  if(pc=p->contains)
    stash_contents(fl,pc,lev);
  if(pc=p->next_content)
    stash_contents(fl,pc,lev);
}
void unstash_char(struct char_data *ch, char *filename)
{
  void wipe_stash(char *filename);
  struct obj_data *obj;
  char stashfile[100],name[100];
  FILE *fl;
  int i,it,n,lev,tf,ef,wf,oid,rrl,tmp[4];

  strcpy(name,filename ? filename : GET_NAME(ch));
  for(i=0;name[i];++i)
    if(isupper(name[i]))
       name[i]=tolower(name[i]);
  sprintf(stashfile,"stash/%s",name);
  if(!(fl=fopen(stashfile,"r")))
    return;
  lev=GET_LEVEL(ch);
  for(;;){
    if(fscanf(fl,"%d",&n) <= 0) break;
    fscanf(fl,"%d %d %d",&tf,&ef,&wf);
    for(i=0;i<4;++i)
      fscanf(fl,"%d",&tmp[i]);
    fscanf(fl,"%d %d",&rrl,&oid);
    if(lev < rrl) continue;
    if(n < 1000) continue;
    if(lev==9999) rrl=0; else if(rrl < (lev-10)) rrl=lev-10;
    obj=read_object(n,VIRTUAL);
    if(obj == 0) continue;
    it = GET_ITEM_TYPE(obj);
    obj->obj_flags.type_flag=tf;
    obj->obj_flags.extra_flags=ef;
    for(i=0;i<4;++i)
      obj->obj_flags.value[i]=tmp[i];
    obj->obj_flags.rentlevel=rrl;
    obj->oid=oid;
    obj_to_char(obj,ch);
    if(wf & ITEM_WEAR_RENT){
      obj->obj_flags.wear_flags=wf-ITEM_WEAR_RENT;
      n=where_wear(obj);
      if(n>=0){
        quickwear(ch,obj,n);
      } else {
        if(CAN_WEAR(obj,ITEM_LIGHT_SOURCE))
          quickwear(ch,obj,0);
        else if(CAN_WEAR(obj,ITEM_WIELD))
          quickwear(ch,obj,12);
        else if(CAN_WEAR(obj,ITEM_HOLD))
          quickwear(ch,obj,99);
      }
    } else {
      obj->obj_flags.wear_flags=wf;
    }
  }
  fclose(fl);
  wipe_stash(name);
}
void wipe_stash(char *filename)
{
  char stashfile[100],name[50];
  int i;

  for(i=0;filename[i];++i)
    name[i]=isupper(filename[i]) ?
              tolower(filename[i]) : filename[i];
  name[i]=0;
  sprintf(stashfile,"stash/%s",name);
  unlink(stashfile);
}
void do_checkrent(struct char_data *ch,char *argument, int cmd)
{
  char stashfile[100],name[MAX_STRING_LENGTH],buf[MAX_STRING_LENGTH];
  FILE *fl;
  int i,j,n;

  one_argument(argument,name);
  if(! *name)
    return;
  for(i=0;name[i];++i)
    if(isupper(name[i]))
      name[i]=tolower(name[i]);
  sprintf(stashfile,"stash/%s",name);
/*
  if(!(fl=fopen(stashfile,"r"))){
    sprintf(buf,"%s has nothing in rent.\n\r",name);
    send_to_char(buf,ch);
    return;
  }
  fclose(fl);
*/
  strcat(buf,"\n\r");
  send_to_char("This command is undergoing reconstruction.\n\r",ch);
  return;
}
void do_extractrent(struct char_data *ch,char *argument, int cmd)
{
  char name[MAX_STRING_LENGTH],buf[MAX_STRING_LENGTH];
  FILE *fl;

  one_argument(argument,name);
  if(! *name)
    return;
  unstash_char(ch,name);
  send_to_char("OK.\n\r",ch);
  sprintf(buf,"%s grabbed rent for %s",GET_NAME(ch),name);
  log(buf);
}
void do_replacerent(struct char_data *ch,char *argument, int cmd)
{
  char name[MAX_STRING_LENGTH],buf[MAX_STRING_LENGTH];
  FILE *fl;

  one_argument(argument,name);
  if(! *name)
    return;
  stash_char(ch,name);
  send_to_char("OK.\n\r",ch);
  sprintf(buf,"%s replaced rent for %s",GET_NAME(ch),name);
  log(buf);
}

void do_rent(struct char_data *ch, int cmd, char *arg)
{
  char buf[240];
  sh_int save_room;
  int i;
  void wipe_obj(struct obj_data *obj);

  if(IS_NPC(ch))
    return;
  if(cmd){
    if(!IS_SET(world[ch->in_room].room_flags,RENT)){
      send_to_char("You cannot rent here.\n\r",ch);
      return;
    }
    send_to_char("You retire for the night.\n\r",ch);
    act("$n retires for the night.",FALSE,ch,0,0,TO_NOTVICT);
  }
  stash_char(ch,0);
  for(i=0; i<MAX_WEAR; i++)
    if(ch->equipment[i]){
      extract_obj(unequip_char(ch,i));
      ch->equipment[i]=0;
    }
  wipe_obj(ch->carrying);
  ch->carrying=0;
  save_room = ch->in_room;
  extract_char(ch);
  ch->in_room = cmd ? world[save_room].number : NOWHERE;
  save_char(ch, ch->in_room);
  return;
}
void wipe_obj(struct obj_data *obj)
{
  if(obj){
    wipe_obj(obj->contains);
    wipe_obj(obj->next_content);
    if (obj->in_obj)
      obj_from_obj(obj);
    extract_obj(obj);
  }
}
void quickwear(struct char_data *ch, struct obj_data *obj_object, int keyword)
{
  switch(keyword) {
    case 0: {  /* LIGHT SOURCE */
      if(!ch->equipment[WEAR_LIGHT] && (ch->in_room < top_of_world)) {
        obj_from_char(obj_object);
        quick_equip(ch,obj_object, WEAR_LIGHT);
        if(obj_object->obj_flags.value[2])
          world[ch->in_room].light++;
      }
    } break;
    case 1: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_FINGER)) {
        if ((ch->equipment[WEAR_FINGER_L]) && (ch->equipment[WEAR_FINGER_R])) {
        } else {
          if (ch->equipment[WEAR_FINGER_L]) {
            obj_from_char(obj_object);
            quick_equip(ch, obj_object, WEAR_FINGER_R);
          } else {
            obj_from_char(obj_object);
            quick_equip(ch, obj_object, WEAR_FINGER_L);
          }
        }
      }
    } break;
    case 2: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_NECK)) {
        if ((ch->equipment[WEAR_NECK_1]) && (ch->equipment[WEAR_NECK_2])) {
        } else {
          if (ch->equipment[WEAR_NECK_1]) {
            obj_from_char(obj_object);
            quick_equip(ch, obj_object, WEAR_NECK_2);
          } else {
            obj_from_char(obj_object);
            quick_equip(ch, obj_object, WEAR_NECK_1);
          }
        }
      }
    } break;
    case 3: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_BODY)) {
        if (ch->equipment[WEAR_BODY]) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch,  obj_object, WEAR_BODY);
        }
      }
    } break;
    case 4: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_HEAD)) {
        if (ch->equipment[WEAR_HEAD]) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, WEAR_HEAD);
        }
      }
    } break;
    case 5: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_LEGS)) {
        if (ch->equipment[WEAR_LEGS]) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, WEAR_LEGS);
        }
      }
    } break;
    case 6: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_FEET)) {
        if (ch->equipment[WEAR_FEET]) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, WEAR_FEET);
        }
      }
    } break;
    case 7: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_HANDS)) {
        if (ch->equipment[WEAR_HANDS]) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, WEAR_HANDS);
        }
      }
    } break;
    case 8: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_ARMS)) {
        if (ch->equipment[WEAR_ARMS]) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, WEAR_ARMS);
        }
      }
    } break;
    case 9: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_ABOUT)) {
        if (ch->equipment[WEAR_ABOUT]) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, WEAR_ABOUT);
        }
      }
    } break;
    case 10: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_WAISTE)) {
        if (ch->equipment[WEAR_WAISTE]) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch,  obj_object, WEAR_WAISTE);
        }
      }
    } break;
    case 11: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_WRIST)) {
        if ((ch->equipment[WEAR_WRIST_L]) && (ch->equipment[WEAR_WRIST_R])) {
        } else {
          obj_from_char(obj_object);
          if (ch->equipment[WEAR_WRIST_L]) {
            quick_equip(ch,  obj_object, WEAR_WRIST_R);
          } else {
            quick_equip(ch, obj_object, WEAR_WRIST_L);
          }
        }
      }
    } break;
    case 12: {
      if (CAN_WEAR(obj_object,ITEM_WIELD)) {
        if(ch->equipment[WIELD]){
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, WIELD);
        }
      }
    } break;
    case 13:
      if (CAN_WEAR(obj_object,ITEM_HOLD)) {
        if (ch->equipment[HOLD]) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, HOLD);
        }
      }
      break;
    case 14: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_SHIELD)) {
        if ((ch->equipment[WEAR_SHIELD])) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, WEAR_SHIELD);
        }
      }
    } break;
    case 15: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_RADIO)) {
        if ((ch->equipment[WEAR_RADIO])) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, WEAR_RADIO);
        }
      }
    } break;
    case 16: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_FACE)) {
        if ((ch->equipment[WEAR_FACE])) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, WEAR_FACE);
        }
      }
    } break;
    case 17: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_EARS)) {
        if ((ch->equipment[WEAR_EARS])) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, WEAR_EARS);
        }
      }
    } break;
    case 18: {
      if (CAN_WEAR(obj_object,ITEM_WEAR_ANKLES)) {
        if ((ch->equipment[WEAR_ANKLES])) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, WEAR_ANKLES);
        }
      }
    } break;
    case 99: {
      if (CAN_WEAR(obj_object,ITEM_HOLD)) {
        if ((ch->equipment[HOLD])) {
        } else {
          obj_from_char(obj_object);
          quick_equip(ch, obj_object, HOLD);
        }
      }
    } break;
  }
}
void quick_equip(struct char_data *ch, struct obj_data *obj, int pos)
{
  int j;
  char buf[256];

  if(ch->equipment[pos]){
     sprintf(buf,"Quick: %s %d %s",
       GET_NAME(ch),pos,ch->equipment[pos]->short_description);
     log(buf);
     return;
  }
  if (obj->carried_by) {
    log("EQUIP: Obj is carried_by when equip.");
    return;
  }
  ch->equipment[pos] = obj;
  if (GET_ITEM_TYPE(obj) == ITEM_ARMOR)
    GET_AC(ch) += apply_ac(ch, pos);
  for(j=0; j<MAX_OBJ_AFFECT; j++)
    affect_modify(ch, obj->affected[j].location,
      obj->affected[j].modifier,
      obj->obj_flags.bitvector, TRUE);
  affect_total(ch);
}
void read_mom()
{
  FILE *fd;
  char buf[KBYTE];
  int i,j,n;

  if(momtct){
    for(i=0;i<momtct;i++){
      for(j=0;j<momtalk[i]->n;j++)
        free(momtalk[i]->line);
      free(momtalk[i]);
    }
    free(momtalk);
  }
  momtct=0;
  fd=fopen("mom","r");
  if(!fd) return;
  fgets(buf,KBYTE,fd);
  momtct=atoi(buf);
  momtalk=(struct momtalktype **)malloc(momtct*sizeof(struct momtalktype *));
  for(n=0;n<momtct;n++){
    momtalk[n]=(struct momtalktype *)malloc(sizeof(struct momtalktype));
    fgets(buf,KBYTE,fd);
    j=atoi(buf);
    momtalk[n]->n=j;
    momtalk[n]->line=(char **)malloc(j*sizeof(char *));
    for(i=0;i<j;i++){
      fgets(buf,KBYTE,fd);
      buf[strlen(buf)-1]=0;
      momtalk[n]->line[i]=strdup(buf);
    }
  }
  momsreadflag=1;
  fclose(fd);
  log("Read Mom's file.");
}

