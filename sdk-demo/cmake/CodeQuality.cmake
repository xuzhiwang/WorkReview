# 代码质量检查配置 - 统一管理所有代码质量工具

# 防止重复包含
if(DEFINED CODE_QUALITY_CONFIGURED)
    return()
endif()
set(CODE_QUALITY_CONFIGURED TRUE)

message(STATUS "Configuring code quality tools...")

# =============================================================================
# 查找工具
# =============================================================================

# 查找clang-format
find_program(CLANG_FORMAT_EXE 
    NAMES clang-format clang-format-15 clang-format-14 clang-format-13
    PATHS
        /usr/bin
        /usr/local/bin
        /opt/homebrew/bin
        ${CMAKE_SOURCE_DIR}/tools
)

# 查找clang-tidy
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")

# 查找cppcheck
find_program(CPPCHECK_EXE NAMES "cppcheck")

# =============================================================================
# 收集源文件
# =============================================================================

# 查找所有需要检查的源文件
file(GLOB_RECURSE ALL_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.c
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/*.hpp
    ${CMAKE_SOURCE_DIR}/include/*.h
    ${CMAKE_SOURCE_DIR}/include/*.hpp
    ${CMAKE_SOURCE_DIR}/examples/*.cpp
    ${CMAKE_SOURCE_DIR}/examples/*.c
    ${CMAKE_SOURCE_DIR}/tests/*.cpp
    ${CMAKE_SOURCE_DIR}/tests/*.c
    ${CMAKE_SOURCE_DIR}/tests/*.h
)

# 过滤掉不需要检查的文件
if(ALL_SOURCE_FILES)
    list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".*/_deps/.*")
    list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".*/build/.*")
    list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".*/third_party/.*")
    list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".*\\.pb\\.(h|cc)$")  # protobuf生成的文件
endif()

# =============================================================================
# Clang-Format配置
# =============================================================================

if(CLANG_FORMAT_EXE AND ALL_SOURCE_FILES)
    message(STATUS "Found clang-format: ${CLANG_FORMAT_EXE}")
    
    # 计算文件数量
    list(LENGTH ALL_SOURCE_FILES FILE_COUNT)
    
    # 格式化目标
    if(NOT TARGET format)
        add_custom_target(format
            COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_SOURCE_FILES}
            COMMENT "Formatting ${FILE_COUNT} source files with clang-format"
            VERBATIM
        )
    endif()
    
    # 检查格式目标
    if(NOT TARGET format-check)
        add_custom_target(format-check
            COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror -style=file ${ALL_SOURCE_FILES}
            COMMENT "Checking code format for ${FILE_COUNT} files"
            VERBATIM
        )
    endif()
    
    # 显示格式差异
    if(NOT TARGET format-diff)
        add_custom_target(format-diff
            COMMAND ${CLANG_FORMAT_EXE} --dry-run -style=file ${ALL_SOURCE_FILES}
            COMMENT "Showing format differences"
            VERBATIM
        )
    endif()
    
    set(FORMAT_AVAILABLE TRUE)
    message(STATUS "Format targets available: format, format-check, format-diff (${FILE_COUNT} files)")
    
elseif(NOT CLANG_FORMAT_EXE)
    message(WARNING "clang-format not found")
    set(FORMAT_AVAILABLE FALSE)
else()
    message(STATUS "No source files found for formatting")
    set(FORMAT_AVAILABLE FALSE)
endif()

# =============================================================================
# Clang-Tidy配置
# =============================================================================

