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

# 添加vcpkg库目录到CMAKE_PREFIX_PATH
list(APPEND CMAKE_PREFIX_PATH "D:/vcpkg/installed/x64-windows-static")

# 设置vcpkg为静态链接模式
set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# 查找所需的库
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 直接设置 OpenCV 库路径和包含目录
set(VCPKG_ROOT "D:/vcpkg")
set(OPENCV_INCLUDE_DIR "${VCPKG_ROOT}/installed/x64-windows-static/include/opencv4")
set(OPENCV_LIB_DIR "${VCPKG_ROOT}/installed/x64-windows-static/lib")

# 添加 OpenCV 包含目录
include_directories(${OPENCV_INCLUDE_DIR})

# 设置我们需要的 OpenCV 静态库（最小化引用）
set(OPENCV_LIBS
    ${OPENCV_LIB_DIR}/opencv_core4.lib
    ${OPENCV_LIB_DIR}/opencv_imgproc4.lib  
    ${OPENCV_LIB_DIR}/opencv_imgcodecs4.lib
)

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
    src/CoverExtractor.cpp
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
    src/CoverExtractor.h
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
    ${OPENCV_LIBS}
) # Qt6 Shared Library

# 静态链接运行时库，避免VCRUNTIME140_1.dll等依赖问题
if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
      # 解决库冲突问题 - 排除动态运行时库
    target_link_options(${PROJECT_NAME} PRIVATE 
        /NODEFAULTLIB:msvcrt.lib
        /NODEFAULTLIB:msvcrtd.lib
        /NODEFAULTLIB:msvcprt.lib
        /NODEFAULTLIB:msvcprtd.lib
    )
    
    # 静态链接所有库，减少动态库依赖
    target_link_options(${PROJECT_NAME} PRIVATE 
        /INCREMENTAL:NO
        /LTCG
    )
    
    # 启用静态链接优化
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/GL>  # 全程序优化
        $<$<CONFIG:Release>:/Oi>  # 内联函数优化
    )
    
    # 添加安全编译选项以减少误报
    target_compile_options(${PROJECT_NAME} PRIVATE
        /GS          # 栈保护 (Buffer Security Check)
        /guard:cf    # 控制流保护 (Control Flow Guard)
    )
    
    # 添加安全链接选项
    target_link_options(${PROJECT_NAME} PRIVATE
        /DYNAMICBASE    # 地址空间布局随机化 (ASLR)
        /NXCOMPAT       # 数据执行保护 (DEP)
        /LARGEADDRESSAWARE  # 大地址感知
    )
    
    # SAFESEH 仅适用于 32位 目标
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_link_options(${PROJECT_NAME} PRIVATE /SAFESEH)
    endif()
endif()

# CURL静态链接配置
target_compile_definitions(${PROJECT_NAME} PRIVATE CURL_STATICLIB)

# 确保所有第三方库使用静态链接
if(MSVC)
    # 添加额外的系统库，确保静态链接完整性
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        ws2_32.lib 
        wldap32.lib 
        crypt32.lib 
        normaliz.lib
    )
endif()
