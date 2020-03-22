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
  set(CMAKE_PREFIX_PATH "C:\\Qt\\5.14.1\\msvc2017_64")
endif()
find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(
    speedrunners_src
    main.cpp
    hack.cpp
    speedrunners.cpp
    speedrunners.h
    speedrunners.ui
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
        PRIVATE Qt5::Widgets
        PRIVATE BlackBone
        PRIVATE "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/DIA SDK/lib/amd64/diaguids.lib"
)

if (WIN32)
if (ANDROID)
# set path variable to <QT HOME>/bin or install *.dll, platform files and resources in target folder
    get_target_property(QT5_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(QT5_WINDEPLOYQT_EXECUTABLE ${QT5_QMAKE_EXECUTABLE} PATH)
    set(QT5_WINDEPLOYQT_EXECUTABLE "${QT5_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")

    add_custom_command(TARGET foot_scanner POST_BUILD
       COMMAND ${QT5_WINDEPLOYQT_EXECUTABLE} --qmldir ${CMAKE_SOURCE_DIR} $<TARGET_FILE_DIR:foot_scanner>)


add_custom_command(TARGET speedrunners POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Core> $<TARGET_FILE_DIR:speedrunners>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Gui> $<TARGET_FILE_DIR:speedrunners>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Widgets> $<TARGET_FILE_DIR:speedrunners>
)
endif(ANDROID)
endif(WIN32)

target_include_directories(
        speedrunners
        PRIVATE Blackbone/src
)