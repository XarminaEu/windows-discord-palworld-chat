# DiscordChatRelay – Palworld UE4SS Mod (Windows, DLL)

UE4SS-**C++-Mod** für Palworld, der folgendes in einen Discord-Channel postet (per Webhook):

- **Ingame-Chat** (Global / Gilde / Sagen)
- **Join/Leave-Nachrichten** („Spieler X ist beigetreten / hat verlassen")
- **Server-Online-Embed** beim Start/Neustart („Server XYZ ist wieder online!")

Beim ersten Start legt der Mod **automatisch** an:
- `DiscordChatRelay.ini` (Konfiguration, im Mod-Ordner)
- `enabled.txt` (damit der Mod aktiv bleibt)

## Installation

1. **UE4SS installieren** ([RE-UE4SS](https://github.com/UE4SS-RE/RE-UE4SS), Experimental-Build für Palworld)
   nach `Palworld\Pal\Binaries\Win64\` (Dedicated Server: `PalServer\Pal\Binaries\Win64\`)

2. **DLL installieren** – die fertige `main.dll` (aus GitHub Actions → Artifacts → `DiscordChatRelay-dll`) nach:
   ```
   ...\Win64\Mods\DiscordChatRelay\dlls\main.dll
   ```

3. **Spiel/Server einmal starten** → `DiscordChatRelay.ini` + `enabled.txt` werden automatisch erstellt.

4. **Webhook eintragen**: Discord → Channel → ⚙️ → Integrationen → Webhooks → *Neuer Webhook* → URL kopieren
   und in `Mods\DiscordChatRelay\DiscordChatRelay.ini` bei `WebhookUrl=` einfügen.

5. **Neu starten** – fertig.

## Konfiguration (`DiscordChatRelay.ini`)

```ini
[Discord]
WebhookUrl=https://discord.com/api/webhooks/...
BotName=Palworld Chat
AvatarUrl=

[Chat]
EnableChat=1
MessageFormat=**{sender}**: {message}
ShowCategory=1

[JoinLeave]
EnableJoinLeave=1
JoinFormat=**{player}** ist dem Server beigetreten
LeaveFormat=**{player}** hat den Server verlassen
PollIntervalSeconds=5

[Startup]
EnableStartupMessage=1
ServerName=Mein Palworld Server
StartupTitle=Server Online
StartupText=**{server}** ist wieder online! Viel Spass beim Spielen!
StartupEmbedColor=5763719
StartupDelaySeconds=15

[Misc]
Debug=0
```

Platzhalter: `{sender}`, `{message}`, `{channel}` (Chat), `{player}` (Join/Leave), `{server}` (Startup).
Embed-Farben (dezimal): 5763719 = grün, 15548997 = rot, 3447003 = blau.

## Selbst bauen / CI

- **GitHub Actions**: Jeder Push auf `CppMod/**` baut die DLL automatisch (siehe `.github/workflows/build-dll.yml`).
  Benötigt das Repo-Secret `EPIC_GITHUB_TOKEN` (GitHub-PAT eines mit Epic verknüpften Accounts, Scope `repo`).
- **Lokal (Windows)**: siehe `CppMod/README.md`.

## Funktionsweise

- **Chat**: `ProcessEvent`-Pre-Callback auf `PalPlayerState:EnterChat_Receive`
- **Join/Leave**: periodischer Vergleich der `PalPlayerState`-Liste in `on_update()`
- **Startup-Embed**: Thread mit konfigurierbarer Verzögerung nach `on_unreal_init()`
- **HTTP**: WinHTTP-POST in eigenem Thread (blockiert das Spiel nicht)

## Troubleshooting

- `Debug=1` setzen und UE4SS-Konsole prüfen (`ConsoleEnabled=1` in `UE4SS-settings.ini`)
- Nach Palworld-Updates ggf. prüfen, ob `EnterChat_Receive` noch existiert (UE4SS Object Dumper)
