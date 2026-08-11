-- ============================================================
-- DiscordChatRelay fuer Palworld (UE4SS Lua Mod)
-- Leitet den Ingame-Chat per Discord-Webhook in einen Channel
-- ============================================================

local Config = require("config")

local CATEGORY_NAMES = {
    [0] = "Global",
    [1] = "Guild",
    [2] = "Say",
}

local function Log(msg)
    print("[DiscordChatRelay] " .. tostring(msg) .. "\n")
end

local function DebugLog(msg)
    if Config.Debug then
        Log(msg)
    end
end

-- ------------------------------------------------------------
-- JSON-String escapen
-- ------------------------------------------------------------
local function JsonEscape(s)
    s = tostring(s)
    s = s:gsub("\\", "\\\\")
    s = s:gsub('"', '\\"')
    s = s:gsub("\r", "")
    s = s:gsub("\n", "\\n")
    s = s:gsub("\t", "\\t")
    -- Steuerzeichen entfernen
    s = s:gsub("[%z\1-\31]", "")
    return s
end

-- ------------------------------------------------------------
-- Nachricht per Webhook senden (curl.exe, ist bei Windows 10/11 dabei)
-- ------------------------------------------------------------
local function SendToDiscord(content)
    if Config.WebhookUrl == nil or Config.WebhookUrl == ""
        or Config.WebhookUrl:find("DEINE_WEBHOOK_ID") then
        Log("WARNUNG: Keine gueltige Webhook-URL in config.lua eingetragen!")
        return
    end

    local payload = '{"content":"' .. JsonEscape(content) .. '"'
    if Config.BotName and Config.BotName ~= "" then
        payload = payload .. ',"username":"' .. JsonEscape(Config.BotName) .. '"'
    end
    if Config.AvatarUrl and Config.AvatarUrl ~= "" then
        payload = payload .. ',"avatar_url":"' .. JsonEscape(Config.AvatarUrl) .. '"'
    end
    payload = payload .. "}"

    -- Payload in Temp-Datei schreiben (vermeidet Escaping-Probleme in der Shell)
    local tmpName = os.getenv("TEMP") .. "\\dcr_" .. tostring(os.time()) .. "_" .. tostring(math.random(100000)) .. ".json"
    local f = io.open(tmpName, "wb")
    if not f then
        Log("FEHLER: Konnte Temp-Datei nicht schreiben: " .. tmpName)
        return
    end
    f:write(payload)
    f:close()

    local cmd = string.format(
        'curl -s -X POST -H "Content-Type: application/json" -d @"%s" "%s" & del "%s"',
        tmpName, Config.WebhookUrl, tmpName
    )

    -- Asynchron ausfuehren, damit der Game-Thread nicht blockiert
    ExecuteAsync(function()
        DebugLog("Sende an Discord: " .. content)
        os.execute('start /B cmd /C "' .. cmd .. '" >nul 2>&1')
    end)
end

-- ------------------------------------------------------------
-- Filter pruefen
-- ------------------------------------------------------------
local function CategoryAllowed(categoryName)
    if Config.CategoryFilter == nil or #Config.CategoryFilter == 0 then
        return true
    end
    for _, allowed in ipairs(Config.CategoryFilter) do
        if allowed == categoryName then
            return true
        end
    end
    return false
end

-- ------------------------------------------------------------
-- Chat-Hook registrieren
-- ------------------------------------------------------------
local function OnChatMessage(self, chatMessageParam)
    local ok, err = pcall(function()
        local chatMessage = chatMessageParam:get()

        local sender = chatMessage.Sender:ToString()
        local message = chatMessage.Message:ToString()
        local categoryIdx = chatMessage.Category
        local categoryName = CATEGORY_NAMES[categoryIdx] or ("Kategorie_" .. tostring(categoryIdx))

        DebugLog(string.format("Chat empfangen [%s] %s: %s", categoryName, sender, message))

        if message == nil or message == "" then
            return
        end
        if not CategoryAllowed(categoryName) then
            DebugLog("Kategorie gefiltert: " .. categoryName)
            return
        end

        local text = Config.MessageFormat
        text = text:gsub("{sender}", sender)
        text = text:gsub("{message}", message)
        text = text:gsub("{channel}", categoryName)

        if Config.ShowCategory then
            text = "[" .. categoryName .. "] " .. text
        end

        SendToDiscord(text)
    end)
    if not ok then
        Log("FEHLER im Chat-Hook: " .. tostring(err))
    end
end

local function RegisterChatHook()
    local ok, err = pcall(function()
        RegisterHook("/Script/Pal.PalPlayerState:EnterChat_Receive", OnChatMessage)
    end)
    if ok then
        Log("Chat-Hook registriert (PalPlayerState:EnterChat_Receive).")
    else
        Log("Konnte Hook noch nicht registrieren, warte auf Spielstart... (" .. tostring(err) .. ")")
        -- Erneut versuchen, sobald die Klasse geladen ist
        NotifyOnNewObject("/Script/Pal.PalPlayerState", function()
            local ok2 = pcall(function()
                RegisterHook("/Script/Pal.PalPlayerState:EnterChat_Receive", OnChatMessage)
            end)
            if ok2 then
                Log("Chat-Hook nachtraeglich registriert.")
            end
            return true -- Callback wieder entfernen
        end)
    end
end

-- ------------------------------------------------------------
-- Join/Leave-Erkennung (Spielerliste wird periodisch verglichen)
-- ------------------------------------------------------------
local knownPlayers = {}
local firstPoll = true

local function PollPlayers()
    ExecuteInGameThread(function()
        local ok, err = pcall(function()
            local current = {}
            local states = FindAllOf("PalPlayerState") or {}
            for _, ps in ipairs(states) do
                if ps:IsValid() then
                    local nameOk, name = pcall(function()
                        return ps:GetPlayerName():ToString()
                    end)
                    if nameOk and name and name ~= "" then
                        current[name] = true
                    end
                end
            end

            if firstPoll then
                knownPlayers = current
                firstPoll = false
                return
            end

            for name in pairs(current) do
                if not knownPlayers[name] then
                    DebugLog("Join erkannt: " .. name)
                    SendToDiscord((Config.JoinFormat:gsub("{player}", name)))
                end
            end
            for name in pairs(knownPlayers) do
                if not current[name] then
                    DebugLog("Leave erkannt: " .. name)
                    SendToDiscord((Config.LeaveFormat:gsub("{player}", name)))
                end
            end

            knownPlayers = current
        end)
        if not ok then
            DebugLog("Fehler beim Spieler-Polling: " .. tostring(err))
        end
    end)
end

local function StartJoinLeaveWatcher()
    if not Config.EnableJoinLeave then
        return
    end
    local interval = (Config.PollIntervalSeconds or 5) * 1000
    LoopAsync(interval, function()
        PollPlayers()
        return false -- Loop weiterlaufen lassen
    end)
    Log("Join/Leave-Ueberwachung aktiv (Intervall: " .. tostring(interval / 1000) .. "s).")
end

math.randomseed(os.time())
Log("Mod geladen. Version 1.1")
RegisterChatHook()
StartJoinLeaveWatcher()
