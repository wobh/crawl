#include "AppHdr.h"

#include "dlua.h"
#include "l_libs.h"

#include "abl-show.h"
#include "chardump.h"
#include "delay.h"
#include "food.h"
#include "initfile.h"
#include "los.h"
#include "mon-util.h"
#include "newgame.h"
#include "player.h"
#include "religion.h"
#include "skills2.h"
#include "spells3.h"
#include "spl-util.h"
#include "transfor.h"

/////////////////////////////////////////////////////////////////////
// Bindings to get information on the player (clua).
//

static const char *transform_name()
{
    switch (you.attribute[ATTR_TRANSFORMATION])
    {
    case TRAN_SPIDER:
        return "spider";
    case TRAN_BAT:
        return "bat";
    case TRAN_BLADE_HANDS:
        return "blade";
    case TRAN_STATUE:
        return "statue";
    case TRAN_ICE_BEAST:
        return "ice";
    case TRAN_DRAGON:
        return "dragon";
    case TRAN_LICH:
        return "lich";
    case TRAN_PIG:
        return "pig";
    default:
        return "";
    }
}

LUARET1(you_turn_is_over, boolean, you.turn_is_over)
LUARET1(you_name, string, you.your_name)
LUARET1(you_race, string,
        species_name(you.species, you.experience_level).c_str())
LUARET1(you_class, string, get_class_name(you.char_class))
LUARET1(you_god, string, god_name(you.religion).c_str())
LUARET1(you_good_god, boolean,
        lua_isstring(ls, 1) ? is_good_god(str_to_god(lua_tostring(ls, 1)))
        : is_good_god(you.religion))
LUARET1(you_evil_god, boolean,
        lua_isstring(ls, 1) ? is_evil_god(str_to_god(lua_tostring(ls, 1)))
        : is_evil_god(you.religion))
LUARET1(you_god_likes_fresh_corpses, boolean,
        lua_isstring(ls, 1) ?
        god_likes_fresh_corpses(str_to_god(lua_tostring(ls, 1))) :
        god_likes_fresh_corpses(you.religion))
LUARET1(you_god_likes_butchery, boolean,
        lua_isstring(ls, 1) ?
        god_likes_butchery(str_to_god(lua_tostring(ls, 1))) :
        god_likes_butchery(you.religion))
LUARET2(you_hp, number, you.hp, you.hp_max)
LUARET2(you_mp, number, you.magic_points, you.max_magic_points)
LUARET1(you_hunger, string, hunger_level())
LUARET2(you_strength, number, you.strength, you.max_strength)
LUARET2(you_intelligence, number, you.intel, you.max_intel)
LUARET2(you_dexterity, number, you.dex, you.max_dex)
LUARET1(you_exp, number, you.experience_level)
LUARET1(you_exp_points, number, you.experience)
LUARET1(you_skill, number,
        lua_isstring(ls, 1) ? you.skills[str_to_skill(lua_tostring(ls, 1))]
                            : 0)
LUARET1(you_res_poison, number, player_res_poison(false))
LUARET1(you_res_fire, number, player_res_fire(false))
LUARET1(you_res_cold, number, player_res_cold(false))
LUARET1(you_res_draining, number, player_prot_life(false))
LUARET1(you_res_shock, number, player_res_electricity(false))
LUARET1(you_res_statdrain, number, player_sust_abil(false))
LUARET1(you_res_mutation, number, wearing_amulet(AMU_RESIST_MUTATION, false))
LUARET1(you_res_slowing, number, wearing_amulet(AMU_RESIST_SLOW, false))
LUARET1(you_gourmand, boolean, wearing_amulet(AMU_THE_GOURMAND, false))
LUARET1(you_saprovorous, number, player_mutation_level(MUT_SAPROVOROUS))
LUARET1(you_levitating, boolean, you.flight_mode() == FL_LEVITATE)
LUARET1(you_flying, boolean, you.flight_mode() == FL_FLY)
LUARET1(you_transform, string, transform_name())
LUARET1(you_where, string, level_id::current().describe().c_str())
LUARET1(you_branch, string, level_id::current().describe(false, false).c_str())
LUARET1(you_subdepth, number, level_id::current().depth)
// Increase by 1 because check happens on old level.
LUARET1(you_absdepth, number, you.your_level + 1)
LUAWRAP(you_stop_activity, interrupt_activity(AI_FORCE_INTERRUPT))
LUARET1(you_taking_stairs, boolean,
        current_delay_action() == DELAY_ASCENDING_STAIRS
        || current_delay_action() == DELAY_DESCENDING_STAIRS)
LUARET1(you_turns, number, you.num_turns)
LUARET1(you_can_smell, boolean, player_can_smell())
LUARET1(you_has_claws, number, you.has_claws(false))

static int _you_uniques(lua_State *ls)
{
    ASSERT_DLUA;

    bool unique_found = false;

    if (lua_gettop(ls) >= 1 && lua_isstring(ls, 1))
        unique_found = you.unique_creatures[get_monster_by_name(lua_tostring(ls, 1))];

    lua_pushboolean(ls, unique_found);
    return (1);
}

