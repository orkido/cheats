cmake_minimum_required(VERSION 3.8)

project(speedrunners LANGUAGES CXX)

add_subdirectory(Blackbone/src)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # 64 bits
        set(CMAKE_PREFIX_PATH "C:/Qt/5.14.2/msvc2017_64")
        set(DIASDK_LIBRARIES "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/DIA SDK/lib/amd64/diaguids.lib")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        # 32 bits
        set(CMAKE_PREFIX_PATH "C:/Qt/5.14.2/msvc2017")
        set(DIASDK_LIBRARIES "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/DIA SDK/lib/diaguids.lib")
    endif()
endif()
find_package(Qt5 COMPONENTS Widgets 3DCore 3DExtras 3DRender REQUIRED)

set(
    "speedrunners_src"
    "main.cpp"
    "speedrunners.cpp"
    "speedrunners.h"
    "speedrunners.ui"

    "hack_speedrunners.cpp"
    "hack_apex.cpp"

    #"apexbot/src/stdafx.cpp"
    "apexbot/src/aimassist.cpp"
    "apexbot/src/apexbot.cpp"
    "apexbot/src/cheats.cpp"
    "apexbot/src/config.cpp"
    "apexbot/src/context.cpp"
    "apexbot/src/entities.cpp"
    "apexbot/src/highlight.cpp"
    "apexbot/src/items.cpp"
    "apexbot/src/process.cpp"
    "apexbot/src/scripts.cpp"
    "apexbot/src/solver.cpp"
    "apexbot/src/state.cpp"
    "apexbot/src/overlay.cpp"
)

if(ANDROID)
  add_library(
    speedrunners
    SHARED
    ${speedrunners_src}
  )
else()
  add_executable(
    speedrunners
    ${speedrunners_src}
  )
endif()



# Find the DIA SDK path, it will typically look something like this.
# C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\DIA SDK\include
# C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\DIA SDK\include
# C:\Program Files (x86)\Microsoft Visual Studio 14.0\DIA SDK\include
#find_path(DIASDK_INCLUDE_DIR    # Set variable DIASDK_INCLUDE_DIR
#          dia2.h                # Find a path with dia2.h
#          HINTS "${VSWHERE_LATEST}/DIA SDK/include"
#          HINTS "${VS15_C_PATH32}/../../DIA SDK/include" 
#          HINTS "${VS15_P_PATH32}/../../DIA SDK/include"
#          HINTS "${VS15_E_PATH32}/../../DIA SDK/include"
#          HINTS "${VS_PATH64}/../../DIA SDK/include"
#          HINTS "${VS_PATH32}/../../DIA SDK/include"
#          DOC "path to DIA SDK header files"
#          )
#if (CMAKE_GENERATOR MATCHES "Visual Studio.*Win64" )
#  find_library(DIASDK_GUIDS_LIBRARY NAMES diaguids.lib
#               HINTS "${DIASDK_INCLUDE_DIR}/../lib/amd64" )
#elseif (CMAKE_GENERATOR MATCHES "Visual Studio.*ARM" )
#  find_library(DIASDK_GUIDS_LIBRARY NAMES diaguids.lib
#               HINTS "${DIASDK_INCLUDE_DIR}/../lib/arm")
#else (CMAKE_GENERATOR MATCHES "Visual Studio.*Win64" )
#  find_library(DIASDK_GUIDS_LIBRARY NAMES diaguids.lib
#               HINTS "${DIASDK_INCLUDE_DIR}/../lib")
#endif (CMAKE_GENERATOR MATCHES "Visual Studio.*Win64" )

#set(DIASDK_LIBRARIES ${DIASDK_GUIDS_LIBRARY})
#set(DIASDK_INCLUDE_DIRS ${DIASDK_INCLUDE_DIR})

target_link_libraries(
        speedrunners
        PRIVATE Qt5::Core
        PRIVATE Qt5::Widgets
        PRIVATE Qt5::3DCore
        PRIVATE Qt5::3DExtras
        PRIVATE Qt5::3DRender
        PRIVATE BlackBone
        PRIVATE "${DIASDK_LIBRARIES}"
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
        "inject.cpp"
    )
endif()
