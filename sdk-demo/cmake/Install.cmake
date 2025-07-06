# 安装配置

# 设置安装目录
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install prefix" FORCE)
endif()

# 安装头文件
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# 安装库文件
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# 安装平台抽象层
if(TARGET platform_abstraction)
    install(TARGETS platform_abstraction
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

# 不导出第三方依赖库，只导出我们自己的目标

# 创建并安装配置文件
include(CMakePackageConfigHelpers)

# 生成版本文件
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# 生成配置文件
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
)

# 安装配置文件
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION lib/cmake/${PROJECT_NAME}
)

# 安装目标文件（只导出我们自己的目标）
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME}
    EXPORT_LINK_INTERFACE_LIBRARIES
)

# 安装示例程序（可选）
if(BUILD_EXAMPLES)
    install(TARGETS basic_example c_api_example
        RUNTIME DESTINATION bin/examples
        OPTIONAL
    )
endif()

# 安装文档（如果存在）
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    install(FILES README.md
        DESTINATION share/doc/${PROJECT_NAME}
    )
endif()

# 创建简单的配置模板
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
"@PACKAGE_INIT@

include(CMakeFindDependencyMacro)

# 查找依赖
find_dependency(Threads REQUIRED)

# 包含目标文件
include(\"\${CMAKE_CURRENT_LIST_DIR}/@PROJECT_NAME@Targets.cmake\")

check_required_components(@PROJECT_NAME@)
")
endif()

message(STATUS "Install configuration completed")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
