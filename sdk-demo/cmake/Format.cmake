# 代码格式化配置

# 检查是否已经定义了format目标
if(NOT TARGET format)
    # 查找clang-format
    find_program(CLANG_FORMAT_EXE
        NAMES clang-format clang-format-15 clang-format-14 clang-format-13
        PATHS
            /usr/bin
            /usr/local/bin
            /opt/homebrew/bin
            ${CMAKE_SOURCE_DIR}/tools
    )

    if(CLANG_FORMAT_EXE)
        message(STATUS "Found clang-format: ${CLANG_FORMAT_EXE}")

        # 查找所有需要格式化的文件
        file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
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

        # 过滤掉不需要格式化的文件
        if(ALL_CXX_SOURCE_FILES)
            list(FILTER ALL_CXX_SOURCE_FILES EXCLUDE REGEX ".*/_deps/.*")
            list(FILTER ALL_CXX_SOURCE_FILES EXCLUDE REGEX ".*/build/.*")
            list(FILTER ALL_CXX_SOURCE_FILES EXCLUDE REGEX ".*/third_party/.*")
        endif()

        if(ALL_CXX_SOURCE_FILES)
            # 格式化目标
            add_custom_target(format
                COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_CXX_SOURCE_FILES}
                COMMENT "Formatting source code with clang-format"
                VERBATIM
            )

            # 检查格式目标
            if(NOT TARGET format-check)
                add_custom_target(format-check
                    COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror -style=file ${ALL_CXX_SOURCE_FILES}
                    COMMENT "Checking code format with clang-format"
                    VERBATIM
                )
            endif()

            # 显示需要格式化的文件
            if(NOT TARGET format-diff)
                add_custom_target(format-diff
                    COMMAND ${CLANG_FORMAT_EXE} --dry-run -style=file ${ALL_CXX_SOURCE_FILES}
                    COMMENT "Showing format differences"
                    VERBATIM
                )
            endif()

            # 计算文件数量
            list(LENGTH ALL_CXX_SOURCE_FILES FILE_COUNT)
            message(STATUS "Format targets created: format, format-check, format-diff")
            message(STATUS "Found ${FILE_COUNT} source files to format")
        else()
            message(STATUS "No source files found for formatting")
        endif()

    else()
        message(WARNING "clang-format not found. Format targets will not be available.")

        # 创建空目标以避免构建错误
        add_custom_target(format
            COMMAND ${CMAKE_COMMAND} -E echo "clang-format not available"
            COMMENT "clang-format not found"
        )

        if(NOT TARGET format-check)
            add_custom_target(format-check
                COMMAND ${CMAKE_COMMAND} -E echo "clang-format not available"
                COMMENT "clang-format not found"
            )
        endif()
    endif()
else()
    message(STATUS "Format target already exists, skipping format configuration")
endif()

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
AlwaysBreakAfterDefinitionReturnType: None
AlwaysBreakAfterReturnType: None
AlwaysBreakBeforeMultilineStrings: true
AlwaysBreakTemplateDeclarations: true
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
ForEachMacros: [ foreach, Q_FOREACH, BOOST_FOREACH ]
IncludeCategories:
  - Regex: '^<.*\\.h>'
    Priority: 1
  - Regex: '^<.*'
    Priority: 2
  - Regex: '.*'
    Priority: 3
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
Standard: Cpp11
TabWidth: 8
UseTab: Never
...
")
    message(STATUS "Created .clang-format configuration file")
endif()

message(STATUS "Format configuration completed")
