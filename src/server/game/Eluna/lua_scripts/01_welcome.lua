--[[
    7.3.5 示例 1：登录 / 登出事件

    PLAYER_EVENT_ON_LOGIN = 3
    PLAYER_EVENT_ON_LOGOUT = 4
--]]

local PLAYER_EVENT_ON_LOGIN = 3
local PLAYER_EVENT_ON_LOGOUT = 4

RegisterPlayerEvent(PLAYER_EVENT_ON_LOGIN, function(event, player)
    player:SendBroadcastMessage(string.format(
        "|cff00ff00[Eluna]|r 欢迎回来，%s！", player:GetName()))
end)

RegisterPlayerEvent(PLAYER_EVENT_ON_LOGOUT, function(event, player)
    print(string.format("[Eluna] %s 下线了", player:GetName()))
end)
