// ============================================================
// DiscordChatRelay fuer Palworld (UE4SS C++ Mod)
// - Ingame-Chat -> Discord-Webhook
// - Join/Leave-Nachrichten
// - Config-Datei (INI) wird beim ersten Start automatisch angelegt
// ============================================================

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winhttp.h>

#include <chrono>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <Mod/CppUserModBase.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/FString.hpp>

using namespace RC;
using namespace RC::Unreal;

// ------------------------------------------------------------
// Konfiguration
// ------------------------------------------------------------
struct RelayConfig
{
    std::wstring WebhookUrl   = L"https://discord.com/api/webhooks/DEINE_WEBHOOK_ID/DEIN_WEBHOOK_TOKEN";
    std::wstring BotName      = L"Palworld Chat";
    std::wstring AvatarUrl    = L"";
    std::wstring MessageFormat = L"**{sender}**: {message}";
    std::wstring JoinFormat   = L"**{player}** ist dem Server beigetreten";
    std::wstring LeaveFormat  = L"**{player}** hat den Server verlassen";
    std::wstring ServerName   = L"Mein Palworld Server";
    std::wstring StartupTitle = L"Server Online";
    std::wstring StartupText  = L"**{server}** ist wieder online! Viel Spass beim Spielen!";
    bool ShowCategory         = true;
    bool EnableChat           = true;
    bool EnableJoinLeave      = true;
    bool EnableStartupMessage = true;
    int  StartupEmbedColor    = 5763719; // gruen
    int  StartupDelaySeconds  = 15;
    int  PollIntervalSeconds  = 5;
    bool Debug                = false;
};

static RelayConfig g_config;

static std::wstring GetThisDllDirectory()
{
    // Pfad der eigenen DLL ermitteln
    HMODULE hModule = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&GetThisDllDirectory), &hModule);

    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(hModule, path, MAX_PATH);

    std::wstring dir(path);
    auto pos = dir.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
    {
        dir = dir.substr(0, pos + 1);
    }
    return dir; // z.B. ...\Mods\DiscordChatRelay\dlls
}

static std::wstring GetModDirectory()
{
    // Mod-Ordner = Elternordner des dlls-Ordners
    std::wstring dir = GetThisDllDirectory();
    if (!dir.empty() && (dir.back() == L'\\' || dir.back() == L'/'))
    {
        dir.pop_back();
    }
    auto pos = dir.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
    {
        dir = dir.substr(0, pos + 1);
    }
    return dir; // z.B. ...\Mods\DiscordChatRelay
}

static std::wstring GetConfigPath()
{
    return GetModDirectory() + L"DiscordChatRelay.ini";
}

