#include "structs.h"
#include "limits.h"

const char *spell_wear_off_msg[] = {
  "RESERVED DB.C",
  "You feel less protected.",
  "Things are not so clear now.",
  "You feel less righteous.",
  "You feel a cloak of blindness dissolve.",
  "!Burning Hands!",
  "!Call Lightning",
  "You feel more self-confident.",
  "!Chill Touch!",
  "!Reanimate!",
  "!Color Spray!",
  "!Relocate!",
  "!Create Food!",
  "!Create Water!",
  "!Cure Blind!",
  "!Cure Critic!",
  "!Cure Light!",
  "You feel better.",
  "!Jingle!",
  "The detect invisible wears off.",
  "!Kerplunk!",
  "!Drain MR!",
  "!Dispel Evil!",
  "!Earthquake!",
  "!Enchant Weapon!",
  "!Energy Drain!",
  "!Fireball!",
  "!Harm!",
  "!Heal",
  "You feel yourself exposed.",
  "!Lightning Bolt!",
  "!Doppelganger!",
  "!Magic Missile!",
  "You feel less sick.",
  "You feel less protected.",
  "!Remove Curse!",
  "The white aura around your body fades.",
  "!Shocking Grasp!",
  "You feel less tired.",
  "You feel weaker.",
  "!Summon!",
  "You feel less heroic.",
  "!Word of Recall!",
  "!Remove Poison!",
  "You feel less aware of your surroundings.",
  "!Sunburst!",
  "!UNUSED!",
  "Your eyes feel less sensitive.",
  "You feel a little guilty.",
  "You settle down.",
  "You feel more myopic.",
  "!UNUSED!",
  "!UNUSED!",
  "!Identify!",
  "!UNUSED!",
  "!NOVA!",  /* NO MESSAGE FOR SNEAK*/
  "!Hide!",
  "!Steal!",
  "!Backstab!",
  "!Pick Lock!",
  "You slow down.",
  "You stop vibrating.",
  "!Rescue!",
  "You don't feel quite so stupid now.",
  "\n"
};


const int rev_dir[] = 
{
  2, 3, 0, 1, 5, 4
}; 

const int movement_loss[]=
{
  1,  /* Inside     */
  2,  /* City       */
  2,  /* Field      */
  3,  /* Forest     */
  4,  /* Hills      */
  6,  /* Mountains  */
  4,  /* Swimming   */
  1   /* Unswimable */
};

const char *dirs[] = 
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "\n"
};

const char *dirletters="neswud";

const int revdir[]={2,3,0,1,5,4};

const char *weekdays[7] = { 
  "the Day of the Moon",
  "the Day of the Bull",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the day of the Great Gods",
  "the Day of the Sun" };

const char *month_name[17] = {
  "Month of Winter",           /* 0 */
  "Month of the Winter Wolf",
  "Month of the Frost Giant",
  "Month of the Great Mel",
  "Month of the Grand Struggle",
  "Month of the Spring",
  "Month of Nature",
  "Month of Futility",
  "Month of the Dragon",
  "Month of the Sun",
  "Month of the Heat",
  "Month of the Battle",
  "Month of the Dark Shades",
  "Month of the Shadows",
  "Month of the Long Shadows",
  "Month of the Ancient Darkness",
  "Month of the Great Evil"
};

const int sharp[] = {
   0,
   0,
   0,
   1,    /* Slashing */
   0,
   0,
   0,
   0,    /* Bludgeon */
   0,
   0,
   0,
   0 };  /* Pierce   */

const char *where[] = {
  "light",
  "finger",
  "finger",
  "neck",
  "neck",
  "body",
  "head",
  "legs",
  "feet",
  "hands",
  "arms",
  "shield",
  "about",
  "waist",
  "wrist",
  "wrist",
  "weapon",
  "held",
  "radio",
  "face",
  "ears",
  "ankles"
}; 

const char *drinks[]=
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whiskey",
  "lemonade",
  "Johnnie Walker",
  "milkshake",
  "Courvoisier",
  "milk",
  "tea",
  "coffee",
  "blood",
  "Diet Coke",
  "Coca Cola",
  "Crownguard Cola",
  "the potion of the druids",
  "Royal Kona coffee",
  "piss"
};

const char *drinknames[]=
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "scotch",
  "shake",
  "cognac",
  "milk",
  "tea",
  "coffee",
  "blood",
  "coke",
  "coke",
  "cola",
  "potion",
  "coffee",
  "piss"
};

const int drink_aff[][3] = {
  { 0,1,10 },  /* Water    */
  { 2,2,5 },   /* beer     */
  { 1,3,4 },   /* wine     */
  { 3,2,5 },   /* ale      */
  { 3,2,5 },   /* ale      */
  { 6,1,4 },   /* Whiskey  */
  { 0,2,8 },   /* lemonade */
  { 7,0,4 },  /* scotch   */
  { 0,9,9 },  /* shake    */
  { 7,1,4 },  /* cognac   */
  { 0,4,6 },
  { 0,1,6 },
  {-1,1,3 },
  { 0,2,-1},
  { 0,3,9 },
  { 0,6,9 },
  { 0,6,9 },
  {-9,0,0 },
  {-2,2,3 },
  {-1,1,-1}
};

const char *color_liquid[]=
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "amber",
  "clear",
  "golden",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "black",
  "muddy",
  "strange",
  "golden brown",
  "piss"
};

const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};

