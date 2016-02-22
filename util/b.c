#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

typedef signed char sbyte;
typedef unsigned char ubyte;
typedef signed short int sh_int;
typedef unsigned short int ush_int;
typedef char bool;
typedef char byte;

#define MAX_STRING_LENGTH   4095
#define MAX_INPUT_LENGTH    1024
#define MAX_MESSAGES         128
#define MAX_ITEMS            153

/* Bitvector For 'wear_flags' */

#define ITEM_TAKE              1 
#define ITEM_WEAR_FINGER       2
#define ITEM_WEAR_NECK         4
#define ITEM_WEAR_BODY         8
#define ITEM_WEAR_HEAD        16
#define ITEM_WEAR_LEGS        32
#define ITEM_WEAR_FEET        64
#define ITEM_WEAR_HANDS      128 
#define ITEM_WEAR_ARMS       256
#define ITEM_WEAR_SHIELD     512
#define ITEM_WEAR_ABOUT     1024 
#define ITEM_WEAR_WAISTE    2048
#define ITEM_WEAR_WRIST     4096
#define ITEM_WIELD          8192
#define ITEM_HOLD          16384
#define ITEM_WEAR_RADIO    32768
#define ITEM_LIGHT_SOURCE  65536
#define ITEM_WEAR_FACE    131072
#define ITEM_WEAR_EARS    262144
#define ITEM_WEAR_RENT    524288
#define ITEM_WEAR_ANKLES 1048576

/* UNUSED, CHECKS ONLY FOR ITEM_LIGHT #define ITEM_LIGHT_SOURCE  65536 */

/* Bitvector for 'extra_flags' */

#define ITEM_GLOW            1
#define ITEM_HUM             2
#define ITEM_SFX             4
#define ITEM_LOCK            8
#define ITEM_CHEAP          16
#define ITEM_INVISIBLE      32
#define ITEM_MAGIC          64
#define ITEM_NODROP        128
#define ITEM_BLESS         256
#define ITEM_ANTI_GOOD     512 /* not usable by good people    */
#define ITEM_ANTI_EVIL    1024 /* not usable by evil people    */
#define ITEM_ANTI_NEUTRAL 2048 /* not usable by neutral people */
#define ITEM_PRIME        4096
#define ITEM_POOF         8192
#define ITEM_POOFSOON    16384
#define ITEM_VARIABLE    32768
#define ITEM_ANTIPRIME   65536
#define ITEM_NOKICK     131072
#define ITEM_MUTABLE    262144
#define ITEM_NO_PUT_IN  524288

/* Some different kind of liquids */
#define LIQ_WATER      0
#define LIQ_BEER       1
#define LIQ_WINE       2
#define LIQ_ALE        3
#define LIQ_DARKALE    4
#define LIQ_WHISKY     5
#define LIQ_LEMONADE   6
#define LIQ_FIREBRT    7
#define LIQ_MILKSHAKE  8
#define LIQ_SLIME      9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFE      12
#define LIQ_BLOOD      13
#define LIQ_DIETCOKE   14
#define LIQ_COKE       15

/* for containers  - value[1] */

#define CONT_CLOSEABLE      1
#define CONT_PICKPROOF      2
#define CONT_CLOSED         4
#define CONT_LOCKED         8
#define CONT_COMBO         16

struct extra_descr_data
{
  char *keyword;                 /* Keyword in look/examine          */
  char *description;             /* What to see                      */
  struct extra_descr_data *next; /* Next in list                     */
};

#define MAX_OBJ_AFFECT 3
#define OBJ_NOTIMER    -7000000

struct obj_flag_data
{
  int value[4];       /* Values of the item (see list)    */
  byte type_flag;     /* Type of item                     */
  int wear_flags;     /* Where you can wear it            */
  int extra_flags;    /* If it hums,glows etc             */
  int weight;         /* Weigt what else                  */
  int cost;           /* Value when sold (gp.)            */
  int rentlevel;
  int timer;          /* Timer for object                 */
  long bitvector;     /* To set chars bits                */
};

/* Used in OBJ_FILE_ELEM *DO*NOT*CHANGE* */
struct obj_affected_type {
  byte location;      /* Which ability to change (APPLY_XXX) */
  long modifier;     /* How much it changes by              */
};

/* The following defs and structures are related to char_data   */

/* For 'equipment' */

#define WEAR_LIGHT      0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_SHIELD    11
#define WEAR_ABOUT     12
#define WEAR_WAISTE    13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WIELD          16
#define HOLD           17
#define WEAR_RADIO     18
#define WEAR_FACE      19
#define WEAR_EARS      20
#define WEAR_ANKLES    21

/* For 'char_player_data' */

#define MAX_WEAR    22
#define MAX_SKILLS  82
#define MAX_AFFECT  25    /* Used in CHAR_FILE_U *DO*NOT*CHANGE* */

