/*
 * Eluna 7.3.5 核心桥接（LegionCore-Reforged）
 * ---------------------------------------------------------------------------
 * 作用：把 LegionCore-Reforged 原生的 ScriptMgr 回调转发给 Eluna。
 * 这样不需要修改核心的 ScriptMgr.cpp，只需要把本文件加入 core 的脚本加载列表。
 *
 * 注意：
 *   - Reforged 原生没有 UnitScript / ServerScript / AllCreatureScript，
 *     因此单位伤害、封包、全量生物事件暂时无法桥接，需要核心补丁。
 *   - PlayerScript::OnChat 返回 void，Eluna 的“拦截聊天”返回值无法生效，
 *     但 Lua 修改 msg 仍然有效。
 * ---------------------------------------------------------------------------
 */

#include "Eluna_SC.h"

#include "LuaEngine.h"
#include "ElunaIncludes.h"
#include "ElunaConfig.h"
#include "ElunaLoader.h"

#include "QuestDef.h"
#include "Item.h"
#include "AuctionHouseMgr.h"
#include "Group.h"
#include "GameTime.h"
#include "Vehicle.h"

namespace
{
    inline bool ElunaReady()
    {
        return sEluna != nullptr && sEluna->HasLuaState();
    }
}

/* ===========================================================================
 * WorldScript：负责初始化、脚本加载、世界更新与关闭
 * ========================================================================= */

class ElunaWorldScript : public WorldScript
{
public:
    ElunaWorldScript() : WorldScript("ElunaWorldScript") { }

    void OnConfigLoad(bool reload) override
    {
        sElunaConfig->Initialize();

        if (sElunaConfig->IsElunaEnabled())
            sElunaLoader->LoadScripts();

        if (ElunaReady())
            sEluna->OnConfigLoad(reload);
    }

    void OnStartup() override
    {
        if (!sElunaConfig->IsElunaEnabled())
            return;

        if (!sEluna)
            sEluna = new Eluna(nullptr);

        if (ElunaReady())
            sEluna->OnStartup();
    }

    void OnUpdate(uint32 diff) override
    {
        if (!ElunaReady())
            return;

        sEluna->UpdateEluna(diff);
        sEluna->OnWorldUpdate(diff);
    }

    void OnShutdown() override
    {
        if (ElunaReady())
            sEluna->OnShutdown();

        // 进程即将退出，这里不 delete，避免 Map/Player 析构阶段访问已释放的 Lua 状态
    }

    void OnOpenStateChange(bool open) override
    {
        if (ElunaReady())
            sEluna->OnOpenStateChange(open);
    }

    void OnShutdownInitiate(ShutdownExitCode code, ShutdownMask mask) override
    {
        if (ElunaReady())
            sEluna->OnShutdownInitiate(code, mask);
    }

    void OnShutdownCancel() override
    {
        if (ElunaReady())
            sEluna->OnShutdownCancel();
    }
};

/* ===========================================================================
 * PlayerScript
 * ========================================================================= */

class ElunaPlayerScript : public PlayerScript
{
public:
    ElunaPlayerScript() : PlayerScript("ElunaPlayerScript") { }

    /*
     * Reforged 的 ScriptMgr::OnPlayerLogin 会同时调用
     *   OnLogin(player) 和 OnLogin(player, firstLogin)
     * 所以这里只重写带 firstLogin 的版本，避免 Lua 登录事件触发两次。
     */
    void OnLogin(Player* player, bool firstLogin) override
    {
        if (!ElunaReady())
            return;

        sEluna->OnLogin(player);

        if (firstLogin)
            sEluna->OnFirstLogin(player);
    }

    void OnLogout(Player* player) override
    {
        if (ElunaReady())
            sEluna->OnLogout(player);
    }

    void OnCreate(Player* player) override
    {
        if (ElunaReady())
            sEluna->OnCreate(player);
    }

    void OnDelete(ObjectGuid const& guid) override
    {
        if (ElunaReady())
            sEluna->OnDelete(static_cast<uint32>(guid.GetCounter()));
    }

