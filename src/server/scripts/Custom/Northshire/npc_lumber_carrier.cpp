/*
 * Northshire Valley -- the log-carrying peasant (11260).
 *
 * What it does, in plain words: the core walks the peasant around his waypoint
 * loop on its own (from creature_addon.path_id). This script only decides when
 * the log is on his shoulder: he picks it up when he ARRIVES at the FIRST node
 * of the loop, and puts it down when he ARRIVES at the LAST node.
 *
 * Node indexing: WaypointMovementGenerator<Creature>::OnArrived() calls
 * MovementInform(creature) which forwards AI()->MovementInform(WAYPOINT_MOTION_TYPE,
 * _currentNode). _currentNode is ZERO based (it is used directly as
 * _path->at(_currentNode)), so the last node is "id + 1 == path->size()" and
 * the start is "id == 0".
 *
 * ---------------------------------------------------------------------------
 * STATE MACHINE -- pick the log up at node 0, put it down at the delivery node
 * ---------------------------------------------------------------------------
 *   spawn / before reaching node 0 -> 11354 (forced in Reset, no log)
 *   arrive at FIRST node (id == 0) -> 11355 (pick the log up)
 *   walking out with the log       -> 11355 (kept, intermediate nodes do nothing)
 *   arrive at DELIVERY node        -> 11354 (put the log down)
 *   walking back empty             -> 11354 (stays, does NOT flip back)
 *   arrive at FIRST node again     -> 11355 ... and the loop repeats
 *
 * The delivery node is NOT necessarily the last node of the path. When the
 * return trip also runs on waypoints (see LumberCarrierDeliveryNodes below),
 * the delivery node sits in the MIDDLE of the path and the nodes behind it
 * walk him home empty handed. A path that is not listed there keeps the
 * original behaviour of putting the log down at its last node.
 *
 * Reset() calls SetCarry(FALSE), never SetCarry(true). Reset also fires on
 * evade and grid reload, not only on spawn, so forcing "carrying" there would
 * flip the model back to 11355 in the middle of the empty handed walk home.
 * Forcing "no log" is safe because SetCarry() short circuits when the display
 * already matches, so on the walk home it is a no-op.
 *
 * ---------------------------------------------------------------------------
 * WHY C++ AND NOT SmartAI
 * ---------------------------------------------------------------------------
 * 1. Creature::AI_Initialize() checks ScriptName first and only falls back to
 *    AIName when ScriptName is empty. As soon as ScriptName is set on 11260,
 *    a SmartAI row would never run -- so SmartAI is not an alternative here,
 *    it is dead data.
 * 2. creature_addon.bytes2 is NOT an anim kit. Creature.cpp:3399 maps it to
 *    UNIT_BYTES_2_OFFSET_SHEATH_STATE (weapon sheathe state). There is no
 *    anim-kit column anywhere in this schema, so a carrying AnimKit can only
 *    be applied from C++.
 *
 * ---------------------------------------------------------------------------
 * WHY THE NPC DOES NOT MOVE -- THIS IS A DATABASE PROBLEM, NOT THIS SCRIPT
 * ---------------------------------------------------------------------------
 * Verified in sql/base/world (2026-10-19): the 11260 spawns have
 *   creature.MovementType = 0, spawndist = 0, MovementID = 0
 *   no creature_addon row            -> path_id defaults to 0
 *   creature_template_addon.path_id  = 0
 * GetWaypointPath() returns m_path_id, which is loaded from
 * CreatureAddon::path_id (Creature.cpp:3412) -- so it returns 0 and Reset()
 * below has nothing to start. Attach a path with the companion SQL
 * Northshire_lumber_carrier.sql, otherwise this NPC stands still forever and
 * no amount of script fixing will change that.
 *
 * ---------------------------------------------------------------------------
 * THE VISUAL: SWAP BETWEEN TWO FIXED DISPLAYS
 * ---------------------------------------------------------------------------
 * A SpellVisualKit is the "proper" way to attach a prop to a bone, but there
 * is no ready-made log kit in this install: the hotfixes DB only ships kits
 * 94936 / 94938 / 94939 / 94941 and their effect names carry ModelFileDataID
 * = 0, i.e. they attach no model at all.
 *
 * So the log is shown by swapping the whole display between the two peasant
 * models that already ship in the 7.3.5 client -- same skeleton, one with a
 * log on the shoulder and one without:
 *     11354  creature/humanmalepeasant/humanmalepeasant.m2       empty handed
 *     11355  creature/humanmalepeasant/humanmalepeasantwood.m2   carrying
 * Both are hardcoded rather than captured, so the put down state is
 * deterministic and does not depend on whatever the database happens to
 * assign. The spawn display is still read once and logged when it differs,
 * purely as a diagnostic. See DISPLAY_NO_LOG / DISPLAY_CARRYING_LOG below.
 *
 * Answering the original question -- "C++ or SmartAI, or a different model?":
 *   - C++, not SmartAI: ScriptName outranks AIName, and creature_addon.bytes2
 *     is SheathState (Creature.cpp:3399), not an anim kit, so an AnimKit
 *     cannot be stored in this schema at all.
 *   - The model does not need to be *replaced*: the same peasant skeleton has
 *     a wood variant, so swapping the display id is enough and is reversible.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Creature.h"
#include "MotionMaster.h"
#include "WaypointManager.h"
#include "Log.h"

enum LumberCarrier
{
    NPC_LUMBER_CARRIER = 11260,

    // --- how the carried log is actually shown ---------------------------------
    //
    // The log is shown by swapping the whole display between two peasant models
    // that ship inside the 7.3.5 client, so no server side kit lookup is needed
    // and the change is free and fully reversible.
    //
    // Source: Resike/BlizzardInterfaceResources, Resources/Data/ModelDisplayPath.lua
    // for build 26124 (7.3.5):
    //     [11354] = "creature/humanmalepeasant/humanmalepeasant.m2"       <- no log
    //     [11355] = "creature/humanmalepeasant/humanmalepeasantwood.m2"   <- log on shoulder
    //
    // 11354 and 11355 are adjacent ids and form the intended pair. Other ids
    // point at the same two models if you ever need alternatives:
    //     no log : 301 / 308 / 309 / 310 / 341 / 556 / 584 / 586 / 656 /
    //              2438 / 2441 / 2444 / 2528 / 12048 / 17330 / 19305 /
    //              20488 / 21614 / 25850 / 27030 / 44865-44867 / 45341 /
    //              47904 / 47905
    //     log    : 89 / 19142 / 25586 / 11355
    //
    // State machine. The display is changed from MovementInform (node arrivals)
    // and once from Reset() (forcing "no log" for the walk from the spawn point
    // out to the first node):
    //     spawn                    -> 11354 (forced in Reset)
    //     arrive at FIRST node     -> DISPLAY_CARRYING_LOG (pick up)
    //     walking out with the log -> DISPLAY_CARRYING_LOG
    //     arrive at DELIVERY node  -> DISPLAY_NO_LOG        (put down)
    //     walking back empty       -> DISPLAY_NO_LOG        (stays, does NOT flip back)
    DISPLAY_NO_LOG        = 11354,
    DISPLAY_CARRYING_LOG  = 11355,

    // SECONDARY (optional, both default to off): a SpellVisualKit can attach a
    // prop to a bone and an AnimKit can supply a carrying pose. Neither has a
    // usable "log" value in this install -- the hotfixes DB only ships kits
    // 94936 / 94938 / 94941, all with ModelFileDataID = 0, so they attach no
    // model. Leave them 0 unless you find a real kit id.
    SPELLVISUAL_KIT_CARRY = 0,
    SPELLVISUAL_KIT_TYPE  = 0,
    ANIMKIT_CARRY         = 0
};

// ---------------------------------------------------------------------------
// GUIDs whose display must NEVER be touched.
//
// These spawns use display 11035, which is the TREE CHOPPING model, not a log
// carrier. Swapping their display would kill the chopping look, so the script
// leaves them completely alone -- they still walk their waypoint path, they
// just never show or hide a log.
//
// Every other 11260 spawn goes through the normal 11354 / 11355 state machine.
// ---------------------------------------------------------------------------
static constexpr uint32 LumberCarrierDisplayLockedGUIDs[] =
{
    102993,
    6038,
    102987,
    6034
};

// ---------------------------------------------------------------------------
// Which node of a path is the "put the log down" node.
//
// The default (a path that is NOT listed here) is the LAST node, which is what
// this script did originally and is still correct when the delivery point
// really is the end of the path.
//
// A path is listed here when the return trip ALSO runs on waypoints: the
// delivery node then sits in the MIDDLE of the path, and every node behind it
// walks the peasant home empty handed until the loop wraps back to node 0 and
// the log goes on again. Node indices are ZERO based, exactly like the id that
// WaypointMovementGenerator::OnArrived() reports.
//
// Keep this table in sync with the waypoint_data rows. If a listed index runs
// past the end of its path the script logs an error and falls back to the last
// node instead of silently never putting the log down.
// ---------------------------------------------------------------------------
struct LumberCarrierDelivery
{
    uint32 pathId;
    uint32 nodeIndex;
};

static constexpr LumberCarrierDelivery LumberCarrierDeliveryNodes[] =
{
    // 60350: 5 nodes laid out 1-2-3-2-1. Turn around at point 3, then walk
    // home empty handed through point 4 back to the pile.
    { 60350,   2 },

    // 60360: 8 outbound nodes (points 1..8) plus the mirrored return nodes
    // added by Northshire_lumber_carrier_return_legs.sql, so the turn around
    // is point 8 and nodes 9..15 walk him home.
    { 60360,   7 },

    // 60370: 7 nodes laid out 1-2-3-4-3-2-1. Turn around at point 4, then walk
    // home empty handed through point 5.
    { 60370,   3 },

    // 1029960: 9 nodes laid out 1-2-3-4-5-4-3-2-1. Turn around at point 5,
    // then walk home empty handed through point 6.
    { 1029960, 4 }
};

class npc_lumber_carrier : public CreatureScript
{
public:
    npc_lumber_carrier() : CreatureScript("npc_lumber_carrier") { }

    struct npc_lumber_carrierAI : public ScriptedAI
    {
        npc_lumber_carrierAI(Creature* creature) : ScriptedAI(creature), _carrying(false), _displayLocked(false), _normalDisplayId(0), _deliveryNode(0) { }

        void Reset() override
        {
            // creature.modelid is 0 on all spawns, so the real display comes
            // from the client DBC and cannot be read off the database. It is
            // captured once at spawn purely so the running value is visible.
            _normalDisplayId = me->GetDisplayId();
            _displayLocked   = IsDisplayLocked();

            TC_LOG_ERROR("scripts", "[lumber_carrier] guid=%u entry=%u spawned with display %u, "
                "display %s (no log = %d, carrying = %d).",
                me->GetGUIDLow(), me->GetEntry(), _normalDisplayId,
                _displayLocked ? "LOCKED - will not be modified" : "forced to no log until node 0",
                DISPLAY_NO_LOG, DISPLAY_CARRYING_LOG);

            // Force "no log" for the whole walk from the spawn point to the
            // first waypoint node. Without this the NPC wears whatever the
            // database / client DBC happens to give it until node 0 is reached,
            // which is not 11354 and looks wrong.
            //
            // This is SetCarry(FALSE), never SetCarry(true). Reset() also fires
            // on evade and grid reload, not only on spawn, so forcing
            // "carrying" here would swap the model back to 11355 in the middle
            // of the empty handed walk home. Forcing "no log" is safe: on the
            // walk home it is already the correct state and SetCarry()
            // short circuits on a matching display, and after an evade he
            // restarts from home anyway.
            SetCarry(false);

            // The core already launches the waypoint walk from
            // creature_addon.path_id while spawning. Only re-arm it when the
            // slot really is empty (evade / reset after spawn). Re-issuing the
            // walk unconditionally would restart the loop from node 0 on every
            // single Reset and the peasant would never get anywhere.
            //
            // Why InitDefault() and not MovePath(pathId, ...)? MotionMaster::
            // MovePath() rejects a path id of 0, and a NON-ZERO id makes
            // WaypointMovementGenerator::LoadPath() resolve through
            // sWaypointMgr->GetPathScript() -- i.e. the waypoint_data_script
            // table, which is a DIFFERENT table from the waypoint_data one that
            // GetPath() reads (and that the core's own default generator uses
            // through id 0). When the two tables disagree on the node count the
            // creature walks one path while MovementInform() measures another,
            // so the delivery node test never fires. InitDefault() rebuilds the
            // creature's default generator with id 0, which walks precisely the
            // path measured below.
            WaypointPath const* path = sWaypointMgr->GetPath(me->GetWaypointPath());
            if (!path || path->empty())
            {
                TC_LOG_ERROR("scripts", "[lumber_carrier] guid=%u entry=%u: waypoint_data has no "
                    "path %u. He will stand still. Attach a path first.",
                    me->GetGUIDLow(), me->GetEntry(), me->GetWaypointPath());
                return;
            }

            // Which node is the "put the log down" node?
            //
            // Default is the LAST node (id + 1 == path->size()), the original
            // behaviour, still correct when the delivery point really is the
            // end of the path.
            //
            // When the return trip also runs on waypoints the delivery node is
            // in the MIDDLE of the path and the nodes behind it walk him home
            // empty handed. Those paths are listed in LumberCarrierDeliveryNodes
            // above; anything not listed falls back to the last node, so a new
            // path keeps working without being registered here first.
            _deliveryNode = uint32(path->size()) - 1;
            for (LumberCarrierDelivery const& entry : LumberCarrierDeliveryNodes)
                if (entry.pathId == me->GetWaypointPath())
                {
                    if (entry.nodeIndex < path->size())
                        _deliveryNode = entry.nodeIndex;
                    else
                        TC_LOG_ERROR("scripts", "[lumber_carrier] path %u lists delivery node %u but only has "
                            "%u nodes - falling back to the last node.",
                            entry.pathId, entry.nodeIndex, uint32(path->size()));
                    break;
                }

            if (_deliveryNode == 0)
                TC_LOG_ERROR("scripts", "[lumber_carrier] path %u resolves the delivery node to 0, which is "
                    "also the pick up node - the log would go straight back on. Check the table.",
                    me->GetWaypointPath());

            TC_LOG_ERROR("scripts", "[lumber_carrier] guid=%u walks waypoint_data path %u, %u nodes, "
                "delivery node %u.",
                me->GetGUIDLow(), me->GetWaypointPath(), uint32(path->size()), _deliveryNode);

            if (me->GetMotionMaster()->GetMotionSlotType(MOTION_SLOT_IDLE) != WAYPOINT_MOTION_TYPE)
                me->GetMotionMaster()->InitDefault();
        }

        // OnArrived() forwards the ZERO based node index, so id == 0 is the
        // first node of the loop. The put down node is _deliveryNode, which
        // Reset() resolved: the last node by default, or the entry from
        // LumberCarrierDeliveryNodes when the return trip runs on waypoints.
        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != WAYPOINT_MOTION_TYPE)
                return;

            WaypointPath const* path = sWaypointMgr->GetPath(me->GetWaypointPath());
            if (!path || path->empty())
                return;

            if (id >= path->size())
                return;

            // Diagnostic: log every arrival together with its coordinates. If
            // the LAST node sits on top of the FIRST one, the empty handed walk
            // home is zero length -- the generator wraps to node 0 immediately,
            // the log goes back on within the same tick, and the put down is
            // never visible. The coordinates make that obvious at a glance.
            TC_LOG_ERROR("scripts", "[lumber_carrier] guid=%u reached node %u/%u at "
                "(%.1f, %.1f, %.1f), display now %u",
                me->GetGUIDLow(), id, uint32(path->size()),
                double(path->at(id).x), double(path->at(id).y), double(path->at(id).z),
                me->GetDisplayId());

            if (id == 0)
                SetCarry(true);
            else if (id == _deliveryNode)
                SetCarry(false);
        }

        // Nothing to do per tick: the walk is driven by the waypoint
        // generator, the log state by MovementInform. Kept explicit so the
        // intent is obvious and so ScriptedAI's default is not relied on.
        void UpdateAI(uint32 /*diff*/) override { }

    private:
        // True for the GUIDs listed in LumberCarrierDisplayLockedGUIDs. Those
        // spawns use the tree chopping model (11035) and must never have their
        // display swapped, so every display change short circuits on this.
        bool IsDisplayLocked() const
        {
            uint32 const guid = me->GetGUIDLow();
            for (uint32 locked : LumberCarrierDisplayLockedGUIDs)
                if (guid == locked)
                    return true;
            return false;
        }

        void SetCarry(bool carry)
        {
            if (_displayLocked)
                return;

            uint32 const want    = carry ? DISPLAY_CARRYING_LOG : DISPLAY_NO_LOG;
            uint32 const current = me->GetDisplayId();

            _carrying = carry;

            // Compare against the display that is actually on screen, not just
            // against _carrying. The spawn display is whatever the database
            // gives, which may be neither DISPLAY_NO_LOG nor
            // DISPLAY_CARRYING_LOG, so a flag-only check would let the first
            // "put down" pass as a no-op and he would stay on the spawn model.
            if (current == want)
                return;

            me->SetDisplayId(want);

            if (SPELLVISUAL_KIT_CARRY)
                me->SendPlaySpellVisualKit(carry ? SPELLVISUAL_KIT_CARRY : 0, SPELLVISUAL_KIT_TYPE);

            if (ANIMKIT_CARRY)
                me->SetAnimKitId(carry ? ANIMKIT_CARRY : 0);

            TC_LOG_ERROR("scripts", "[lumber_carrier] guid=%u %s the log (display %u -> %u)",
                me->GetGUIDLow(), carry ? "picked up" : "put down", current, want);
        }

        bool _carrying;
        bool _displayLocked;
        uint32 _normalDisplayId;
        uint32 _deliveryNode;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lumber_carrierAI(creature);
    }
};

void AddSC_npc_lumber_carrier()
{
    new npc_lumber_carrier;
}
