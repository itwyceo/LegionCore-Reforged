// The name of this function should match:
// void Add${NameOfDirectory}Scripts()

void AddSC_custom_artifact_knowledge();
void AddSC_custom_silithus_wound();
void AddSC_northshire_brother_paxton();
void AddSC_npc_lumber_carrier();

// npc_stormwind_infantry / npc_blackrock_battle_worg / npc_injured_stormwind_infantry_dummy
// are deliberately NOT registered from here. Those three CreatureScript names belong to
// the upstream implementation in EasternKingdoms/zone_elwynn_forest.cpp, and a script
// name cannot be registered twice -- ScriptMgr.cpp:805 logs "already assigned with the
// same script name" and then ABORT()s, so registering them here as well would crash the
// server on startup. The hand written versions that used to live in Custom/Northshire/
// were removed when the upstream implementation was adopted.

void AddCustomScripts()
{
    AddSC_custom_artifact_knowledge();
    AddSC_custom_silithus_wound();
    AddSC_northshire_brother_paxton();
    AddSC_npc_lumber_carrier();
}