static void EnsureModSetup()
{
    // enabled.txt automatisch anlegen, damit der Mod aktiv bleibt
    const std::wstring enabledTxt = GetModDirectory() + L"enabled.txt";
    if (GetFileAttributesW(enabledTxt.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        std::ofstream f(enabledTxt);
        Output::send<LogLevel::Verbose>(STR("[DiscordChatRelay] enabled.txt angelegt: {}\n"), enabledTxt);
    }
}

static void WriteDefaultConfig(const std::wstring& path)
{
    std::wofstream out(path);
    out << L"; ============================================================\n";
    out << L"; DiscordChatRelay - Konfiguration\n";
    out << L"; Diese Datei wurde automatisch erstellt.\n";
    out << L"; ============================================================\n\n";
    out << L"[Discord]\n";
    out << L"; Webhook-URL des Ziel-Channels (Channel -> Integrationen -> Webhooks)\n";
    out << L"WebhookUrl=" << g_config.WebhookUrl << L"\n";
    out << L"; Anzeigename des Webhooks\n";
    out << L"BotName=" << g_config.BotName << L"\n";
    out << L"; Optionales Avatar-Bild (leer = Standard)\n";
    out << L"AvatarUrl=\n\n";
    out << L"[Chat]\n";
    out << L"; Chatnachrichten weiterleiten (1/0)\n";
    out << L"EnableChat=1\n";
    out << L"; Format: {sender}, {message}, {channel}\n";
    out << L"MessageFormat=" << g_config.MessageFormat << L"\n";
    out << L"; Kategorie (z.B. [Global]) voranstellen (1/0)\n";
    out << L"ShowCategory=1\n\n";
    out << L"[JoinLeave]\n";
    out << L"; Join/Leave-Nachrichten senden (1/0)\n";
    out << L"EnableJoinLeave=1\n";
    out << L"; Format: {player}\n";
    out << L"JoinFormat=" << g_config.JoinFormat << L"\n";
    out << L"LeaveFormat=" << g_config.LeaveFormat << L"\n";
    out << L"; Pruefintervall der Spielerliste in Sekunden\n";
    out << L"PollIntervalSeconds=5\n\n";
    out << L"[Startup]\n";
    out << L"; Beim Serverstart eine Embed-Nachricht senden (1/0)\n";
    out << L"EnableStartupMessage=1\n";
    out << L"; Name des Servers ({server} in Titel/Text)\n";
    out << L"ServerName=" << g_config.ServerName << L"\n";
    out << L"StartupTitle=" << g_config.StartupTitle << L"\n";
    out << L"StartupText=" << g_config.StartupText << L"\n";
    out << L"; Embed-Farbe als Dezimalwert (5763719=gruen, 15548997=rot, 3447003=blau)\n";
    out << L"StartupEmbedColor=5763719\n";
    out << L"; Verzoegerung in Sekunden nach Modstart\n";
    out << L"StartupDelaySeconds=15\n\n";
    out << L"[Misc]\n";
    out << L"; Debug-Ausgaben in der UE4SS-Konsole (1/0)\n";
    out << L"Debug=0\n";
}

static std::wstring Trim(const std::wstring& s)
{
    auto start = s.find_first_not_of(L" \t\r\n");
    auto end = s.find_last_not_of(L" \t\r\n");
    if (start == std::wstring::npos) return L"";
    return s.substr(start, end - start + 1);
}

static void LoadConfig()
{
    const std::wstring path = GetConfigPath();

    DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        WriteDefaultConfig(path);
        Output::send<LogLevel::Verbose>(STR("[DiscordChatRelay] Config erstellt: {}\n"), path);
        return;
    }

    std::wifstream in(path);
    std::wstring line;
    std::map<std::wstring, std::wstring> values;
    while (std::getline(in, line))
    {
        line = Trim(line);
        if (line.empty() || line[0] == L';' || line[0] == L'[') continue;
        auto eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;
        values[Trim(line.substr(0, eq))] = Trim(line.substr(eq + 1));
    }

    auto getS = [&](const wchar_t* key, std::wstring& target) {
        auto it = values.find(key);
        if (it != values.end()) target = it->second;
    };
    auto getB = [&](const wchar_t* key, bool& target) {
        auto it = values.find(key);
        if (it != values.end()) target = (it->second == L"1" || it->second == L"true");
    };
    auto getI = [&](const wchar_t* key, int& target) {
        auto it = values.find(key);
        if (it != values.end()) target = _wtoi(it->second.c_str());
    };

    getS(L"WebhookUrl", g_config.WebhookUrl);
    getS(L"BotName", g_config.BotName);
    getS(L"AvatarUrl", g_config.AvatarUrl);
    getS(L"MessageFormat", g_config.MessageFormat);
    getS(L"JoinFormat", g_config.JoinFormat);
    getS(L"LeaveFormat", g_config.LeaveFormat);
    getS(L"ServerName", g_config.ServerName);
    getS(L"StartupTitle", g_config.StartupTitle);
    getS(L"StartupText", g_config.StartupText);
    getB(L"ShowCategory", g_config.ShowCategory);
    getB(L"EnableChat", g_config.EnableChat);
    getB(L"EnableJoinLeave", g_config.EnableJoinLeave);
    getB(L"EnableStartupMessage", g_config.EnableStartupMessage);
    getB(L"Debug", g_config.Debug);
    getI(L"StartupEmbedColor", g_config.StartupEmbedColor);
    getI(L"StartupDelaySeconds", g_config.StartupDelaySeconds);
    getI(L"PollIntervalSeconds", g_config.PollIntervalSeconds);
    if (g_config.PollIntervalSeconds < 1) g_config.PollIntervalSeconds = 5;
    if (g_config.StartupDelaySeconds < 0) g_config.StartupDelaySeconds = 15;
}

// ------------------------------------------------------------
// Hilfsfunktionen
// ------------------------------------------------------------
static std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), out.data(), len, nullptr, nullptr);
    return out;
}

static std::string JsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s)
    {
        switch (c)
        {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': break;
        case '\t': out += "\\t";  break;
        default:
            if (static_cast<unsigned char>(c) >= 0x20 || static_cast<unsigned char>(c) >= 0x80)
            {
                out += c;
            }
            break;
        }
    }
    return out;
}