static int _you_gold(lua_State *ls)
{
    if (lua_gettop(ls) >= 1)
    {
        ASSERT_DLUA;
        const int new_gold = luaL_checkint(ls, 1);
        const int old_gold = you.gold;
        you.gold = std::max(new_gold, 0);
        if (new_gold > old_gold)
            you.attribute[ATTR_GOLD_FOUND] += new_gold - old_gold;
        else if (old_gold > new_gold)
            you.attribute[ATTR_MISC_SPENDING] += old_gold - new_gold;
    }
    PLUARET(number, you.gold);
}

void lua_push_floor_items(lua_State *ls);
static int you_floor_items(lua_State *ls)
{
    lua_push_floor_items(ls);
    return (1);
}

static int l_you_spells(lua_State *ls)
{
    lua_newtable(ls);
    int index = 0;
    for (int i = 0; i < 52; ++i)
    {
        const spell_type spell = get_spell_by_letter( index_to_letter(i) );
        if (spell == SPELL_NO_SPELL)
            continue;

        lua_pushstring(ls, spell_title(spell));
        lua_rawseti(ls, -2, ++index);
    }
    return (1);
}

static int l_you_abils(lua_State *ls)
{
    lua_newtable(ls);

    std::vector<const char *>abils = get_ability_names();
    for (int i = 0, size = abils.size(); i < size; ++i)
    {
        lua_pushstring(ls, abils[i]);
        lua_rawseti(ls, -2, i + 1);
    }
    return (1);
}

static int you_can_consume_corpses(lua_State *ls)
{
    lua_pushboolean(ls,
                    can_ingest(OBJ_FOOD, FOOD_CHUNK, true, false, false)
                    || can_ingest(OBJ_CORPSES, CORPSE_BODY, true, false, false)
                    );
    return (1);
}

static const struct luaL_reg you_clib[] =
{
    { "turn_is_over", you_turn_is_over },
    { "turns"       , you_turns },
    { "spells"      , l_you_spells },
    { "abilities"   , l_you_abils },
    { "name"        , you_name },
    { "race"        , you_race },
    { "class"       , you_class },
    { "god"         , you_god },
    { "gold"        , _you_gold },
    { "good_god"    , you_good_god },
    { "evil_god"    , you_evil_god },
    { "hp"          , you_hp },
    { "mp"          , you_mp },
    { "hunger"      , you_hunger },
    { "strength"    , you_strength },
    { "intelligence", you_intelligence },
    { "dexterity"   , you_dexterity },
    { "skill"       , you_skill },
    { "uniques"     , _you_uniques },
    { "xl"          , you_exp },
    { "exp"         , you_exp_points },
    { "res_poison"  , you_res_poison },
    { "res_fire"    , you_res_fire   },
    { "res_cold"    , you_res_cold   },
    { "res_draining", you_res_draining },
    { "res_shock"   , you_res_shock },
    { "res_statdrain", you_res_statdrain },
    { "res_mutation", you_res_mutation },
    { "res_slowing",  you_res_slowing },
    { "saprovorous",  you_saprovorous },
    { "gourmand",     you_gourmand },
    { "levitating",   you_levitating },
    { "flying",       you_flying },
    { "transform",    you_transform },

    { "god_likes_fresh_corpses",  you_god_likes_fresh_corpses },
    { "god_likes_butchery",       you_god_likes_butchery },
    { "can_consume_corpses",      you_can_consume_corpses },

    { "stop_activity", you_stop_activity },
    { "taking_stairs", you_taking_stairs },

    { "floor_items",  you_floor_items },

    { "where",        you_where },
    { "branch",       you_branch },
    { "subdepth",     you_subdepth },
    { "absdepth",     you_absdepth },

    { "can_smell",         you_can_smell },
    { "has_claws",         you_has_claws },

    { NULL, NULL },
};

void cluaopen_you(lua_State *ls)
{
    luaL_openlib(ls, "you", you_clib, 0);
}

/////////////////////////////////////////////////////////////////////
// Player information (dlua). Grid coordinates etc.
//

LUARET1(you_can_hear_pos, boolean,
        player_can_hear(coord_def(luaL_checkint(ls,1), luaL_checkint(ls, 2))))
LUARET1(you_x_pos, number, you.pos().x)
LUARET1(you_y_pos, number, you.pos().y)
LUARET2(you_pos, number, you.pos().x, you.pos().y)

LUARET1(you_see_cell, boolean,
        see_cell(luaL_checkint(ls, 1), luaL_checkint(ls, 2)))
LUARET1(you_see_cell_no_trans, boolean,
        see_cell_no_trans(luaL_checkint(ls, 1), luaL_checkint(ls, 2)))

LUAFN(you_moveto)
{
    const coord_def place(luaL_checkint(ls, 1), luaL_checkint(ls, 2));
    ASSERT(map_bounds(place));
    you.moveto(place);
    return (0);
}

LUAFN(you_random_teleport)
{
    you_teleport_now(false, false);
    return (0);
}

LUAFN(you_losight)
{
    calc_show_los();
    return (0);
}

static const struct luaL_reg you_dlib[] =
{
{ "hear_pos", you_can_hear_pos },
{ "x_pos", you_x_pos },
{ "y_pos", you_y_pos },
{ "pos",   you_pos },
{ "moveto", you_moveto },
{ "see_cell",          you_see_cell },
{ "see_cell_no_trans", you_see_cell_no_trans },
{ "random_teleport", you_random_teleport },
{ "losight", you_losight },
{ NULL, NULL }
};

void dluaopen_you(lua_State *ls)
{
    luaL_openlib(ls, "you", you_dlib, 0);
}
