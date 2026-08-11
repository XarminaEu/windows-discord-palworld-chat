# DiscordChatRelay – C++ / DLL-Version (UE4SS)

UE4SS-**C++-Mod** (DLL) für Palworld:
- Ingame-Chat → Discord-Webhook
- **Join/Leave-Nachrichten**
- Config-Datei `DiscordChatRelay.ini` wird **beim ersten Start automatisch erstellt**

## Build-Voraussetzungen (Windows)

- Visual Studio 2022 (Workload „Desktopentwicklung mit C++")
- CMake ≥ 3.22
- Git
- Rust/Cargo (wird von UE4SS für den Build benötigt)

## Bauen

```bat
cd CppMod
git clone --recursive https://github.com/UE4SS-RE/RE-UE4SS.git
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Game__Shipping__Win64 --target DiscordChatRelay
```

> Hinweis: UE4SS nutzt eigene Build-Konfigurationen (z.B. `Game__Shipping__Win64`).
> Falls diese Konfiguration nicht angeboten wird, `Release` verwenden bzw. die
> [UE4SS C++-Modding-Doku](https://docs.ue4ss.com/dev/guides/creating-a-c%2B%2B-mod.html) beachten –
> die DLL muss mit **derselben UE4SS-Version** gebaut werden, die im Spiel installiert ist.

Ergebnis: `build\DiscordChatRelay\<Config>\main.dll`

## Installation

```
Palworld\Pal\Binaries\Win64\Mods\DiscordChatRelay\dlls\main.dll
```

Dann in `Mods\mods.txt` aktivieren:
```
DiscordChatRelay : 1
```

## Konfiguration

Beim ersten Start wird neben der DLL automatisch `DiscordChatRelay.ini` erstellt:

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

[Misc]
Debug=0
```

Webhook-URL eintragen, Spiel/Server neu starten – fertig.

## Funktionsweise

- **Chat**: `ProcessEvent`-Pre-Callback filtert auf `EnterChat_Receive`
  (`PalPlayerState`) und liest `Sender`/`Message`/`Category` dynamisch über
  Property-Offsets (robust gegen kleinere Layout-Änderungen).
- **Join/Leave**: In `on_update()` wird alle `PollIntervalSeconds` die Liste der
  `PalPlayerState`-Objekte verglichen; neue Namen = Join, fehlende = Leave.
- **HTTP**: WinHTTP-POST an den Webhook in einem eigenen Thread (blockiert das Spiel nicht).

## Hinweise

- Der Code ist **ungetestet kompiliert** (dieses Projekt wurde auf Linux erstellt) –
  je nach UE4SS-Version können kleinere API-Anpassungen nötig sein
  (z.B. Signatur von `RegisterProcessEventPreCallback` oder `GetOffset_Internal`).
- Die **Lua-Version** in `../Mods/DiscordChatRelay` bietet denselben Funktionsumfang
  ohne Kompilieren und ist der einfachere Weg.