static std::wstring ReplaceAll(std::wstring s, const std::wstring& from, const std::wstring& to)
{
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::wstring::npos)
    {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

// ------------------------------------------------------------
// Discord-Webhook (WinHTTP, asynchron)
// ------------------------------------------------------------
static std::string BuildCommonFields()
{
    std::string fields;
    if (!g_config.BotName.empty())
    {
        fields += ",\"username\":\"" + JsonEscape(WideToUtf8(g_config.BotName)) + "\"";
    }
    if (!g_config.AvatarUrl.empty())
    {
        fields += ",\"avatar_url\":\"" + JsonEscape(WideToUtf8(g_config.AvatarUrl)) + "\"";
    }
    return fields;
}

static void SendDiscordPayload(const std::string& payload)
{
    if (g_config.WebhookUrl.find(L"DEINE_WEBHOOK_ID") != std::wstring::npos || g_config.WebhookUrl.empty())
    {
        Output::send<LogLevel::Warning>(STR("[DiscordChatRelay] Keine gueltige Webhook-URL in DiscordChatRelay.ini!\n"));
        return;
    }

    std::wstring url = g_config.WebhookUrl;
    bool debug = g_config.Debug;

    std::thread([url, payload, debug]() {
        // URL zerlegen
        URL_COMPONENTS uc{};
        uc.dwStructSize = sizeof(uc);
        wchar_t host[256]{}, urlPath[2048]{};
        uc.lpszHostName = host;   uc.dwHostNameLength = 255;
        uc.lpszUrlPath = urlPath; uc.dwUrlPathLength = 2047;
        if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc))
        {
            Output::send<LogLevel::Warning>(STR("[DiscordChatRelay] Ungueltige Webhook-URL.\n"));
            return;
        }

        HINTERNET hSession = WinHttpOpen(L"DiscordChatRelay/1.0",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return;

        HINTERNET hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (hConnect)
        {
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlPath,
                nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
            if (hRequest)
            {
                const wchar_t* headers = L"Content-Type: application/json\r\n";
                BOOL ok = WinHttpSendRequest(hRequest, headers, (DWORD)-1L,
                    (LPVOID)payload.data(), (DWORD)payload.size(), (DWORD)payload.size(), 0);
                if (ok) WinHttpReceiveResponse(hRequest, nullptr);

                if (debug)
                {
                    DWORD statusCode = 0, size = sizeof(statusCode);
                    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size, WINHTTP_NO_HEADER_INDEX);
                    Output::send<LogLevel::Verbose>(STR("[DiscordChatRelay] Discord HTTP-Status: {}\n"), statusCode);
                }
                WinHttpCloseHandle(hRequest);
            }
            WinHttpCloseHandle(hConnect);
        }
        WinHttpCloseHandle(hSession);
    }).detach();
}

static void SendToDiscord(const std::wstring& content)
{
    std::string payload = "{\"content\":\"" + JsonEscape(WideToUtf8(content)) + "\"" + BuildCommonFields() + "}";
    SendDiscordPayload(payload);
}

static void SendEmbedToDiscord(const std::wstring& title, const std::wstring& description, int color)
{
    std::string payload = "{\"embeds\":[{"
        "\"title\":\"" + JsonEscape(WideToUtf8(title)) + "\","
        "\"description\":\"" + JsonEscape(WideToUtf8(description)) + "\","
        "\"color\":" + std::to_string(color) +
        "}]" + BuildCommonFields() + "}";
    SendDiscordPayload(payload);
}

// ------------------------------------------------------------
// Mod-Klasse
// ------------------------------------------------------------
class DiscordChatRelay : public CppUserModBase
{
private:
    std::set<std::wstring> m_knownPlayers;
    bool m_firstPoll = true;
    std::chrono::steady_clock::time_point m_lastPoll = std::chrono::steady_clock::now();

    static const wchar_t* CategoryName(uint8_t category)
    {
        switch (category)
        {
        case 0: return L"Global";
        case 1: return L"Guild";
        case 2: return L"Say";
        default: return L"Unbekannt";
        }
    }

    void HandleChatMessage(UFunction* function, void* parms)
    {
        if (!g_config.EnableChat) return;

        // Parameter-Struct (FPalChatMessage) dynamisch ueber Property-Offsets lesen,
        // damit der Mod bei Layout-Aenderungen nicht sofort bricht.
        FProperty* senderProp = function->GetPropertyByNameInChain(STR("Sender"));
        FProperty* messageProp = function->GetPropertyByNameInChain(STR("Message"));
        FProperty* categoryProp = function->GetPropertyByNameInChain(STR("Category"));

        // Bei EnterChat_Receive ist der einzige Parameter eine FPalChatMessage-Struct;
        // deren innere Properties liegen relativ zum Parms-Puffer.
        if (!senderProp || !messageProp)
        {
            // Fallback: Properties liegen evtl. in der Struct des ersten Parameters
            FProperty* structParam = function->GetPropertyByNameInChain(STR("ChatMessage"));
            if (!structParam) return;
            // Offsets innerhalb der Struct aufloesen ist versionsabhaengig -> abbrechen
            if (g_config.Debug)
            {
                Output::send<LogLevel::Verbose>(STR("[DiscordChatRelay] Unerwartetes Parameter-Layout.\n"));
            }
            return;
        }

        auto* base = static_cast<uint8_t*>(parms);
        auto* sender = reinterpret_cast<FString*>(base + senderProp->GetOffset_Internal());
        auto* message = reinterpret_cast<FString*>(base + messageProp->GetOffset_Internal());

        std::wstring senderStr = sender->GetCharArray() ? sender->GetCharArray() : L"";
        std::wstring messageStr = message->GetCharArray() ? message->GetCharArray() : L"";
        if (messageStr.empty()) return;

        uint8_t category = 0;
        if (categoryProp)
        {
            category = *(base + categoryProp->GetOffset_Internal());
        }

        std::wstring text = g_config.MessageFormat;
        text = ReplaceAll(text, L"{sender}", senderStr);
        text = ReplaceAll(text, L"{message}", messageStr);
        text = ReplaceAll(text, L"{channel}", CategoryName(category));
        if (g_config.ShowCategory)
        {
            text = L"[" + std::wstring(CategoryName(category)) + L"] " + text;
        }

        if (g_config.Debug)
        {
            Output::send<LogLevel::Verbose>(STR("[DiscordChatRelay] Chat: {} -> {}\n"), senderStr, messageStr);
        }
        SendToDiscord(text);
    }

