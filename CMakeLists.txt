cmake_minimum_required(VERSION 3.25) # CMake install : https://cmake.org/download/

project(DownloadIntegrator LANGUAGES CXX)
set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/icon.rc")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_PREFIX_PATH "d:/Qt/6.6.3/msvc2019_64") # Qt Kit Dir
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加vcpkg库目录到CMAKE_PREFIX_PATH（用于pugixml）
list(APPEND CMAKE_PREFIX_PATH "D:/vcpkg/installed/x64-windows")

# 查找所需的库
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Qt组件
find_package(Qt6 COMPONENTS Widgets Network Core Concurrent REQUIRED)

# 查找pugixml库
find_package(pugixml CONFIG REQUIRED)

# Define source files explicitly instead of using aux_source_directory
set(SOURCES
    src/main.cpp
    src/DownloadIntegrator.cpp
    src/ConfigManager.cpp
    src/SettingsDialog.cpp
    src/ModifierParser.cpp
    src/NetworkManager.cpp
    src/ModifierManager.cpp
    src/FileSystem.cpp
    src/UIHelper.cpp
    src/UpdateManager.cpp
    src/DownloadManager.cpp
    src/ModifierInfoManager.cpp
    src/SearchManager.cpp
    src/LanguageManager.cpp
    src/GameMappingManager.cpp
    src/translator.cpp
)

# Define header files
set(HEADERS
    src/DownloadIntegrator.h
    src/ConfigManager.h
    src/SettingsDialog.h
    src/ModifierParser.h
    src/NetworkManager.h
    src/ModifierManager.h
    src/FileSystem.h
    src/UIHelper.h
    src/UpdateManager.h
    src/DownloadManager.h
    src/ModifierInfoManager.h
    src/SearchManager.h
    src/LanguageManager.h
    src/GameMappingManager.h
    src/translator.h
)

# Define UI files
set(UI_FILES
    src/DownloadIntegrator.ui
)

# Define resource files
set(RESOURCE_FILES
    resources/resources.qrc
)

# Add build directory to include path
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Specify MSVC UTF-8 encoding   
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(${PROJECT_NAME}
    WIN32 # If you need a terminal for debug, please comment this statement
    ${app_icon_resource_windows}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCE_FILES}
) 

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Widgets 
    Qt6::Network
    Qt6::Core
    Qt6::Concurrent
    pugixml::pugixml
    CURL::libcurl
    nlohmann_json::nlohmann_json
) # Qt6 Shared Library

# 静态链接运行时库，避免VCRUNTIME140_1.dll等依赖问题
if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    # 解决库冲突问题
    target_link_options(${PROJECT_NAME} PRIVATE 
        $<$<CONFIG:Debug>:/NODEFAULTLIB:msvcrt.lib>
        $<$<CONFIG:Release>:/NODEFAULTLIB:msvcrtd.lib>
    )
endif()