    void OnLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (ElunaReady())
            sEluna->OnLevelChanged(player, oldLevel);
    }

    void OnFreeTalentPointsChanged(Player* player, uint32 points) override
    {
        if (ElunaReady())
            sEluna->OnFreeTalentPointsChanged(player, points);
    }

    void OnTalentsReset(Player* player, bool noCost) override
    {
        if (ElunaReady())
            sEluna->OnTalentsReset(player, noCost);
    }

    void OnMoneyChanged(Player* player, int64& amount) override
    {
        if (ElunaReady())
            sEluna->OnMoneyChanged(player, amount);
    }

    void OnGiveXP(Player* player, uint32& amount, Unit* victim) override
    {
        if (ElunaReady())
            sEluna->OnGiveXP(player, amount, victim);
    }

    void OnReputationChange(Player* player, uint32 factionId, int32& standing, bool incremental) override
    {
        if (ElunaReady())
            sEluna->OnReputationChange(player, factionId, standing, incremental);
    }

    void OnDuelRequest(Player* target, Player* challenger) override
    {
        if (ElunaReady())
            sEluna->OnDuelRequest(target, challenger);
    }

    void OnDuelStart(Player* player1, Player* player2) override
    {
        if (ElunaReady())
            sEluna->OnDuelStart(player1, player2);
    }

    void OnDuelEnd(Player* winner, Player* loser, DuelCompleteType type) override
    {
        if (ElunaReady())
            sEluna->OnDuelEnd(winner, loser, type);
    }

    /* 聊天：msg 为引用，Lua 修改可生效；返回值无法阻断（核心签名是 void） */
    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg) override
    {
        if (ElunaReady())
            sEluna->OnChat(player, type, lang, msg);
    }

    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Player* receiver) override
    {
        if (ElunaReady())
            sEluna->OnChat(player, type, lang, msg, receiver);
    }

    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Group* group) override
    {
        if (ElunaReady())
            sEluna->OnChat(player, type, lang, msg, group);
    }

    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Guild* guild) override
    {
        if (ElunaReady())
            sEluna->OnChat(player, type, lang, msg, guild);
    }

    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Channel* channel) override
    {
        if (ElunaReady())
            sEluna->OnChat(player, type, lang, msg, channel);
    }

    void OnTextEmote(Player* player, uint32 textEmote, uint32 emoteNum, ObjectGuid const& guid) override
    {
        if (ElunaReady())
            sEluna->OnTextEmote(player, textEmote, emoteNum, guid);
    }

    void OnSpellCast(Player* player, Spell* spell, bool skipCheck) override
    {
        if (ElunaReady())
            sEluna->OnSpellCast(player, spell, skipCheck);
    }

    void OnBindToInstance(Player* player, Difficulty difficulty, uint32 mapId, bool permanent) override
    {
        if (ElunaReady())
            sEluna->OnBindToInstance(player, difficulty, mapId, permanent);
    }

    void OnUpdateZone(Player* player, uint32 newZone, uint32 newArea) override
    {
        if (ElunaReady())
            sEluna->OnUpdateZone(player, newZone, newArea);
    }

    void OnMapChanged(Player* player) override
    {
        if (ElunaReady())
            sEluna->OnMapChanged(player);
    }

    void OnSpellLearned(Player* player, uint32 spellId) override
    {
        if (ElunaReady())
            sEluna->OnLearnSpell(player, spellId);
    }

    void OnSendMail(Player* player, std::string& /*subject*/, std::string& /*body*/, ObjectGuid receiver) override
    {
        if (ElunaReady())
            sEluna->OnSendMail(player, receiver);
    }

    void OnLootItem(Player* player, Item* item, uint32 count) override
    {
        if (ElunaReady())
            sEluna->OnLootItem(player, item, count, ObjectGuid::Empty);
    }

    void OnPVPKill(Player* killer, Player* killed) override
    {
        if (ElunaReady())
            sEluna->OnPVPKill(killer, killed);
    }

    void OnCreatureKill(Player* killer, Creature* killed) override
    {
        if (ElunaReady())
            sEluna->OnCreatureKill(killer, killed);
    }

    void OnPlayerKilledByCreature(Creature* killer, Player* killed) override
    {
        if (ElunaReady())
            sEluna->OnPlayerKilledByCreature(killer, killed);
    }

    void OnQuestAbandon(Player* player, Quest const* quest) override
    {
        if (ElunaReady() && quest)
            sEluna->OnQuestAbandon(player, quest->GetQuestId());
    }
};

