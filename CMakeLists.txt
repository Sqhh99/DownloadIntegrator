cmake_minimum_required(VERSION 3.25) # CMake install : https://cmake.org/download/

# ============================================================================
# [CRITICAL] vcpkg 工具链设置必须在 project() 之前！
# ============================================================================
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
    else()
        message(WARNING "VCPKG_ROOT environment variable not set. Vcpkg integration might fail.")
    endif()
endif()

# 设置vcpkg为静态链接模式
set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "vcpkg triplet")

# ============================================================================
# 项目定义
# ============================================================================
project(DownloadIntegrator LANGUAGES CXX)

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/icon.rc")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 建议：尽量不要硬编码 Qt 路径，尽量通过环境变量 CMAKE_PREFIX_PATH 传入
# set(CMAKE_PREFIX_PATH "D:/Qt/6.10.0/msvc2022_64") 

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 MSVC 运行时库为静态 (MT/MTd)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# ============================================================================
# 查找依赖库 (通过vcpkg清单模式自动安装)
# ============================================================================
# [Fix] 显式查找 ZLIB，防止 CURL 静态链接缺符号
find_package(ZLIB REQUIRED) 
# [Fix] 使用 CONFIG 模式查找 CURL
find_package(CURL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)

# Qt6 组件
find_package(Qt6 COMPONENTS Network Core Gui Concurrent Quick Qml QuickControls2 REQUIRED)

# ============================================================================
# 源文件与头文件
# ============================================================================
set(SOURCES
    src/main.cpp
    src/ConfigManager.cpp
    src/ModifierParser.cpp
    src/NetworkManager.cpp
    src/ModifierManager.cpp
    src/FileSystem.cpp
    src/UpdateManager.cpp
    src/DownloadManager.cpp
    src/ModifierInfoManager.cpp
    src/SearchManager.cpp
    src/LanguageManager.cpp
    src/GameMappingManager.cpp
    src/translator.cpp
    src/CoverExtractor.cpp
    src/Backend.cpp
    src/ModifierListModel.cpp
    src/DownloadedModifierModel.cpp
)

set(HEADERS
    src/ConfigManager.h
    src/ModifierParser.h
    src/NetworkManager.h
    src/ModifierManager.h
    src/FileSystem.h
    src/UpdateManager.h
    src/DownloadManager.h
    src/ModifierInfoManager.h
    src/SearchManager.h
    src/LanguageManager.h
    src/GameMappingManager.h
    src/translator.h
    src/CoverExtractor.h
    src/Backend.h
    src/ModifierListModel.h
    src/DownloadedModifierModel.h
)

set(RESOURCE_FILES
    resources/resources.qrc
    qml/qml.qrc
)

# Add build directory to include path
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Specify MSVC UTF-8 encoding   
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(${PROJECT_NAME}
    WIN32 # 如果你需要发布正式版并隐藏黑框，请取消注释这一行
    ${app_icon_resource_windows}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCE_FILES}
) 

# ============================================================================
# 链接库
# ============================================================================
target_link_libraries(${PROJECT_NAME} PRIVATE 
    # Qt6 组件
    Qt6::Network
    Qt6::Core
    Qt6::Gui      # [Added] 加上这个比较稳妥
    Qt6::Concurrent
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    # vcpkg 依赖
    pugixml::pugixml
    CURL::libcurl
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS} # OpenCV 有时候也可以用 opencv_core 等具体 target
)

# CURL静态链接宏
target_compile_definitions(${PROJECT_NAME} PRIVATE CURL_STATICLIB)

# ============================================================================
# MSVC 特定配置 (安全与优化)
# ============================================================================
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
    
    # 静态链接所有库
    target_link_options(${PROJECT_NAME} PRIVATE 
        /INCREMENTAL:NO
        /LTCG
    )
    
    # 启用静态链接优化
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/GL>  # 全程序优化
        $<$<CONFIG:Release>:/Oi>  # 内联函数优化
    )
    
    # 安全编译选项
    target_compile_options(${PROJECT_NAME} PRIVATE
        /GS          # 栈保护
        /guard:cf    # 控制流保护
    )
    
    # 安全链接选项
    target_link_options(${PROJECT_NAME} PRIVATE
        /DYNAMICBASE    # ASLR
        /NXCOMPAT       # DEP
        /LARGEADDRESSAWARE
    )
    
    # 添加额外的系统库，确保静态链接完整性
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        ws2_32.lib 
        wldap32.lib 
        crypt32.lib 
        normaliz.lib
        User32.lib   # [Added] 基础系统库
        Gdi32.lib    # [Added] 基础图形库
        Advapi32.lib # [Added] 注册表与安全库
    )
endif()