cmake_minimum_required(VERSION 3.22)

project(DiscordChatRelayMods)

# RE-UE4SS muss als Unterordner "RE-UE4SS" neben dieser Datei liegen
# (git clone --recursive https://github.com/UE4SS-RE/RE-UE4SS.git)
add_subdirectory(RE-UE4SS)
add_subdirectory(DiscordChatRelay)