const char *item_types[] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "FIRE WEAPON",
  "MISSILE",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "TRAP",
  "CONTAINER",
  "NOTE",
  "LIQUID CONTAINER",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "BOAT",
  "PILL",
  "RADIO",
  "CHARGE",
  "BOMB",
  "SHOVEL",
  "DISPENSER",
  "TOKEN",
  "\n"
};

const char *wear_bits[] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAISTE",
  "WRIST",
  "WIELD",
  "HOLD",
  "RADIO",
  "LIGHT-SOURCE",
  "WEAR-FACE",
  "WEAR-EARS",
  "RENTED",
  "WEAR-ANKLES",
  "\n"
};

const char *extra_bits[] = {
  "GLOW",
  "HUM",
  "SPECIAL-FX",
  "LOCK",
  "CHEAP",
  "INVISIBLE",
  "MAGIC",
  "NODROP",
  "BLESS",
  "ANTI-GOOD",
  "ANTI-EVIL",
  "ANTI-NEUTRAL",
  "PRIME",
  "POOF",
  "POOFSOON",
  "VARIABLE",
  "ANTIPRIME",
  "NO-KICK",
  "MUTABLE",
  "NO-PUT-IN",
  "\n"
};

const char *room_bits[] = {
  "DARK",
  "NOSUMMON",
  "NO_MOB",
  "INDOORS",
  "LAWFUL",
  "NEUTRAL",
  "TRICKY",
  "NO_MAGIC",
  "CONFUSING",
  "PRIVATE",
  "OFF_LIMITS",
  "RENT",
  "NORELOCATE",
  "NO_DIG",
  "MOD",
  "PK",
  "UNOWNED",
  "\n"
};

const char *exit_bits[] = {
  "IS-DOOR",
  "CLOSED",
  "LOCKED",
  "\n"
};

const char *sector_types[] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Water Swim",
  "Water NoSwim",
  "\n"
};

const char *equipment_types[] = {
  "Special",
  "Worn on right finger",
  "Worn on left finger",
  "First worn around Neck",
  "Second worn around Neck",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn as shield",
  "Worn about body",
  "Worn around waiste",
  "Worn around right wrist",
  "Worn around left wrist",
  "Wielded",
  "Held",
  "Radio",
  "Worn on face",
  "\n"
};
  
const char *affected_bits[] = 
{ "BLIND",
  "INVISIBLE",
  "DETECT-EVIL",
  "DETECT-INVISIBLE",
  "DETECT-MAGIC",
  "SENSE-LIFE",
  "HOLD",
  "SANCTUARY",
  "GROUP",
  "UNUSED",
  "CURSE",
  "REGENERATION",
  "POISON",
  "PROTECT-EVIL",
  "CLARITY",
  "INFRA",
  "FARSEE",
  "SLEEP",
  "FEARLESS",
  "SNEAK",
  "HIDE",
  "FEAR",
  "CHARM",
  "FOLLOW",
  "HASTE",
  "HOLD-ALIGN",
  "HYPER-REGEN",
  "STUPIDITY",
  "NOSUMMON",
  "\n"
};

const char *apply_types[] = {
  "NONE",
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",
  "SEX",
  "",
  "LEVEL",
  "AGE",
  "KICK",
  "SPELL_LEVEL",
  "MANA",
  "HIT",
  "MOVE",
  "GOLD",
  "EXP",
  "ARMOR",
  "HITROLL",
  "DAMROLL",
  "SAVING_PARA",
  "SAVING_ROD",
  "SAVING_SUNBURST",
  "SAVING_BREATH",
  "SAVING_SPELL",
  "MAGIC_RESISTANCE",
  "META_BONUS",
  "GUT",
  "\n"
};

const char *action_bits[] = {
  "SPECIAL",
  "SENTINEL",
  "SCAVENGER",
  "ISNPC",
  "NICE-THIEF",
  "AGGRESSIVE",
  "STAY-ZONE",
  "WIMPY",
  "HELPER",
  "HUNTER",
  "SMART",
  "HEALER",
  "WHATEVER",
  "CHANGER",
  "SEMI-AGGR",
  "SWIMMER",
  "CASTER",
  "KICKER",
  "CLONE",
  "TRIBBLE",
  "DUMPER",
  "GUARD",
  "\n"
};


const char *player_bits[] = {
  "BRIEF",
  "NOSHOUT",
  "COMPACT",
  "DONTSET",
  "NOTELL",
  "BANISHED",
  "CRIMINAL",
  "WIZINVIS",
  "EARMUFFS",
  "NOSUMMON",
  "TITLES",
  "VERYBRIEF",
  "ECHO",
  "NORELO",
  "BUILDER",
  "AUTOCONVERT",
  "AUTOEXIT",
  "AUTOLOOT",
  "REPEAT",
  "\n"
};


const char *position_types[] = {
  "Dead",
  "Mortally wounded",
  "Incapacitated",
  "Stunned",
  "Sleeping",
  "Resting",
  "Sitting",
  "Fighting",
  "Standing",
  "\n"
};

const char *connected_types[]  =  {
  "Playing",
  "Get name",
  "Confirm name",
  "Read Password",
  "Get new password",
  "Confirm new password",
  "Get sex",
  "Read messages of today",
  "Read Menu",
  "Get extra description",
  "UNUSED",
  "\n"
};

char *shield_fx_name[]={
  "nothing",
  "radiation",
  "nova",
  "slime",
  "nothing",
  "gas",
  "cyclone",
  "bombs",
  "harm",
  0
};
char *eyewear_fx_name[]={
  "nothing",
  "farsight",
  "infra",
  "invisible",
  "sense life",
  "blindness protection",
  "infra and sense life"
};
