cmake_minimum_required(VERSION 3.8)

project(speedrunners LANGUAGES CXX C ASM_MASM)

add_subdirectory(Blackbone/src)

# Configure which functionality should be available in executable
set(CoD_Overlay FALSE)
set(Starcraft2 FALSE)
set(Civilization TRUE)
set(Apex FALSE)

# This is set automatically based on the above settings. Do not change
set(QT_Overlay FALSE)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # 64 bits
        set(CMAKE_PREFIX_PATH "C:/Qt/5.15.1/msvc2019_64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        # 32 bits
        set(CMAKE_PREFIX_PATH "C:/Qt/5.15.1/msvc2019")
    endif()
endif()
find_package(Qt5 COMPONENTS Widgets 3DCore 3DExtras 3DRender REQUIRED)

set(
    "speedrunners_src"

    "speedrunners.ui"
    "main.cpp"
    "speedrunners.cpp"
    "speedrunners.h"

    "hack_speedrunners.cpp"
)

if(CoD_Overlay)
    set(
        "speedrunners_src"
        "${speedrunners_src}"

        "hack_cod_warzone.cpp"

        "CoD_Warzone/Entity.cpp"
        "CoD_Warzone/GameContext.cpp"
        "CoD_Warzone/Process.cpp"
        "CoD_Warzone/CoD_Warzone.cpp"
	)
    add_compile_definitions(CoD_WARZONE_ENABLED=1)
    set(QT_Overlay TRUE)
else()
    add_compile_definitions(CoD_WARZONE_ENABLED=0)
endif()

if(Starcraft2)
    set(
        "speedrunners_src"
        "${speedrunners_src}"

        "hack_starcraft2.cpp"

        "Starcraft2/Entity.cpp"
        "Starcraft2/GameContext.cpp"
        "Starcraft2/Process.cpp"
        "Starcraft2/Starcraft2.cpp"
	)
    add_compile_definitions(STARCRAFT2_ENABLED=1)
else()
    add_compile_definitions(STARCRAFT2_ENABLED=0)
endif()

if(Civilization)
    set(
        "speedrunners_src"
        "${speedrunners_src}"

        "hack_civilization.cpp"
	)
    add_compile_definitions(CIVILIZATION_ENABLED=1)
else()
    add_compile_definitions(CIVILIZATION_ENABLED=0)
endif()

if(Apex)
    set(
        "speedrunners_src"
        "${speedrunners_src}"

        "hack_apex.cpp"

        #"apexbot/src/stdafx.cpp"
        "apexbot/src/aimassist.cpp"
        "apexbot/src/apexbot.cpp"
        "apexbot/src/cheats.cpp"
        "apexbot/src/config.cpp"
        "apexbot/src/context.cpp"
        "apexbot/src/entities.cpp"
        "apexbot/src/highlight.cpp"
        "apexbot/src/overlay.cpp"
        "apexbot/src/items.cpp"
        "apexbot/src/process.cpp"
        "apexbot/src/scripts.cpp"
        "apexbot/src/solver.cpp"
        "apexbot/src/state.cpp"
    )
    add_compile_definitions(APEX_ENABLED=1)
    set(QT_Overlay TRUE)
else()
    add_compile_definitions(APEX_ENABLED=0)
endif()

if(QT_Overlay)
    set(
        "speedrunners_src"
        "${speedrunners_src}"

        "QtOverlay/overlay.cpp"
        "QtOverlay/overlay_data_interface.cpp"
    )
    add_compile_definitions(QT_OVERLAY_ENABLED=1)
else()
    add_compile_definitions(QT_OVERLAY_ENABLED=0)
endif()

add_executable(
    speedrunners
    "${speedrunners_src}"
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_link_directories(
        speedrunners
        PRIVATE Blackbone/src/3rd_party/DIA/lib/amd64
        PRIVATE Blackbone/src/3rd_party/BeaEngine/Win64/Dll
    )
else()
    target_link_directories(
        speedrunners
        PRIVATE Blackbone/src/3rd_party/DIA/lib
        PRIVATE Blackbone/src/3rd_party/BeaEngine/Win32/Dll
    )
endif()
target_link_libraries(
        speedrunners
        PRIVATE Qt5::Core
        PRIVATE Qt5::Widgets
        PRIVATE Qt5::3DCore
        PRIVATE Qt5::3DExtras
        PRIVATE Qt5::3DRender
        PRIVATE BlackBone
        PRIVATE diaguids
)

if (WIN32)
    get_target_property(QT5_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(QT5_WINDEPLOYQT_EXECUTABLE ${QT5_QMAKE_EXECUTABLE} PATH)
    set(QT5_WINDEPLOYQT_EXECUTABLE "${QT5_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")
    add_custom_command(TARGET speedrunners POST_BUILD
       COMMAND ${QT5_WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE_DIR:speedrunners>)
endif(WIN32)

target_include_directories(
        speedrunners
        PRIVATE "Blackbone/src"
        PRIVATE "apexbot/src"
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # Injection DLL only 32 bits version
    add_library(
        speedrunners_inject
        SHARED
        "inject_speedrunners.cpp"
    )
endif()

if(Starcraft2)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        # Injection DLL only 32 bits version
        add_library(
            starcraft2_inject
            SHARED
            "inject_starcraft2.asm"
            "inject_starcraft2.c"
        )
        target_compile_options(starcraft2_inject PUBLIC "/MT")
        target_link_options(starcraft2_inject PUBLIC "/INCREMENTAL:NO")
    endif()
endif()

if(Civilization)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # Injection DLL only 64 bits version
        add_library(
            inject_civilization
            SHARED
            "inject_civilization.asm"
            "inject_civilization.cpp"
        )
    endif()
endif()
