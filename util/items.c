#define ITEM_LIGHT      1
#define ITEM_SCROLL     2
#define ITEM_WAND       3
#define ITEM_STAFF      4
#define ITEM_WEAPON     5
#define ITEM_FIREWEAPON 6
#define ITEM_MISSILE    7
#define ITEM_TREASURE   8
#define ITEM_ARMOR      9
#define ITEM_POTION    10 
#define ITEM_WORN      11
#define ITEM_OTHER     12
#define ITEM_TRASH     13
#define ITEM_TRAP      14
#define ITEM_CONTAINER 15
#define ITEM_NOTE      16
#define ITEM_DRINKCON  17
#define ITEM_KEY       18
#define ITEM_FOOD      19
#define ITEM_MONEY     20
#define ITEM_PEN       21
#define ITEM_BOAT      22
#define ITEM_PILL      23
#define ITEM_RADIO     24
#define ITEM_CHARGE    25
#define ITEM_BOMB      26
#define ITEM_SHOVEL    27
#define ITEM_DISPENSER 28

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
#define ITEM_WEAR_FACE     131072
#define ITEM_WEAR_EARS     262144

#define ITEM_GLOW            1
#define ITEM_HUM             2
#define ITEM_SFX             4
#define ITEM_LOCK            8
#define ITEM_EVIL           16
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

#define OBJ_NOTIMER    -7000000

struct item {
  char *name;
  unsigned char pair, type;
} item_list[]={
 { "ring", 0, 9},
 { "chain", 0, 9},
 { "suit", 0, 9},
 { "toupee", 0, 9},
 { "trousers", 1, 9},
 { "socks", 1, 9},
 { "gloves", 1, 9},
 { "sleeves", 1, 9},
 { "laptop", 0, 9},
 { "trenchcoat", 0, 9},
 { "belt", 0, 9},
 { "watch", 0, 9},
 { "bat", 0, 5},
 { "briefcase", 0, 9},
 { "cellular phone", 0, 9},
 { "flashlight", 0, 1},
 { "glasses", 0, 9},
 { "earmuffs", 1, 9}
};

#define VOWEL(X) (((X)=='a')||((X)=='e')||((X)=='i')||((X)=='o')||((X)=='u'))

#define BASE 17410
#define RNDARMOR (5+(random()%20))
#define NDESCR 9
char *descr[]={
  "diamond",
  "gold",
  "polyester",
  "blonde",
  "polyester",
  "nylon",
  "calf-skin",
  "gheppish",
  "sickening yellow"
};
main()
{
  int i,j,k;
  char *s;
  unsigned char anf;

  srandom(getpid());
  for(i=ITEM_WEAR_FINGER,j=0;i<=ITEM_WEAR_EARS;i<<=1,j++){
    printf("#%d\n",BASE+j);
    s=item_list[j].name;
    k=(random()%NDESCR);
    anf=(!item_list[j].pair)&&(VOWEL(*descr[k]));
    printf("%s~\n",s);
    printf("%s%s%s %s~\n",
      anf ? "An" : "A",
      item_list[j].pair ? " pair of " : " ",
      descr[k],
      s);
    printf("%s%s%s %s is in the dirt.~\n",
      anf ? "An" : "A",
      item_list[j].pair ? " pair of " : " ",
      descr[k],
      s);
    printf("~\n%d %d %d\n",item_list[j].type,rndflags(),i+1);
    printf("%d 0 0 0\n",RNDARMOR);
    printf("%d %d %d\n",10,1000,100);
  }
}
rndflags()
{
}
