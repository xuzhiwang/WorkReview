# 平台抽象层

add_library(platform_abstraction STATIC
    file_system.cpp
)

# 平台特定源文件
if(WIN32)
    target_sources(platform_abstraction PRIVATE
        windows/platform_utils_windows.cpp
        windows/file_system_windows.cpp
        windows/network_utils_windows.cpp
        windows/system_info_windows.cpp
    )
    target_link_libraries(platform_abstraction PRIVATE
        ws2_32 winmm kernel32 user32
    )
elseif(APPLE)
    target_sources(platform_abstraction PRIVATE
        apple/platform_utils_apple.cpp
        apple/file_system_apple.cpp
        apple/network_utils_apple.cpp
        apple/system_info_apple.cpp
    )
    if(IOS)
        target_sources(platform_abstraction PRIVATE
            apple/ios_specific.cpp
        )
    else()
        target_sources(platform_abstraction PRIVATE
            apple/macos_specific.cpp
        )
    endif()
    target_link_libraries(platform_abstraction PRIVATE
        "-framework Foundation"
        "-framework CoreFoundation"
    )
elseif(ANDROID)
    target_sources(platform_abstraction PRIVATE
        android/platform_utils_android.cpp
        android/file_system_android.cpp
        android/network_utils_android.cpp
        android/system_info_android.cpp
        android/jni_helpers.cpp
    )
    target_link_libraries(platform_abstraction PRIVATE
        log android
    )
else()
    # Linux和其他Unix系统
    target_sources(platform_abstraction PRIVATE
        unix/platform_utils_unix.cpp
        unix/file_system_unix.cpp
        unix/network_utils_unix.cpp
        unix/system_info_unix.cpp
    )
    target_link_libraries(platform_abstraction PRIVATE
        pthread dl
    )
endif()

target_include_directories(platform_abstraction
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 创建平台特定目录
file(MAKE_DIRECTORY 
    ${CMAKE_CURRENT_SOURCE_DIR}/windows
    ${CMAKE_CURRENT_SOURCE_DIR}/apple
    ${CMAKE_CURRENT_SOURCE_DIR}/android
    ${CMAKE_CURRENT_SOURCE_DIR}/unix
)
