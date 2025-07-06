# 打包配置

include(CPack)

# 基本包信息
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "CrossPlatform SDK")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern C++ Cross-Platform SDK")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

# 包描述
set(CPACK_PACKAGE_DESCRIPTION "
CrossPlatform SDK is a modern C++ library that provides:
- High-performance thread pool with task management
- HTTP client based on libcurl
- Logging system based on spdlog
- Platform abstraction layer
- Complete C API for maximum compatibility
")

# 联系信息
set(CPACK_PACKAGE_CONTACT "xuzhiwang <1311783245@qq.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/xuzhiwang/WorkReview")

# 许可证
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "MIT License - See project repository for details")
endif()

# README
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# 平台特定配置
if(WIN32)
    # Windows NSIS安装包
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME}")
    set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_MODIFY_PATH ON)
    
    # 安装目录
    set(CPACK_NSIS_INSTALL_ROOT "C:\\Program Files")
    
elseif(APPLE)
    # macOS DMG包
    set(CPACK_GENERATOR "DragNDrop;TGZ")
    set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
    set(CPACK_DMG_FORMAT "UDBZ")
    
    # Bundle配置
    set(CPACK_BUNDLE_NAME "${PROJECT_NAME}")
    set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.icns")
    set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist")
    
else()
    # Linux包
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    
    # DEB包配置
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcurl4, libspdlog-dev")
    
    # RPM包配置
    set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_REQUIRES "libcurl-devel, spdlog-devel")
    
endif()

# 组件配置
set(CPACK_COMPONENTS_ALL Runtime Development Documentation Examples)

# Runtime组件
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Runtime Libraries")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Runtime libraries and executables")
set(CPACK_COMPONENT_RUNTIME_REQUIRED ON)

# Development组件
set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "Development Files")
set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "Header files and development libraries")
set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS Runtime)

# Documentation组件
set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")
set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "API documentation and user guides")

# Examples组件
set(CPACK_COMPONENT_EXAMPLES_DISPLAY_NAME "Examples")
set(CPACK_COMPONENT_EXAMPLES_DESCRIPTION "Example programs and source code")
set(CPACK_COMPONENT_EXAMPLES_DEPENDS Development)

# 高级包配置
set(CPACK_PACKAGE_EXECUTABLES "basic_example;c_api_example")
set(CPACK_CREATE_DESKTOP_LINKS "basic_example")

# 源码包配置
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES
    "/\\.git/"
    "/\\.gitignore"
    "/build/"
    "/\\.vscode/"
    "/\\.idea/"
    "\\.DS_Store"
    ".*~$"
)

# 包文件名
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-Source")

# 输出目录
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")

message(STATUS "Package configuration completed")
message(STATUS "Package generators: ${CPACK_GENERATOR}")
message(STATUS "Package directory: ${CPACK_PACKAGE_DIRECTORY}")