/* ===========================================================================
 * CreatureScript
 * ========================================================================= */

class ElunaCreatureScript : public CreatureScript
{
public:
    ElunaCreatureScript() : CreatureScript("ElunaCreatureScript") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        return ElunaReady() ? sEluna->OnGossipHello(player, creature) : false;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        return ElunaReady() ? sEluna->OnGossipSelect(player, creature, sender, action) : false;
    }

    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code) override
    {
        return ElunaReady() ? sEluna->OnGossipSelectCode(player, creature, sender, action, code) : false;
    }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        return ElunaReady() ? sEluna->OnQuestAccept(player, creature, quest) : false;
    }

    bool OnQuestReward(Player* player, Creature* creature, Quest const* quest, uint32 opt) override
    {
        return ElunaReady() ? sEluna->OnQuestReward(player, creature, quest, opt) : false;
    }

    bool OnDummyEffect(Unit* caster, uint32 spellId, SpellEffIndex effIndex, Creature* target) override
    {
        if (!ElunaReady())
            return false;

        sEluna->OnDummyEffect(caster, spellId, effIndex, target);
        return false;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return ElunaReady() ? sEluna->GetAI(creature) : nullptr;
    }
};

/* ===========================================================================
 * GameObjectScript
 * ========================================================================= */

class ElunaGameObjectScript : public GameObjectScript
{
public:
    ElunaGameObjectScript() : GameObjectScript("ElunaGameObjectScript") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        return ElunaReady() ? sEluna->OnGossipHello(player, go) : false;
    }

    bool OnGossipSelect(Player* player, GameObject* go, uint32 sender, uint32 action) override
    {
        return ElunaReady() ? sEluna->OnGossipSelect(player, go, sender, action) : false;
    }

    bool OnGossipSelectCode(Player* player, GameObject* go, uint32 sender, uint32 action, const char* code) override
    {
        return ElunaReady() ? sEluna->OnGossipSelectCode(player, go, sender, action, code) : false;
    }

    bool OnQuestAccept(Player* player, GameObject* go, Quest const* quest) override
    {
        return ElunaReady() ? sEluna->OnQuestAccept(player, go, quest) : false;
    }

    bool OnQuestReward(Player* player, GameObject* go, Quest const* quest, uint32 opt) override
    {
        return ElunaReady() ? sEluna->OnQuestReward(player, go, quest, opt) : false;
    }

    bool OnDummyEffect(Unit* caster, uint32 spellId, SpellEffIndex effIndex, GameObject* target) override
    {
        if (!ElunaReady())
            return false;

        sEluna->OnDummyEffect(caster, spellId, effIndex, target);
        return false;
    }

    void OnDestroyed(GameObject* go, Player* player) override
    {
        if (ElunaReady())
            sEluna->OnDestroyed(go, player);
    }

    void OnDamaged(GameObject* go, Player* player) override
    {
        if (ElunaReady())
            sEluna->OnDamaged(go, player);
    }

    void OnLootStateChanged(GameObject* go, uint32 state, Unit* /*unit*/) override
    {
        if (ElunaReady())
            sEluna->OnLootStateChanged(go, state);
    }

    void OnGameObjectStateChanged(GameObject* go, uint32 state) override
    {
        if (ElunaReady())
            sEluna->OnGameObjectStateChanged(go, state);
    }
};

/* ===========================================================================
 * ItemScript
 * ========================================================================= */

class ElunaItemScript : public ItemScript
{
public:
    ElunaItemScript() : ItemScript("ElunaItemScript") { }

