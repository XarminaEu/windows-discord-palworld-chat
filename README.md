# DiscordChatRelay – Palworld UE4SS Mod (Windows)

Leitet den Palworld-Ingame-Chat per **Discord-Webhook** in einen bestimmten Discord-Channel weiter.
Zusätzlich werden **Join/Leave-Nachrichten** gesendet.

> Es gibt zwei Varianten:
> - **Lua-Mod** (dieser Ordner, `Mods/DiscordChatRelay`) – sofort nutzbar, kein Kompilieren nötig
> - **C++/DLL-Mod** (`CppMod/`) – mit automatisch erzeugter `DiscordChatRelay.ini`, muss unter Windows kompiliert werden (siehe `CppMod/README.md`)

## Voraussetzungen

- Palworld (Windows, Client als Host oder Dedicated Server)
- [UE4SS](https://github.com/UE4SS-RE/RE-UE4SS) (bzw. die Palworld-kompatible Variante, z.B. über PalSchema/Experimental Build)
- Windows 10/11 (nutzt das mitgelieferte `curl.exe`)

## Installation

1. **UE4SS installieren**
   Den UE4SS-Ordnerinhalt nach:
   ```
   Palworld\Pal\Binaries\Win64\
   ```
   (Beim Dedicated Server entsprechend `PalServer\Pal\Binaries\Win64\`)

2. **Mod kopieren**
   Den Ordner `Mods\DiscordChatRelay` aus diesem Verzeichnis nach:
   ```
   Palworld\Pal\Binaries\Win64\Mods\DiscordChatRelay
   ```
   Die Datei `enabled.txt` im Mod-Ordner aktiviert den Mod automatisch.
   Alternativ in `Mods\mods.txt` eintragen:
   ```
   DiscordChatRelay : 1
   ```

3. **Discord-Webhook erstellen**
   - Discord → gewünschter Channel → ⚙️ Channel bearbeiten → **Integrationen** → **Webhooks** → *Neuer Webhook*
   - Webhook-URL kopieren

4. **Konfigurieren**
   `Mods\DiscordChatRelay\Scripts\config.lua` öffnen und die Webhook-URL eintragen:
   ```lua
   Config.WebhookUrl = "https://discord.com/api/webhooks/....."
   ```

5. **Spiel/Server starten** – fertig. Chatnachrichten erscheinen im Discord-Channel.

## Konfiguration (`config.lua`)

| Option           | Beschreibung                                                    |
|------------------|-----------------------------------------------------------------|
| `WebhookUrl`     | Discord-Webhook-URL des Ziel-Channels                            |
| `BotName`        | Anzeigename des Webhooks in Discord                              |
| `AvatarUrl`      | Optionales Avatar-Bild                                           |
| `MessageFormat`  | Format mit `{sender}`, `{message}`, `{channel}`                  |
| `ShowCategory`   | Chat-Kategorie (z.B. `[Global]`) voranstellen                    |
| `CategoryFilter` | Nur bestimmte Kategorien senden, z.B. `{ "Global" }` – leer = alle |
| `Debug`          | Debug-Ausgaben in der UE4SS-Konsole                              |

## Funktionsweise

Der Mod hookt die Funktion `/Script/Pal.PalPlayerState:EnterChat_Receive`,
die bei jeder empfangenen Chatnachricht (Global / Gilde / Sagen) aufgerufen wird.
Die Nachricht wird als JSON-Payload per `curl` asynchron an den Discord-Webhook gesendet,
damit der Game-Thread nicht blockiert.

## Hinweise / Troubleshooting

- **Keine Nachrichten in Discord?**
  - `Config.Debug = true` setzen und die UE4SS-Konsole prüfen.
  - Webhook-URL im Browser testen (sollte JSON-Fehler `Method Not Allowed` o.ä. liefern, nicht 404).
  - Prüfen, ob `curl` verfügbar ist: `curl --version` in `cmd`.
- **Hook schlägt fehl?**
  Nach Palworld-Updates können sich Funktionsnamen ändern. Mit dem UE4SS
  *Object Dumper* prüfen, ob `PalPlayerState:EnterChat_Receive` noch existiert.
- Auf einem **Dedicated Server** sieht der Mod alle Nachrichten. Als Client/Host
  nur die Nachrichten, die dein Client empfängt (Global + eigene Gilde + Nähe).