#define DRUNK        0
#define FULL         1
#define THIRST       2

/* Bitvector for 'affected_by' */
#define AFF_BLIND             1
#define AFF_INVISIBLE         2
#define AFF_DETECT_EVIL       4
#define AFF_DETECT_INVISIBLE  8
#define AFF_DETECT_MAGIC      16
#define AFF_SENSE_LIFE        32
#define AFF_HOLD              64
#define AFF_SANCTUARY         128
#define AFF_GROUP             256
#define AFF_CURSE             1024
#define AFF_REGEN             2048
#define AFF_POISON            4096
#define AFF_PROTECT_EVIL      8192
#define AFF_CLARITY           16384
#define AFF_INFRAVISION       32768
#define AFF_FARSEE            65536
#define AFF_SLEEP             131072
#define AFF_FEARLESSNESS      262144
#define AFF_SNEAK             524288
#define AFF_HIDE              1048576
#define AFF_FEAR              2097152
#define AFF_CHARM             4194304
#define AFF_FOLLOW            8388608
#define AFF_HASTE             16777216
#define AFF_HOLDALIGN         33554432
#define AFF_HYPERREGEN        67108864
#define AFF_STUPIDITY         134217728
#define AFF_NOSUMMON          268435456

/* modifiers to char's abilities */

#define APPLY_NONE              0
#define APPLY_STR               1
#define APPLY_DEX               2
#define APPLY_INT               3
#define APPLY_WIS               4
#define APPLY_CON               5
#define APPLY_SEX               6
#define APPLY_LEVEL             8
#define APPLY_AGE               9
#define APPLY_KICK             10
#define APPLY_XXXX             11
#define APPLY_MANA             12
#define APPLY_HIT              13
#define APPLY_MOVE             14
#define APPLY_GOLD             15
#define APPLY_EXP              16
#define APPLY_AC               17
#define APPLY_ARMOR            17
#define APPLY_HITROLL          18
#define APPLY_DAMROLL          19
#define APPLY_SAVING_PARA      20
#define APPLY_SAVING_ROD       21
#define APPLY_SAVING_SUNBURST  22
#define APPLY_SAVING_BREATH    23
#define APPLY_SAVING_SPELL     24
#define APPLY_MAG_RES          25

/* sex */
#define SEX_NEUTRAL   0
#define SEX_MALE      1
#define SEX_FEMALE    2

/* positions */
#define POSITION_DEAD       0
#define POSITION_MORTALLYW  1
#define POSITION_INCAP      2
#define POSITION_STUNNED    3
#define POSITION_SLEEPING   4
#define POSITION_RESTING    5
#define POSITION_SITTING    6
#define POSITION_FIGHTING   7
#define POSITION_STANDING   8

/* for mobile actions: specials.act */
#define ACT_SPEC         1     /* special routine to be called if exist   */
#define ACT_SENTINEL     2     /* this mobile not to be moved             */
#define ACT_SCAVENGER    4     /* pick up stuff lying around              */
#define ACT_ISNPC        8     /* This bit is set for use with IS_NPC()   */
#define ACT_NICE_THIEF  16     /* Set if a thief should NOT be killed     */
#define ACT_AGGRESSIVE  32     /* Set if automatic attack on NPC's        */
#define ACT_STAY_ZONE   64     /* MOB Must stay inside its own zone       */
#define ACT_WIMPY      128     /* MOB Will flee when injured, and if      */
                               /* aggressive only attack sleeping players */
#define ACT_HELPER     256     /* will come to the aid of other mobs */
#define ACT_HUNTER     512     /* will remember enemies */
#define ACT_SMART     1024     /* will pick on weakest opponent */
#define ACT_HEALER    2048     /* will heal self from poison and blindness */
#define ACT_WHATEVER  4096     /* can be ridden */
#define ACT_CHANGES   8192     /* changes special */
#define ACT_SEMIAGGR 16384     /* attack only higher level players */
#define ACT_SWIMMER  32768
#define ACT_CASTER   65536
#define ACT_KICKER  131072

/* For players : specials.act */

#define PLR_BRIEF         1
#define PLR_NOSHOUT       2
#define PLR_COMPACT       4
#define PLR_DONTSET       8   /* Dont EVER set */
#define PLR_NOTELL       16
#define PLR_BANISHED     32
#define PLR_CRIMINAL     64
#define PLR_WIZINVIS    128
#define PLR_EARMUFFS    256
#define PLR_NOSUMMON    512
#define PLR_TITLE      1024
#define PLR_VERYBRIEF  2048   /* No fight messages */
#define PLR_ECHO       4096
#define PLR_NORELO     8192
#define PLR_BUILDER   16384
#define PLR_AUTOCNVRT 32768

struct time_info_data {
  byte hours, day, month;
  sh_int year;
};

struct time_data {
  time_t birth;
  time_t logon;
  int played;
};