    bool OnQuestAccept(Player* player, Item* item, Quest const* quest) override
    {
        return ElunaReady() ? sEluna->OnQuestAccept(player, item, quest) : false;
    }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& targets) override
    {
        return ElunaReady() ? sEluna->OnUse(player, item, targets) : false;
    }

    bool OnExpire(Player* player, ItemTemplate const* proto) override
    {
        return ElunaReady() ? sEluna->OnExpire(player, proto) : false;
    }

    bool OnDummyEffect(Unit* caster, uint32 spellId, SpellEffIndex effIndex, Item* target) override
    {
        if (!ElunaReady())
            return false;

        sEluna->OnDummyEffect(caster, spellId, effIndex, target);
        return false;
    }
};

/* ===========================================================================
 * GuildScript
 * ========================================================================= */

class ElunaGuildScript : public GuildScript
{
public:
    ElunaGuildScript() : GuildScript("ElunaGuildScript") { }

    void OnAddMember(Guild* guild, Player* player, uint8& plRank) override
    {
        if (ElunaReady())
            sEluna->OnAddMember(guild, player, plRank);
    }

    void OnRemoveMember(Guild* guild, Player* player, bool isDisbanding, bool /*isKicked*/) override
    {
        if (ElunaReady())
            sEluna->OnRemoveMember(guild, player, isDisbanding);
    }

    void OnMOTDChanged(Guild* guild, std::string const& newMotd) override
    {
        if (ElunaReady())
            sEluna->OnMOTDChanged(guild, newMotd);
    }

    void OnInfoChanged(Guild* guild, std::string const& newInfo) override
    {
        if (ElunaReady())
            sEluna->OnInfoChanged(guild, newInfo);
    }

    void OnCreate(Guild* guild, Player* leader, std::string const& name) override
    {
        if (ElunaReady())
            sEluna->OnCreate(guild, leader, name);
    }

    void OnDisband(Guild* guild) override
    {
        if (ElunaReady())
            sEluna->OnDisband(guild);
    }

    void OnMemberWitdrawMoney(Guild* guild, Player* player, uint64& amount, bool isRepair) override
    {
        if (ElunaReady())
            sEluna->OnMemberWitdrawMoney(guild, player, amount, isRepair);
    }

    void OnMemberDepositMoney(Guild* guild, Player* player, uint64& amount) override
    {
        if (ElunaReady())
            sEluna->OnMemberDepositMoney(guild, player, amount);
    }

    void OnItemMove(Guild* guild, Player* player, Item* item, bool isSrcBank, uint8 srcContainer,
        uint8 srcSlotId, bool isDestBank, uint8 destContainer, uint8 destSlotId) override
    {
        if (ElunaReady())
            sEluna->OnItemMove(guild, player, item, isSrcBank, srcContainer, srcSlotId,
                isDestBank, destContainer, destSlotId);
    }

    void OnEvent(Guild* guild, uint8 eventType, uint32 playerGuid1, uint32 playerGuid2, uint8 newRank) override
    {
        if (ElunaReady())
            sEluna->OnEvent(guild, eventType, playerGuid1, playerGuid2, newRank);
    }

    void OnBankEvent(Guild* guild, uint8 eventType, uint8 tabId, uint32 playerGuid,
        uint32 itemOrMoney, uint16 itemStackCount, uint8 destTabId) override
    {
        if (ElunaReady())
            sEluna->OnBankEvent(guild, eventType, tabId, playerGuid, itemOrMoney, itemStackCount, destTabId);
    }
};

/* ===========================================================================
 * GroupScript
 * ========================================================================= */

class ElunaGroupScript : public GroupScript
{
public:
    ElunaGroupScript() : GroupScript("ElunaGroupScript") { }

    void OnAddMember(Group* group, ObjectGuid const& guid) override
    {
        if (ElunaReady())
            sEluna->OnAddMember(group, guid);
    }

    void OnInviteMember(Group* group, ObjectGuid const& guid) override
    {
        if (ElunaReady())
            sEluna->OnInviteMember(group, guid);
    }

