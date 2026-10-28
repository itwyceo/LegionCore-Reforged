////////////////////////////////////////////
///                                      ///
///         LEGIONCORE REFORGED          ///
///    Copyright 2026 Titans-Project     ///
///         All Rights Reserved.         ///
///                                      ///
///      Based on TrinityCore code       ///
///   Copyright 2008-2012 TrinityCore    ///
///   <http://www.trinitycore.org/>      ///
///                                      ///
////////////////////////////////////////////

#include "WaypointManager.h"
#include "DatabaseEnv.h"
#include "GridDefines.h"
#include "Log.h"

WaypointMgr* WaypointMgr::instance()
{
    static WaypointMgr instance;
    return &instance;
}

void WaypointMgr::Load()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result;

    // Optional per-path metadata. A missing or empty table means every path keeps the legacy
    // looping behaviour, so a failed query here is informational rather than an error. It is
    // loaded before waypoint_data because the waypoint_data_script block below can return early.
    //       0        1
    result = WorldDatabase.Query("SELECT PathId, Flags FROM waypoint_path");
    if (result)
    {
        uint32 const knownFlags = uint32(WAYPOINT_PATH_FLAG_FOLLOW_PATH_BACKWARDS_FROM_END);

        do
        {
            Field* fields = result->Fetch();

            uint32 pathId = fields[0].GetUInt32();
            uint32 flags = fields[1].GetUInt32();

            if (flags & ~knownFlags)
                TC_LOG_ERROR("sql.sql", "waypoint_path: path %u has unknown flag bits 0x%X (known mask 0x%X), ignoring them", pathId, flags, knownFlags);

            _pathFlagsStore[pathId] = flags & knownFlags;
        }
        while (result->NextRow());
    }

    //                                                0    1         2           3          4            5           6        7      8       9        10              11
    result = WorldDatabase.Query("SELECT id, point, position_x, position_y, position_z, orientation, move_type, speed, delay, action, action_chance, delay_chance FROM waypoint_data ORDER BY id, point");

    // An empty waypoint_data must not skip waypoint_data_script below
    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 waypoints. DB table `waypoint_data` is empty!");
    }

    uint32 count = 0;

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 pathId = fields[0].GetUInt32();
            WaypointPath& path = _waypointStore[pathId];

            float x = fields[2].GetFloat();
            float y = fields[3].GetFloat();
            float z = fields[4].GetFloat();
            float o = fields[5].GetFloat();

            Trinity::NormalizeMapCoord(x);
            Trinity::NormalizeMapCoord(y);

            WaypointData waypoint;
            waypoint.id = fields[1].GetUInt32();
            waypoint.x = x;
            waypoint.y = y;
            waypoint.z = z;
            waypoint.orientation = o;
            waypoint.move_type = fields[6].GetUInt32();

            if (waypoint.move_type >= WAYPOINT_MOVE_TYPE_MAX)
            {
                TC_LOG_ERROR("sql.sql", "Waypoint %u in waypoint_data has invalid move_type, ignoring", waypoint.id);
                continue;
            }

            waypoint.speed = fields[7].GetFloat();
            waypoint.delay = fields[8].GetUInt32();
            waypoint.event_id = fields[9].GetUInt32();
            waypoint.event_chance = fields[10].GetInt16();
            waypoint.delay_chance = fields[11].GetInt16();

            path.push_back(std::move(waypoint));
            ++count;
        }
        while (result->NextRow());
    }

    //                                    0    1         2           3          4            5           6        7      8       9        10
    result = WorldDatabase.Query("SELECT id, point, position_x, position_y, position_z, orientation, move_type, speed, delay, action, action_chance FROM waypoint_data_script ORDER BY id, point");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 waypoints. DB table `waypoint_data_script` is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 pathId = fields[0].GetUInt32();
        WaypointPath& path = _waypointScriptStore[pathId];

        float x = fields[2].GetFloat();
        float y = fields[3].GetFloat();
        float z = fields[4].GetFloat();
        float o = fields[5].GetFloat();

        Trinity::NormalizeMapCoord(x);
        Trinity::NormalizeMapCoord(y);

        WaypointData waypoint;
        waypoint.id = fields[1].GetUInt32();
        waypoint.x = x;
        waypoint.y = y;
        waypoint.z = z;
        waypoint.orientation = o;
        waypoint.move_type = fields[6].GetUInt32();

        if (waypoint.move_type >= WAYPOINT_MOVE_TYPE_MAX)
        {
            TC_LOG_ERROR("sql.sql", "Waypoint %u in waypoint_data_script has invalid move_type, ignoring", waypoint.id);
            continue;
        }

        waypoint.speed = fields[7].GetFloat();
        waypoint.delay = fields[8].GetUInt32();
        waypoint.event_id = fields[9].GetUInt32();
        waypoint.event_chance = fields[10].GetInt16();

        path.push_back(std::move(waypoint));
        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u waypoints in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void WaypointMgr::ReloadPath(uint32 id)
{
    auto itr = _waypointStore.find(id);
    if (itr != _waypointStore.end())
        _waypointStore.erase(itr);

    // Refresh the optional flags entry as well, so a change to waypoint_path is picked up together
    // with the nodes. Generators that are already running keep their value until they re-load.
    _pathFlagsStore.erase(id);
    if (QueryResult flagsResult = WorldDatabase.PQuery("SELECT Flags FROM waypoint_path WHERE PathId = %u", id))
        _pathFlagsStore[id] = flagsResult->Fetch()[0].GetUInt32() & uint32(WAYPOINT_PATH_FLAG_FOLLOW_PATH_BACKWARDS_FROM_END);

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_WAYPOINT_DATA_BY_ID);
    stmt->setUInt32(0, id);
    PreparedQueryResult result = WorldDatabase.Query(stmt);
    if (!result)
        return;

    WaypointPath& path = _waypointStore[id];

    do
    {
        Field* fields = result->Fetch();

        float x = fields[1].GetFloat();
        float y = fields[2].GetFloat();
        float z = fields[3].GetFloat();
        float o = fields[4].GetFloat();

        Trinity::NormalizeMapCoord(x);
        Trinity::NormalizeMapCoord(y);

        WaypointData waypoint;
        waypoint.id = fields[0].GetUInt32();
        waypoint.x = x;
        waypoint.y = y;
        waypoint.z = z;
        waypoint.orientation = o;
        waypoint.move_type = fields[5].GetUInt32();

        if (waypoint.move_type >= WAYPOINT_MOVE_TYPE_MAX)
        {
            TC_LOG_ERROR("sql.sql", "Waypoint %u in waypoint_data has invalid move_type, ignoring", waypoint.id);
            continue;
        }

        waypoint.speed = fields[6].GetFloat();
        waypoint.delay = fields[7].GetUInt32();
        waypoint.event_id = fields[8].GetUInt32();
        waypoint.event_chance = fields[9].GetUInt8();
        waypoint.delay_chance = fields[10].GetUInt8();

        path.push_back(std::move(waypoint));

    }
    while (result->NextRow());

    itr = _waypointScriptStore.find(id);
    if (itr != _waypointScriptStore.end())
        _waypointScriptStore.erase(itr);

    stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_WAYPOINT_DATA_SCRIPT_BY_ID);
    stmt->setUInt32(0, id);
    result = WorldDatabase.Query(stmt);
    if (!result)
        return;

    WaypointPath& pathS = _waypointScriptStore[id];

    do
    {
        Field* fields = result->Fetch();

        float x = fields[1].GetFloat();
        float y = fields[2].GetFloat();
        float z = fields[3].GetFloat();
        float o = fields[4].GetFloat();

        Trinity::NormalizeMapCoord(x);
        Trinity::NormalizeMapCoord(y);

        WaypointData waypoint;
        waypoint.id = fields[0].GetUInt32();
        waypoint.x = x;
        waypoint.y = y;
        waypoint.z = z;
        waypoint.orientation = o;
        waypoint.move_type = fields[5].GetUInt32();

        if (waypoint.move_type >= WAYPOINT_MOVE_TYPE_MAX)
        {
            TC_LOG_ERROR("sql.sql", "Waypoint %u in waypoint_data_script has invalid move_type, ignoring", waypoint.id);
            continue;
        }

        waypoint.speed = fields[6].GetFloat();
        waypoint.delay = fields[7].GetUInt32();
        waypoint.event_id = fields[8].GetUInt32();
        waypoint.event_chance = fields[9].GetUInt8();

        pathS.push_back(std::move(waypoint));

    }
    while (result->NextRow());
}