    void PollPlayers()
    {
        std::set<std::wstring> current;

        std::vector<UObject*> playerStates;
        UObjectGlobals::FindAllOf(STR("PalPlayerState"), playerStates);

        for (UObject* ps : playerStates)
        {
            if (!ps) continue;

            UFunction* getName = ps->GetFunctionByNameInChain(STR("GetPlayerName"));
            if (!getName) continue;

            struct { FString ReturnValue; } params{};
            ps->ProcessEvent(getName, &params);

            if (params.ReturnValue.GetCharArray())
            {
                std::wstring name = params.ReturnValue.GetCharArray();
                if (!name.empty()) current.insert(name);
            }
        }

        if (m_firstPoll)
        {
            m_knownPlayers = current;
            m_firstPoll = false;
            return;
        }

        for (const auto& name : current)
        {
            if (!m_knownPlayers.contains(name))
            {
                if (g_config.Debug) Output::send<LogLevel::Verbose>(STR("[DiscordChatRelay] Join: {}\n"), name);
                SendToDiscord(ReplaceAll(g_config.JoinFormat, L"{player}", name));
            }
        }
        for (const auto& name : m_knownPlayers)
        {
            if (!current.contains(name))
            {
                if (g_config.Debug) Output::send<LogLevel::Verbose>(STR("[DiscordChatRelay] Leave: {}\n"), name);
                SendToDiscord(ReplaceAll(g_config.LeaveFormat, L"{player}", name));
            }
        }

        m_knownPlayers = current;
    }

public:
    DiscordChatRelay()
    {
        ModName = STR("DiscordChatRelay");
        ModVersion = STR("1.0");
        ModDescription = STR("Palworld Chat + Join/Leave -> Discord Webhook");
        ModAuthors = STR("WinDiscord");
    }

    void on_unreal_init() override
    {
        EnsureModSetup();
        LoadConfig();
        Output::send<LogLevel::Verbose>(STR("[DiscordChatRelay] Mod geladen. Config: {}\n"), GetConfigPath());

        // Server-Online-Embed nach konfigurierbarer Verzoegerung senden
        if (g_config.EnableStartupMessage)
        {
            std::thread([]() {
                std::this_thread::sleep_for(std::chrono::seconds(g_config.StartupDelaySeconds));
                std::wstring title = ReplaceAll(g_config.StartupTitle, L"{server}", g_config.ServerName);
                std::wstring text = ReplaceAll(g_config.StartupText, L"{server}", g_config.ServerName);
                SendEmbedToDiscord(title, text, g_config.StartupEmbedColor);
                Output::send<LogLevel::Verbose>(STR("[DiscordChatRelay] Server-Online-Nachricht gesendet.\n"));
            }).detach();
        }

        Hook::RegisterProcessEventPreCallback(
            [this](UObject* context, UFunction* function, void* parms) {
                if (!function) return;
                if (function->GetName() == STR("EnterChat_Receive"))
                {
                    HandleChatMessage(function, parms);
                }
            });
    }

    void on_update() override
    {
        if (!g_config.EnableJoinLeave) return;

        auto now = std::chrono::steady_clock::now();
        if (now - m_lastPoll < std::chrono::seconds(g_config.PollIntervalSeconds)) return;
        m_lastPoll = now;

        PollPlayers();
    }
};

// ------------------------------------------------------------
// UE4SS-Exportfunktionen
// ------------------------------------------------------------
#define DISCORD_CHAT_RELAY_API __declspec(dllexport)
extern "C"
{
    DISCORD_CHAT_RELAY_API CppUserModBase* start_mod()
    {
        return new DiscordChatRelay();
    }

    DISCORD_CHAT_RELAY_API void uninstall_mod(CppUserModBase* mod)
    {
        delete mod;
    }
}
