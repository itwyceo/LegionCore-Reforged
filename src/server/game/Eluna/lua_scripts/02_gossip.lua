--[[
    7.3.5 示例 2：生物对话菜单

    GOSSIP_EVENT_ON_HELLO  = 1
    GOSSIP_EVENT_ON_SELECT = 2

    把 ENTRY 换成你服务器里实际存在的生物 entry。
    方法名沿用 Eluna/ALE 写法；如果 P2 阶段缺少某个方法，
    会在 Lua 报错中直接看到缺失的函数名。
--]]

local GOSSIP_EVENT_ON_HELLO  = 1
local GOSSIP_EVENT_ON_SELECT = 2

local ENTRY = 900001

RegisterCreatureGossipEvent(ENTRY, GOSSIP_EVENT_ON_HELLO, function(event, player, creature)
    player:GossipMenuAddItem(0, "传送到暴风城", 0, 1)
    player:GossipSendMenu(1, creature)
    return false
end)

RegisterCreatureGossipEvent(ENTRY, GOSSIP_EVENT_ON_SELECT, function(event, player, creature, sender, intid)
    if intid == 1 then
        player:Teleport(0, -8949.95, -132.493, 83.5312, 0)
        player:GossipClearMenu()
    end
    return false
end)
