/* ************************************************************************
*  file: spells.h , Implementation of magic spells.       Part of DIKUMUD *
*  Usage : Spells                                                         *
************************************************************************* */

#define MAX_WEAPON_SPELL             14

#define MAX_BUF_LENGTH              240

#define TYPE_UNDEFINED               -1
#define SPELL_RESERVED_DBC            0  /* SKILL NUMBER ZERO */
#define SPELL_ARMOR                   1
#define SPELL_CLARITY                 2
#define SPELL_BLESS                   3
#define SPELL_BLINDNESS               4
#define SPELL_BURNING_HANDS           5
#define SPELL_MORPHIA                 6
#define SPELL_CHARM_PERSON            7
#define SPELL_CHILL_TOUCH             8
#define SPELL_REANIMATE               9
#define SPELL_COLOUR_SPRAY           10 
#define SPELL_RELOCATE               11 
#define SPELL_CREATE_FOOD            12 
#define SPELL_CREATE_WATER           13 
#define SPELL_CURE_BLIND             14 
#define SPELL_CURE_CRITIC            15 
#define SPELL_CURE_LIGHT             16 
#define SPELL_CURSE                  17 
#define SPELL_JINGLE                 18 
#define SPELL_DETECT_INVISIBLE       19 
#define SPELL_SCAN                   20 
#define SPELL_DRAIN_MR               21 
#define SPELL_DISPEL_EVIL            22 
#define SPELL_EARTHQUAKE             23 
#define SPELL_ENCHANT_WEAPON         24 
#define SPELL_ENERGY_DRAIN           25 
#define SPELL_FIREBALL               26 
#define SPELL_HARM                   27 
#define SPELL_HEAL                   28 
#define SPELL_INVISIBLE              29 
#define SPELL_LIGHTNING_BOLT         30 
#define SPELL_DOPPELGANGER           31 
#define SPELL_MAGIC_MISSILE          32 
#define SPELL_POISON                 33 
#define SPELL_PROTECT_FROM_EVIL      34 
#define SPELL_REMOVE_CURSE           35 
#define SPELL_SANCTUARY              36 
#define SPELL_SHOCKING_GRASP         37 
#define SPELL_SLEEP                  38 
#define SPELL_STRENGTH               39 
#define SPELL_SUMMON                 40 
#define SPELL_FEARLESSNESS           41 
#define SPELL_WORD_OF_RECALL         42
#define SPELL_REMOVE_POISON          43
#define SPELL_SENSE_LIFE             44
#define SPELL_SUNBURST               45
#define SPELL_CLONE                  46
#define SPELL_INFRAVISION            47
#define SPELL_HOLDALIGN              48
#define SPELL_REGEN                  49
#define SPELL_FARSEE                 50
#define SPELL_GROUPHEAL              51
#define SPELL_MOGRIFICATION          52
#define SPELL_BURLYHEAL              53
#define SPELL_RADIATION              54
#define SPELL_NOVA                   55
#define SPELL_FORGETFULNESS          56
#define SPELL_EXTRAHEAL              57
#define SPELL_BARF                   58
#define SPELL_INVIGORATE             59
#define SPELL_HASTE                  60
#define SPELL_HYPERREGEN             61
#define SPELL_SLIME                  62
#define SPELL_STUPIDITY              63
#define SPELL_DISPEL_SPELL           64
#define SPELL_EXTREMELYBURLY         65
#define SPELL_OESOPHAGOSTENOSIS      66
#define SPELL_TRANSMUTATION          67
#define SPELL_GAS                    68
#define SPELL_CYCLONE                69
#define MAXSPELL                     70

#define SKILL_ATTACK                 70
#define SKILL_SNEAK                  71
#define SKILL_HIDE                   72
#define SKILL_STEAL                  73
#define SKILL_BACKSTAB               74
#define SKILL_PICK_LOCK              75
#define SKILL_KICK                   76
#define SKILL_BASH                   77
#define SKILL_RESCUE                 78
#define SKILL_GR_RESCUE              79
#define SKILL_SHOOT                  80
#define SKILL_BOMB                   81

#define SPELL_MANAHEAL               83
#define SPELL_MOVEHEAL               84

#define SPELL_FIRE_BREATH            85
#define SPELL_GAS_BREATH             86
#define SPELL_FROST_BREATH           87
#define SPELL_ACID_BREATH            88
#define SPELL_LIGHTNING_BREATH       89

#define SPELL_MAGIC_MISSILE_WPN      90
#define SPELL_COLOUR_SPRAY_WPN       91
#define SPELL_FIREBALL_WPN           92
#define SPELL_DISPEL_EVIL_WPN        93
#define SPELL_BOLT_WPN               94
#define SPELL_SUNBURST_WPN           95
#define SPELL_SLIME_WPN              96
#define SPELL_GAS_WPN                97

#define TYPE_HIT                     100
#define TYPE_BLUDGEON                101
#define TYPE_PIERCE                  102
#define TYPE_SLASH                   103
#define TYPE_WHIP                    104
#define TYPE_BLAST                   105
#define TYPE_SPLAT                   106
#define TYPE_SPANK                   107
#define TYPE_CRUSH                   108
#define TYPE_BURN                    109
#define TYPE_SHOOT                   110

#define TYPE_SUFFERING               200

/* More anything but spells and weapontypes can be inserted here! */

#define MAX_TYPES 70

#define SAVING_PARA      0
#define SAVING_ROD       1
#define SAVING_SUNBURST  2
#define SAVING_BREATH    3
#define SAVING_SPELL     4


#define MAX_SPL_LIST  127


#define TAR_IGNORE        1
#define TAR_CHAR_ROOM     2
#define TAR_CHAR_WORLD    4
#define TAR_FIGHT_SELF    8
#define TAR_FIGHT_VICT   16
#define TAR_SELF_ONLY    32 /* Only a check, use with ei. TAR_CHAR_ROOM */
#define TAR_SELF_NONO    64 /* Only a check, use with ei. TAR_CHAR_ROOM */
#define TAR_OBJ_INV     128
#define TAR_OBJ_ROOM    256
#define TAR_OBJ_WORLD   512
#define TAR_OBJ_EQUIP  1024

struct spell_info_type
{
  void (*spell_pointer) (short level, struct char_data *ch, char *arg, int type,
                         struct char_data *tar_ch, struct obj_data *tar_obj);
  byte minimum_position;  /* Position for caster              */
  short min_usesmana;     /* Amount of mana used by a spell   */
  short beats;             /* Heartbeats until ready for next */
  short max_skill;
  short min_level;
  short targets;         /* See below for use with TAR_XXX  */
};

/* Possible Targets:

   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self.

*/

#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4

#define SHIELD_RAD      1
#define SHIELD_NOVA     2
#define SHIELD_SLIME    3
#define SHIELD_DRAIN    4
#define SHIELD_GAS      5
#define SHIELD_CYCLONE  6
#define SHIELD_BOMB     7
#define SHIELD_HARM     8

/* Attacktypes with grammar */

struct attack_hit_type {
  char *singular;
  char *plural;
};

struct weapon_spell_list {
  int lo,hi;
  int lev,shift;
  char *name;
  void (*spellfun)(short l, struct char_data *ch,
    struct char_data *ta, struct obj_data *ob);
};