struct char_player_data {
  short level;
  char *name;
  char *short_descr;
  char *long_descr;
  char *description;
  char *title;
  struct time_data time;  /* PCs AGE in days */
  byte xyzzy;
  byte sex;
  ubyte weight;
  ubyte height;
};


/* Used in CHAR_FILE_U *DO*NOT*CHANGE* */
struct char_ability_data {
  short str; 
  short intel;
  short wis; 
  short dex; 
  short con; 
};

/* Used in CHAR_FILE_U *DO*NOT*CHANGE* */
struct char_point_data {
  int mana;
  int max_mana;
  int hit;
  int max_hit;
  int move;
  int max_move;
  int armor;
  int gold;
  int exp;
  int hitroll;
  int damroll;
  int kills;
  int deaths;
};

struct char_special_data {
  struct char_data *fighting;
  long lastback;
  long affected_by;
  byte position;
  byte default_pos;
  byte it;
  unsigned act;
  short spells_to_learn;    /* How many can you learn yet this level   */
  int carry_weight;
  byte carry_items;
  int timer;               /* Timer for update                        */
  sh_int was_in_room;      /* storage of location for linkdead people */
  sh_int apply_saving_throw[5];
  sh_int conditions[3];      /* Drunk full etc.                        */
  byte last_direction;      /* The last direction the monster went    */
  long damnodice;           /* The number of damage dice's            */
  long damsizedice;         /* The size of the damage dice's          */
  int attack_type;          /* The Attack Type Bitvector for NPC's    */
  int alignment;
  int magres, recall_room, xxx;
};

/* Used in CHAR_FILE_U *DO*NOT*CHANGE* */
struct char_skill_data {
  byte learned;           /* % chance for success 0 = not learned   */
  bool recognise;         /* If you can recognise the scroll etc.   */
};

/* Used in CHAR_FILE_U *DO*NOT*CHANGE* */
struct affected_type {
  sbyte type;           /* The type of spell that caused this      */
  sh_int duration;      /* For how long its effects will last      */
  sbyte modifier;       /* This is added to apropriate ability     */
  byte location;        /* Tells which ability to change(APPLY_XXX)*/
  long bitvector;       /* Tells which bits to set (AFF_XXX)       */
  struct affected_type *next;
};

struct follow_type {
  struct char_data *follower;
  struct follow_type *next;
};

struct char_file_u {
  short level;
  byte sex;
  time_t birth;
  int played;
  ubyte weight,height;
  char title[80];
  char description[236];
  int metapts;
  sh_int load_room;
  struct char_ability_data abilities;
  struct char_point_data points;
  struct char_skill_data skills[MAX_SKILLS];
  struct affected_type affected[MAX_AFFECT];
  short spells_to_learn;  
  int alignment;     
  time_t last_logon;
  unsigned act;
  int bank,kills,deaths;
  char name[20];
  char pwd[11];
  int conditions[3];
};

void del(char *filename)
{
   FILE *fl,*fo;
   struct char_file_u p;
   int i,pos,num,now,t,end,sum;

   t=time(0);
   if (!(fl = fopen(filename, "r"))) {
      perror("list");
      exit();
   }
   for (num = 1, pos = 0;; pos++, num++) {
      fread(&p, sizeof(p), 1, fl);
      if (feof(fl))
         exit(0);
      printf("%d %d %d %d %d %d Smith %d %d\n",
        p.level,p.sex,p.birth,p.played,
        p.weight,p.height,p.metapts,p.load_room);
      printf("%d %d %d %d %d\n",
        p.abilities.str,
        p.abilities.intel,
        p.abilities.wis,
        p.abilities.dex,
        p.abilities.con); 
      printf("%d %d %d %d %d %d %d %d %d %d %d %d %d\n",
        p.points.mana,
        p.points.max_mana,
        p.points.hit,
        p.points.max_hit,
        p.points.move,
        p.points.max_move,
        p.points.armor,
        p.points.gold,
        p.points.exp,
        p.points.hitroll,
        p.points.damroll,
        p.points.kills,
        p.points.deaths);
      for(i=0;i<MAX_SKILLS;i++)
        printf(" %d",p.skills[i].learned);
      printf("\n");
      for(i=0;i<MAX_AFFECT;i++)
        printf("%d %d %d %d %d\n",
          p.affected[i].type,
          p.affected[i].duration,
          p.affected[i].modifier,
          p.affected[i].location,
          p.affected[i].bitvector);
      printf("%d %d %d %d %d %d %d\n",
        p.spells_to_learn,
        p.alignment,
        p.last_logon,
        p.act,
        p.bank,
        p.kills,
        p.deaths);
      printf("%s %s %d %d %d\n",
       p.name,p.pwd,p.conditions[0],p.conditions[1],p.conditions[2]);
   }
   fclose(fl);
}
main(int argc, char **argv)
{
   del("/ge/dm/lib/players");
}
