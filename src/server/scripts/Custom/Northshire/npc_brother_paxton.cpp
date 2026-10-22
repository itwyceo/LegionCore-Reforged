/*
 * Northshire Valley -- Brother Paxton (951), the healer priest.
 *
 * The database already points this NPC at script name "npc_brother_paxton":
 *
 *   creature_template.ScriptName = 'npc_brother_paxton'   (entry 951)
 *   creature_template.AIName     = ''                     (empty)
 *
 * Upstream TrinityCore ships a file that answers to that name, LegionCore's
 * 7.3.5 port does not -- nothing registered the script, so Paxton stood there
 * with no AI at all and never healed anybody. This file is that missing half.
 *
 * What it does, in plain words: every few seconds he looks around for a nearby
 * wounded friend (a Stormwind Infantry below the health threshold, standing close
 * enough to be reached) and casts Heal on the person who needs it most. One
 * cast is not always enough, so the check simply comes back around 15-21s later
 * -- which is what turns it into a loop that keeps patching people back up.
 *
 * The healing radius is deliberately generous (see HEAL_SEARCH_RANGE).
 * The official Paxton spawns inside Northshire Abbey while the infantry fights
 * out in the valley floor, roughly 160 yards apart, so the SmartAI equivalent
 * (SMART_EVENT_FRIENDLY_HEALTH, whose search range is hard coded in the core
 * and far too small here) would never fire at all. A priest walking over to his
 * patients is also more in character than a priest standing still in a chapel.
 *
 * Related: 951_帕克斯顿修士_治疗_Northshire_heal.sql
 *          (the Stormwind Infantry / Blackrock Worg fight loop and the wounded
 *           infantry's call for help -- the pawns Paxton is here to help)
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Map.h"
#include "SpellInfo.h"
#include "Log.h"
#include "Player.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectVisitors.hpp"
#include "GridNotifiers.h"

#include <algorithm>
#include <list>
#include <vector>

enum BrotherPaxton
{
    NPC_BROTHER_PAXTON      = 951,

    SPELL_HEAL              = 11642,// Heal, rank 1 priest
    SPELL_PRAYER_OF_HEALING  = 93091,// area heal, used once several people are down

    YELL_INFANTRY_SOUND     = 0,

    // How often the wounded are looked for. Slightly irregular so a crowd of
    // infantry does not all get topped up on the same frame.
    HEAL_CHECK_MIN_MS       = 15000,
    HEAL_CHECK_MAX_MS       = 21000,

    // Above this many wounded patients at once, switch to the area heal.
    PRAYER_OF_HEALING_COUNT = 3
};

// Health threshold below which somebody counts as wounded, as a float.
//
// This lives outside the enum for the same reason HEAL_SEARCH_RANGE does, and
// for one more: Unit::GetHealthPct() returns float, and comparing a float to an
// enum is warning C5055 ("operator >= is deprecated between enumeration and
// floating-point type"). Two MSVC-specific traps in three lines of tunables,
// which is why every tunable in this file is a named constant.
static const float HEAL_MISSING_HEALTH_PCT = 70.0f;

// Sweep radius for wounded friendlies. Kept outside the enum on purpose: C++
// enums only accept integral constant expressions, a float literal here is
// MSVC error C2058 -- the exact trap TREE_SEARCH_RANGE in npc_northshire_farmer
// already fell into once.
//
// The official Paxton spawns inside Northshire Abbey while the infantry fights
// out in the valley floor, roughly 24-205 yards away depending on which
// infantry you pick, so this has to be large enough to actually reach the
// wounded. See README.txt for the two ways to cover the whole valley.
static const float HEAL_SEARCH_RANGE = 60.0f;

enum BrotherPaxtonEvents
{
    EVENT_CHECK_FOR_WOUNDED = 1
};

class npc_brother_paxton : public CreatureScript
{
public:
    npc_brother_paxton() : CreatureScript("npc_brother_paxton") { }

    struct npc_brother_paxtonAI : public ScriptedAI
    {
        npc_brother_paxtonAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_CHECK_FOR_WOUNDED,
                urand(HEAL_CHECK_MIN_MS, HEAL_CHECK_MAX_MS));
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CHECK_FOR_WOUNDED:
                        HealWoundedFriendly();
                        _events.ScheduleEvent(EVENT_CHECK_FOR_WOUNDED,
                            urand(HEAL_CHECK_MIN_MS, HEAL_CHECK_MAX_MS));
                        break;
                }
            }
        }

    private:
        // Collects nearby friendlies below HEAL_MISSING_HEALTH_PCT, worst hit
        // first. Paxton keeps his distance during combat, so there is no point
        // listing a patient he cannot reach.
        void GatherWounded(std::vector<Unit*>& wounded) const
        {
            std::list<Unit*> candidates;

            // Creatures first: the wounded infantry this valley is built around.
            std::list<Creature*> creatureList;
            me->GetCreatureListInGrid(creatureList, HEAL_SEARCH_RANGE);
            for (Creature* creature : creatureList)
                candidates.push_back(creature);

            // Then players: an adventurer at 20% health next to a priest is
            // still a patient. Map::GetPlayers() only has a const overload, so
            // the iterator is Map::PlayerList::const_iterator and the player
            // comes out of getSource(), not get().
            for (Map::PlayerList::const_iterator itr = me->GetMap()->GetPlayers().begin();
                 itr != me->GetMap()->GetPlayers().end(); ++itr)
            {
                if (Player* player = itr->getSource())
                {
                    if (player->IsInWorld() && player->IsWithinDistInMap(me, HEAL_SEARCH_RANGE))
                        candidates.push_back(player);
                }
            }

            for (Unit* unit : candidates)
            {
                if (!unit || unit == me || !unit->IsAlive())
                    continue;

                // Same faction only. IsFriendlyTo covers players, creatures and
                // totems in one test and refuses anything that is not in our
                // faction -- a neutral or hostile target is never a patient.
                //
                // Note there is deliberately no "skip units in combat" filter:
                // the whole point is the infantry being beaten by the worgs
                // mid-fight. Paxton heals them while the fight is still going
                // on, which is exactly what the wounded are asking for when
                // they yell.
                if (!me->IsFriendlyTo(unit))
                    continue;

                if (unit->GetHealthPct() >= HEAL_MISSING_HEALTH_PCT)
                    continue;

                wounded.push_back(unit);
            }

            std::sort(wounded.begin(), wounded.end(),
                [](Unit const* a, Unit const* b)
                {
                    return a->GetHealthPct() < b->GetHealthPct();
                });
        }

        void HealWoundedFriendly() const
        {
            if (!me->IsAlive())
                return;

            std::vector<Unit*> wounded;
            GatherWounded(wounded);

            if (wounded.empty())
                return;

            Unit* patient = wounded.front();

            // Face the person being helped, then heal. A priest who heals over
            // his own shoulder looks broken even when the numbers work.
            me->SetFacingToObject(patient);

            uint32 spellId = wounded.size() >= PRAYER_OF_HEALING_COUNT
                ? SPELL_PRAYER_OF_HEALING
                : SPELL_HEAL;

            me->CastSpell(patient, spellId, false);

            TC_LOG_ERROR("scripts", "[northshire_heal] guid=%u healed guid=%u "
                "hp=%u pct patients=%u spell=%u",
                me->GetGUIDLow(), patient->GetGUIDLow(),
                uint32(patient->GetHealthPct()), uint32(wounded.size()), spellId);
        }

        EventMap _events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_brother_paxtonAI(creature);
    }
};

// ---------------------------------------------------------------------------
// The rest of the Northshire Valley cast.
//
// Paxton above only covers the healing. What actually produces the wounded he
// heals is this trio:
//
//   49869  Stormwind Infantry                 shouts for a healer, then calls
//                                             Paxton over and asks for a heal
//   49871  Blackrock Battle Worg              locks onto a nearby infantry and
//                                             fights it (this is what wounds them)
//   50378  Injured Stormwind Infantry (dummy) hands the "Fear No Evil" quest
//                                             instructions to players nearby
//
// They are the upstream implementation, taken from The-Cataclysm-Preservation-
// Project/TrinityCore -- the Elwynn Forest script the 4.3.4 client ships. They
// used to live in EasternKingdoms/zone_elwynn_forest.cpp and were moved in here
// so the whole Northshire cast sits in one file. RegisterCreatureAI() registers
// by script NAME, not by file, so the database does not care where they live;
// AddSC_northshire_brother_paxton() at the bottom of this file already runs,
// which is why nothing in the build system had to change.
//
// Ported from 4.3.4 to this 7.3.5 tree. Every difference below is a SILENT
// runtime trap rather than a compile error, which is why they are listed:
//   DamageTaken(Unit*, uint32&)          -> + DamageEffectType   (UnitAI.h:214)
//   Talk(id, me)                         -> Talk(id, me->GetGUID())
//   Cell::VisitWorldObjects(...)         -> Trinity::VisitNearbyWorldObject(...)
//   std::vector<Player*> + searcher      -> std::list<Player*> (searcher only takes a list)
//   me->m_Events.AddEventAtOffset(...)   -> EventMap + ScheduleEvent (no such member)
//   infantry->IsAIEnabled()              -> infantry->IsAIEnabled (a field, not a method)
//   #include "CellImpl.h"                -> dropped, this tree has no VisitWorldObjects
//
// NPC_BROTHER_PAXTON and SPELL_PRAYER_OF_HEALING are deliberately NOT redefined
// in the enum below: they already exist in enum BrotherPaxton further up with
// the same values, and redefining an enumerator is a compile error.
// ---------------------------------------------------------------------------

enum NorthshireValley
{
    NPC_STORWIND_INFANTRY                   = 49869,
    NPC_INJURED_NORTHSHIRE_INFANTRY_DUMMY   = 50378,

    EVENT_ASK_FOR_HELP                      = 1,
    EVENT_SELECT_INFANTRY,
    EVENT_HEAL_INFANTRY,
    EVENT_GIVE_RIGHT_CLICK_INSTRUCTIONS,

    // The infantry picks one of four lines at random. CreatureTextMgr randomises
    // WITHIN a group, so four separate groups are what makes it random across
    // the four -- hence four ids and not one group with four rows.
    SAY_ASK_FOR_HELP_1                      = 0,
    SAY_ASK_FOR_HELP_2                      = 1,
    SAY_ASK_FOR_HELP_3                      = 2,
    SAY_ASK_FOR_HELP_4                      = 3,
    SAY_HEAL_INFANTRY                       = 0,
    SAY_RIGHT_CLICK                         = 0,

    SPELL_RENEW                             = 93094,
    SPELL_FLASH_HEAL                        = 17843,

    SPELL_GIVEN_RIGHT_CLICK_INSTRUCTION     = 93450,
};

static uint32 const FearNoEvilQuests[] =
{
    28806,
    28807,
    28808,
    28809,
    28810,
    28811,
    28812,
    28813,
    29082
};

struct npc_stormwind_infantry : public ScriptedAI
{
    npc_stormwind_infantry(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_ASK_FOR_HELP, Seconds(urand(1, 10) * 10));
    }

    void UpdateAI(uint32 diff) override
    {
        // No UpdateVictim() here on purpose: this NPC shouts for a healer, it is
        // not meant to fight back on its own.
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_ASK_FOR_HELP:
                    Talk({ SAY_ASK_FOR_HELP_1, SAY_ASK_FOR_HELP_2, SAY_ASK_FOR_HELP_3, SAY_ASK_FOR_HELP_4 });
                    _events.ScheduleEvent(EVENT_ASK_FOR_HELP, Seconds(urand(3, 10) * 10));
                    _events.ScheduleEvent(EVENT_HEAL_INFANTRY, 2s);
                    break;
                case EVENT_HEAL_INFANTRY:
                    if (Creature* paxton = me->FindNearestCreature(NPC_BROTHER_PAXTON, 30.0f, true))
                    {
                        if (!paxton->HasUnitState(UNIT_STATE_CASTING))
                        {
                            if (CreatureAI* paxtonAI = paxton->AI())
                                paxtonAI->Talk(SAY_HEAL_INFANTRY, me->GetGUID());

                            if (me->GetHealthPct() <= 85)
                            {
                                paxton->StopMoving();
                                switch (urand(0, 2))
                                {
                                    case 0: // Prayer of Healing
                                        paxton->CastSpell(paxton, SPELL_PRAYER_OF_HEALING);
                                        break;
                                    case 1: // Renew
                                        paxton->SetFacingToObject(me);
                                        paxton->CastSpell(me, SPELL_RENEW);
                                        break;
                                    case 2: // Flash Heal
                                        paxton->SetFacingToObject(me);
                                        paxton->CastSpell(me, SPELL_FLASH_HEAL);
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap _events;
};

struct npc_blackrock_battle_worg : public ScriptedAI
{
    npc_blackrock_battle_worg(Creature* creature) : ScriptedAI(creature), _isAttackingInfantry(false) { }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_SELECT_INFANTRY, 1s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Creature* infantry = ObjectAccessor::GetCreature(*me, _infantryGUID))
            if (CreatureAI* ai = infantry->AI())
                ai->EnterEvadeMode();
    }

    void DamageTaken(Unit* attacker, uint32& /*damage*/, DamageEffectType /*dmgType*/) override
    {
        if (attacker && attacker->GetEntry() != NPC_STORWIND_INFANTRY)
            _isAttackingInfantry = false;
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SELECT_INFANTRY:
                    SelectInfantry();
                    // Upstream tries exactly once, a second after spawning. If no
                    // infantry happens to stand within 5 yards at that moment the
                    // worg would never fight anything for the rest of its life, so
                    // keep looking every 5s until he finds somebody.
                    if (!_isAttackingInfantry)
                        _events.ScheduleEvent(EVENT_SELECT_INFANTRY, 5s);
                    break;
                default:
                    break;
            }
        }

        if (!_isAttackingInfantry && !UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }

