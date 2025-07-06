# 平台抽象层 - 简化版本

add_library(platform_abstraction STATIC
    file_system.cpp
)

# 设置包含目录
target_include_directories(platform_abstraction
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 平台特定链接库
if(WIN32)
    target_link_libraries(platform_abstraction PRIVATE
        ws2_32 winmm kernel32 user32
    )
elseif(APPLE)
    target_link_libraries(platform_abstraction PRIVATE
        "-framework Foundation"
        "-framework CoreFoundation"
    )
elseif(ANDROID)
    target_link_libraries(platform_abstraction PRIVATE
        log android
    )
else()
    # Linux和其他Unix系统
    target_link_libraries(platform_abstraction PRIVATE
        pthread dl
    )
endif()