    void OnRemoveMember(Group* group, ObjectGuid const& guid, RemoveMethod method,
        ObjectGuid const& /*kicker*/, const char* /*reason*/) override
    {
        if (ElunaReady())
            sEluna->OnRemoveMember(group, guid, static_cast<uint8>(method));
    }

    void OnChangeLeader(Group* group, ObjectGuid const& newLeaderGuid, ObjectGuid const& oldLeaderGuid) override
    {
        if (ElunaReady())
            sEluna->OnChangeLeader(group, newLeaderGuid, oldLeaderGuid);
    }

    void OnDisband(Group* group) override
    {
        if (ElunaReady())
            sEluna->OnDisband(group);
    }
};

/* ===========================================================================
 * VehicleScript
 * ========================================================================= */

class ElunaVehicleScript : public VehicleScript
{
public:
    ElunaVehicleScript() : VehicleScript("ElunaVehicleScript") { }

    void OnInstall(Vehicle* vehicle) override
    {
        if (ElunaReady())
            sEluna->OnInstall(vehicle);
    }

    void OnUninstall(Vehicle* vehicle) override
    {
        if (ElunaReady())
            sEluna->OnUninstall(vehicle);
    }

    void OnInstallAccessory(Vehicle* vehicle, Creature* accessory) override
    {
        if (ElunaReady())
            sEluna->OnInstallAccessory(vehicle, accessory);
    }

    void OnAddPassenger(Vehicle* vehicle, Unit* passenger, int8 seatId) override
    {
        if (ElunaReady())
            sEluna->OnAddPassenger(vehicle, passenger, seatId);
    }

    void OnRemovePassenger(Vehicle* vehicle, Unit* passenger) override
    {
        if (ElunaReady())
            sEluna->OnRemovePassenger(vehicle, passenger);
    }
};

/* ===========================================================================
 * AuctionHouseScript
 * ========================================================================= */

class ElunaAuctionHouseScript : public AuctionHouseScript
{
public:
    ElunaAuctionHouseScript() : AuctionHouseScript("ElunaAuctionHouseScript") { }

    void OnAuctionAdd(AuctionHouseObject* ah, AuctionEntry* entry) override
    {
        if (ElunaReady())
            sEluna->OnAdd(ah, entry);
    }

    void OnAuctionRemove(AuctionHouseObject* ah, AuctionEntry* entry) override
    {
        if (ElunaReady())
            sEluna->OnRemove(ah, entry);
    }

    void OnAuctionSuccessful(AuctionHouseObject* ah, AuctionEntry* entry) override
    {
        if (ElunaReady())
            sEluna->OnSuccessful(ah, entry);
    }

    void OnAuctionExpire(AuctionHouseObject* ah, AuctionEntry* entry) override
    {
        if (ElunaReady())
            sEluna->OnExpire(ah, entry);
    }
};

/* ===========================================================================
 * CommandScript：提供 .eluna reload
 * ========================================================================= */

static bool HandleElunaReloadCommand(ChatHandler* handler, char const* /*args*/)
{
    if (!sEluna)
        return false;

    sElunaLoader->ReloadElunaForMap(RELOAD_ALL_STATES);
    handler->SendSysMessage("Eluna scripts reloaded.");
    return true;
}

class ElunaCommandScript : public CommandScript
{
public:
    ElunaCommandScript() : CommandScript("ElunaCommandScript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> elunaCommandTable =
        {
            { "reload", SEC_ADMINISTRATOR, true, &HandleElunaReloadCommand, "Reload Eluna Lua scripts." }
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "eluna", SEC_ADMINISTRATOR, true, nullptr, "Eluna Lua engine commands.", elunaCommandTable }
        };

        return commandTable;
    }
};

/* ===========================================================================
 * 注册入口
 * ========================================================================= */

void AddSC_eluna()
{
    new ElunaWorldScript();
    new ElunaPlayerScript();
    new ElunaCreatureScript();
    new ElunaGameObjectScript();
    new ElunaItemScript();
    new ElunaGuildScript();
    new ElunaGroupScript();
    new ElunaVehicleScript();
    new ElunaAuctionHouseScript();
    new ElunaCommandScript();
}
