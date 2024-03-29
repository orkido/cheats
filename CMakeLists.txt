cmake_minimum_required(VERSION 3.18)

project(speedrunners LANGUAGES CXX C ASM_MASM)

add_subdirectory(Blackbone/src)

# Configure which functionality should be available in executable
set(Fortnite TRUE)
set(Starcraft2 TRUE)
set(Civilization TRUE)
set(Apex TRUE)

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
        set(CMAKE_PREFIX_PATH
            "C:/Qt/6.2.3/msvc2019_64"
            #"C:/Qt/6.0.3/msvc2019_64"
            "C:/Qt/5.15.2/msvc2019_64"
            #"C:/Qt/5.15.1/msvc2019_64"
        )
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        # 32 bits
        set(CMAKE_PREFIX_PATH
            "C:/Qt/5.15.2/msvc2019"
        )
    endif()
endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets Qml QmlModels Quick ShaderTools Quick3D QuickControls2 REQUIRED)
#find_package(Qt6 COMPONENTS Core Gui Widgets Qml QmlModels Quick ShaderTools Quick3D QuickControls2 REQUIRED)

set(
    "speedrunners_src"

    "speedrunners.ui"
    "main.cpp"
    "speedrunners.cpp"
    "speedrunners.h"

    "hack_speedrunners.cpp"
)

if(Fortnite)
    set(
        "speedrunners_src"
        "${speedrunners_src}"
	)
    add_compile_definitions(Fortnite_ENABLED=1)
    set(QT_Overlay TRUE)
else()
    add_compile_definitions(Fortnite_ENABLED=0)
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

qt_add_executable(
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
        PRIVATE Qt${QT_VERSION_MAJOR}::Core
        PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        PRIVATE Qt${QT_VERSION_MAJOR}::Gui
        PRIVATE Qt${QT_VERSION_MAJOR}::Quick
        PRIVATE Qt${QT_VERSION_MAJOR}::Quick3D
        #PRIVATE Qt${QT_VERSION_MAJOR}::3DQuickExtras
        PRIVATE Qt${QT_VERSION_MAJOR}::QuickControls2
        #PRIVATE Qt${QT_VERSION_MAJOR}::3DCore
        #PRIVATE Qt${QT_VERSION_MAJOR}::3DExtras
        #PRIVATE Qt${QT_VERSION_MAJOR}::3DRender
        PRIVATE BlackBone
        PRIVATE diaguids
)

# Resources:
set(qml_resource_files
    "QtOverlay/overlay.qml"
)

qt6_add_resources(speedrunners "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

# Deploy Qt DLL dependencies in install path
if (WIN32)
    get_target_property(QT${QT_VERSION_MAJOR}_QMAKE_EXECUTABLE Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(QT${QT_VERSION_MAJOR}_WINDEPLOYQT_EXECUTABLE ${QT${QT_VERSION_MAJOR}_QMAKE_EXECUTABLE} PATH)
    set(QT${QT_VERSION_MAJOR}_WINDEPLOYQT_EXECUTABLE "${QT${QT_VERSION_MAJOR}_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")
    add_custom_command(TARGET speedrunners POST_BUILD
        COMMAND ${QT${QT_VERSION_MAJOR}_WINDEPLOYQT_EXECUTABLE} --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/QtOverlay" $<TARGET_FILE_DIR:speedrunners>/$<TARGET_FILE_NAME:speedrunners> || (exit 0)
    )
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
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # Injection DLL only 64 bits version
        add_library(
            starcraft2_inject
            SHARED
            "Starcraft2/inject/inject_starcraft2.asm"
            "Starcraft2/inject/inject_starcraft2.c"
        )
        target_include_directories(
            starcraft2_inject
            PRIVATE "Starcraft2/inject"
        )
        # Use multithreaded static runtime libraries
        target_compile_options(starcraft2_inject PUBLIC "/MT")
        # Disable global offet table indirect jumps
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