if(CLANG_TIDY_EXE)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
    
    set(CLANG_TIDY_CHECKS
        "-*"
        "readability-*"
        "performance-*"
        "modernize-*"
        "bugprone-*"
        "clang-analyzer-*"
        "cppcoreguidelines-*"
        "-modernize-use-trailing-return-type"
        "-readability-magic-numbers"
        "-cppcoreguidelines-avoid-magic-numbers"
        "-cppcoreguidelines-pro-bounds-pointer-arithmetic"
        "-cppcoreguidelines-pro-type-reinterpret-cast"
    )
    
    string(REPLACE ";" "," CLANG_TIDY_CHECKS_STR "${CLANG_TIDY_CHECKS}")
    
    # 设置clang-tidy为编译时检查
    set(CMAKE_CXX_CLANG_TIDY 
        ${CLANG_TIDY_EXE}
        -checks=${CLANG_TIDY_CHECKS_STR}
        -header-filter=.*
    )
    
    # 创建clang-tidy目标
    if(NOT TARGET clang-tidy)
        add_custom_target(clang-tidy
            COMMAND ${CLANG_TIDY_EXE}
            -checks=${CLANG_TIDY_CHECKS_STR}
            -header-filter=.*
            -p ${CMAKE_BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/src/*.cpp
            ${CMAKE_SOURCE_DIR}/src/*/*.cpp
            COMMENT "Running clang-tidy static analysis"
        )
    endif()
    
    set(CLANG_TIDY_AVAILABLE TRUE)
    message(STATUS "Clang-tidy target available: clang-tidy")
    
else()
    message(WARNING "clang-tidy not found")
    set(CLANG_TIDY_AVAILABLE FALSE)
endif()

# =============================================================================
# Cppcheck配置
# =============================================================================

if(CPPCHECK_EXE)
    message(STATUS "Found cppcheck: ${CPPCHECK_EXE}")
    
    set(CPPCHECK_ARGS
        --enable=all
        --std=c++17
        --platform=unix64
        --suppress=missingIncludeSystem
        --suppress=unmatchedSuppression
        --suppress=unusedFunction
        --inline-suppr
        --error-exitcode=1
        --quiet
    )
    
    if(NOT TARGET cppcheck)
        add_custom_target(cppcheck
            COMMAND ${CPPCHECK_EXE}
            ${CPPCHECK_ARGS}
            ${CMAKE_SOURCE_DIR}/src/
            COMMENT "Running cppcheck static analysis"
        )
    endif()
    
    set(CPPCHECK_AVAILABLE TRUE)
    message(STATUS "Cppcheck target available: cppcheck")
    
else()
    message(WARNING "cppcheck not found")
    set(CPPCHECK_AVAILABLE FALSE)
endif()

# =============================================================================
# 组合目标
# =============================================================================

# 静态分析组合目标
if(NOT TARGET static-analysis)
    add_custom_target(static-analysis
        COMMENT "Running all static analysis tools"
    )
    
    if(CLANG_TIDY_AVAILABLE AND TARGET clang-tidy)
        add_dependencies(static-analysis clang-tidy)
    endif()
    
    if(CPPCHECK_AVAILABLE AND TARGET cppcheck)
        add_dependencies(static-analysis cppcheck)
    endif()
endif()

# 代码质量检查组合目标
if(NOT TARGET code-quality)
    add_custom_target(code-quality
        COMMENT "Running all code quality checks"
    )
    
    if(FORMAT_AVAILABLE AND TARGET format-check)
        add_dependencies(code-quality format-check)
    endif()
    
    if(TARGET static-analysis)
        add_dependencies(code-quality static-analysis)
    endif()
endif()

# =============================================================================
# 创建配置文件
# =============================================================================

# 创建.clang-format配置文件（如果不存在）
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/.clang-format")
    file(WRITE "${CMAKE_SOURCE_DIR}/.clang-format"
"---
Language: Cpp
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
AccessModifierOffset: -2
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: false
AlignConsecutiveDeclarations: false
AlignOperands: true
AlignTrailingComments: true
AllowAllParametersOfDeclarationOnNextLine: true
AllowShortBlocksOnASingleLine: false
AllowShortCaseLabelsOnASingleLine: false
AllowShortFunctionsOnASingleLine: All
AllowShortIfStatementsOnASingleLine: true
AllowShortLoopsOnASingleLine: true
BinPackArguments: true
BinPackParameters: true
BraceWrapping:
  AfterClass: false
  AfterControlStatement: false
  AfterEnum: false
  AfterFunction: false
  AfterNamespace: false
  AfterStruct: false
  AfterUnion: false
  BeforeCatch: false
  BeforeElse: false
  IndentBraces: false
BreakBeforeBinaryOperators: None
BreakBeforeBraces: Attach
BreakBeforeTernaryOperators: true
BreakConstructorInitializersBeforeComma: false
BreakStringLiterals: true
ConstructorInitializerAllOnOneLineOrOnePerLine: true
ConstructorInitializerIndentWidth: 4
ContinuationIndentWidth: 4
Cpp11BracedListStyle: true
DerivePointerAlignment: true
DisableFormat: false
IndentCaseLabels: true
IndentWrappedFunctionNames: false
KeepEmptyLinesAtTheStartOfBlocks: false
MaxEmptyLinesToKeep: 1
NamespaceIndentation: None
PointerAlignment: Left
ReflowComments: true
SortIncludes: true
SpaceAfterCStyleCast: false
SpaceAfterTemplateKeyword: true
SpaceBeforeAssignmentOperators: true
SpaceBeforeParens: ControlStatements
SpaceInEmptyParentheses: false
SpacesBeforeTrailingComments: 2
SpacesInAngles: false
SpacesInContainerLiterals: true
SpacesInCStyleCastParentheses: false
SpacesInParentheses: false
SpacesInSquareBrackets: false
Standard: Cpp17
TabWidth: 8
UseTab: Never
...
")
    message(STATUS "Created .clang-format configuration file")
endif()

# 创建.clang-tidy配置文件（如果不存在）
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/.clang-tidy")
    file(WRITE "${CMAKE_SOURCE_DIR}/.clang-tidy"
"---
Checks: >
  -*,
  readability-*,
  performance-*,
  modernize-*,
  bugprone-*,
  clang-analyzer-*,
  cppcoreguidelines-*,
  -modernize-use-trailing-return-type,
  -readability-magic-numbers,
  -cppcoreguidelines-avoid-magic-numbers,
  -cppcoreguidelines-pro-bounds-pointer-arithmetic,
  -cppcoreguidelines-pro-type-reinterpret-cast

WarningsAsErrors: ''
HeaderFilterRegex: '.*'
AnalyzeTemporaryDtors: false
FormatStyle: file
...
")
    message(STATUS "Created .clang-tidy configuration file")
endif()

# =============================================================================
# 总结
# =============================================================================

message(STATUS "Code quality tools configuration completed")
message(STATUS "Available targets:")
if(FORMAT_AVAILABLE)
    message(STATUS "  - format: Format all source code")
    message(STATUS "  - format-check: Check code formatting")
    message(STATUS "  - format-diff: Show formatting differences")
endif()
if(CLANG_TIDY_AVAILABLE)
    message(STATUS "  - clang-tidy: Run clang-tidy static analysis")
endif()
if(CPPCHECK_AVAILABLE)
    message(STATUS "  - cppcheck: Run cppcheck static analysis")
endif()
if(TARGET static-analysis)
    message(STATUS "  - static-analysis: Run all static analysis tools")
endif()
if(TARGET code-quality)
    message(STATUS "  - code-quality: Run all code quality checks")
endif()
