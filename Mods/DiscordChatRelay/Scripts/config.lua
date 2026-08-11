-- ============================================================
-- DiscordChatRelay - Konfiguration
-- ============================================================

local Config = {}

-- Discord Webhook URL des Ziel-Channels
-- (Discord: Channel -> Einstellungen -> Integrationen -> Webhooks -> Neuer Webhook)
Config.WebhookUrl = "https://discord.com/api/webhooks/DEINE_WEBHOOK_ID/DEIN_WEBHOOK_TOKEN"

-- Name, der im Discord als Absender angezeigt wird (Webhook-Username-Override)
Config.BotName = "Palworld Chat"

-- Optionales Avatar-Bild fuer den Webhook (leer lassen fuer Standard)
Config.AvatarUrl = ""

-- Nachrichtenformat:
--   {sender}  = Spielername
--   {message} = Chatnachricht
--   {channel} = Chat-Kategorie (Global/Guild/Say etc.)
Config.MessageFormat = "**{sender}**: {message}"

-- Chat-Kategorie mit in die Nachricht schreiben? (z.B. [Global])
Config.ShowCategory = true

-- Nur bestimmte Chat-Kategorien weiterleiten.
-- Leere Tabelle {} = alle weiterleiten.
-- Moegliche Werte: "Global", "Guild", "Say"
Config.CategoryFilter = {}

-- Join/Leave-Nachrichten aktivieren
Config.EnableJoinLeave = true

-- Format fuer Join/Leave ({player} = Spielername)
Config.JoinFormat = "**{player}** ist dem Server beigetreten"
Config.LeaveFormat = "**{player}** hat den Server verlassen"

-- Wie oft (Sekunden) die Spielerliste auf Joins/Leaves geprueft wird
Config.PollIntervalSeconds = 5

-- Debug-Ausgaben in der UE4SS-Konsole
Config.Debug = false

return Config
