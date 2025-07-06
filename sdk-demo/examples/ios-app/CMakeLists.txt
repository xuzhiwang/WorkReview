# iOS测试App

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # iOS App Bundle
    add_executable(SDKTestApp MACOSX_BUNDLE
        main.m
        AppDelegate.h
        AppDelegate.m
        ViewController.h
        ViewController.m
        SDKWrapper.h
        SDKWrapper.mm
    )
    
    # 链接SDK
    target_link_libraries(SDKTestApp
        PRIVATE
            ${PROJECT_NAME}
    )
    
    # iOS特定设置
    set_target_properties(SDKTestApp PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.crossplatformsdk.testapp"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"  # iPhone and iPad
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "14.0"
        XCODE_ATTRIBUTE_ENABLE_BITCODE NO
    )
    
    # 添加框架依赖
    target_link_libraries(SDKTestApp
        PRIVATE
            "-framework UIKit"
            "-framework Foundation"
            "-framework CoreFoundation"
    )
    
    # 设置输出目录
    set_target_properties(SDKTestApp PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/ios-app
    )
    
    # 复制资源文件
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LaunchScreen.storyboard")
        target_sources(SDKTestApp PRIVATE LaunchScreen.storyboard)
        set_source_files_properties(LaunchScreen.storyboard PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources
        )
    endif()
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Main.storyboard")
        target_sources(SDKTestApp PRIVATE Main.storyboard)
        set_source_files_properties(Main.storyboard PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources
        )
    endif()
    
    message(STATUS "iOS Test App configured: SDKTestApp")
else()
    message(STATUS "Skipping iOS App (not building for iOS)")
endif()