private:
    void SelectInfantry()
    {
        if (Creature* infantry = me->FindNearestCreature(NPC_STORWIND_INFANTRY, 5.0f, true))
        {
            _isAttackingInfantry = true;
            _infantryGUID = infantry->GetGUID();
            AttackStart(infantry);

            if (infantry->IsAIEnabled)
                if (CreatureAI* ai = infantry->AI())
                    ai->AttackStart(me);
        }
    }

    EventMap _events;
    ObjectGuid _infantryGUID;
    bool _isAttackingInfantry;
};

struct npc_injured_stormwind_infantry_dummy : public ScriptedAI
{
    npc_injured_stormwind_infantry_dummy(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_GIVE_RIGHT_CLICK_INSTRUCTIONS, 1s);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_GIVE_RIGHT_CLICK_INSTRUCTIONS:
                {
                    std::list<Player*> players;
                    Trinity::AnyPlayerInObjectRangeCheck checker(me, 50.0f);
                    Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, players, checker);
                    Trinity::VisitNearbyWorldObject(me, 50.0f, searcher);
                    for (Player* player : players)
                    {
                        for (uint32 questId : FearNoEvilQuests)
                        {
                            if (player->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (!player->HasAura(SPELL_GIVEN_RIGHT_CLICK_INSTRUCTION))
                                {
                                    me->CastSpell(player, SPELL_GIVEN_RIGHT_CLICK_INSTRUCTION, true);
                                    Talk(SAY_RIGHT_CLICK, player->GetGUID());
                                }
                            }
                        }
                    }
                    _events.Repeat(10s);
                    break;
                }
                default:
                    break;
            }
        }
    }

private:
    EventMap _events;
};

void AddSC_northshire_brother_paxton()
{
    new npc_brother_paxton;
    RegisterCreatureAI(npc_stormwind_infantry);
    RegisterCreatureAI(npc_blackrock_battle_worg);
    RegisterCreatureAI(npc_injured_stormwind_infantry_dummy);
}