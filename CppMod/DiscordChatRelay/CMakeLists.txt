cmake_minimum_required(VERSION 3.22)

set(TARGET DiscordChatRelay)
project(${TARGET})

add_library(${TARGET} SHARED
    "dllmain.cpp"
)

target_include_directories(${TARGET} PRIVATE .)
target_link_libraries(${TARGET} PUBLIC UE4SS winhttp)

set_target_properties(${TARGET} PROPERTIES
    OUTPUT_NAME "main"
    CXX_STANDARD 23
)
