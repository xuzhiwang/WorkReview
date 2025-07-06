# 静态代码分析配置

# 查找静态分析工具
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
find_program(CPPCHECK_EXE NAMES "cppcheck")
find_program(CLANG_FORMAT_EXE NAMES "clang-format")

# Clang-Tidy配置
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
    )
    
    string(REPLACE ";" "," CLANG_TIDY_CHECKS_STR "${CLANG_TIDY_CHECKS}")
    
    set(CMAKE_CXX_CLANG_TIDY 
        ${CLANG_TIDY_EXE}
        -checks=${CLANG_TIDY_CHECKS_STR}
        -header-filter=.*
        -warnings-as-errors=*
    )
    
    # 创建clang-tidy目标
    add_custom_target(clang-tidy
        COMMAND ${CLANG_TIDY_EXE}
        -checks=${CLANG_TIDY_CHECKS_STR}
        -header-filter=.*
        -p ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*/*.cpp
        COMMENT "Running clang-tidy"
    )
else()
    message(WARNING "clang-tidy not found")
endif()

# Cppcheck配置
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
    )
    
    add_custom_target(cppcheck
        COMMAND ${CPPCHECK_EXE}
        ${CPPCHECK_ARGS}
        ${CMAKE_SOURCE_DIR}/src/
        COMMENT "Running cppcheck"
    )
else()
    message(WARNING "cppcheck not found")
endif()

# Clang-Format配置
if(CLANG_FORMAT_EXE)
    message(STATUS "Found clang-format: ${CLANG_FORMAT_EXE}")
    
    # 查找所有源文件
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.h
        ${CMAKE_SOURCE_DIR}/include/*.h
        ${CMAKE_SOURCE_DIR}/examples/*.cpp
        ${CMAKE_SOURCE_DIR}/tests/*.cpp
    )
    
    # 格式化目标
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE}
        -i
        -style=file
        ${ALL_SOURCE_FILES}
        COMMENT "Running clang-format"
    )
    
    # 检查格式目标
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE}
        --dry-run
        --Werror
        -style=file
        ${ALL_SOURCE_FILES}
        COMMENT "Checking code format"
    )
else()
    message(WARNING "clang-format not found")
endif()

# 创建.clang-format配置文件
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/.clang-format)
    file(WRITE ${CMAKE_SOURCE_DIR}/.clang-format
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
  AfterObjCDeclaration: false
  AfterStruct: false
  AfterUnion: false
  BeforeCatch: false
  BeforeElse: false
  IndentBraces: false
BreakBeforeBinaryOperators: None
BreakBeforeBraces: Attach
BreakBeforeTernaryOperators: true
BreakConstructorInitializersBeforeComma: false
BreakAfterJavaFieldAnnotations: false
BreakStringLiterals: true
ConstructorInitializerAllOnOneLineOrOnePerLine: true
ConstructorInitializerIndentWidth: 4
ContinuationIndentWidth: 4
Cpp11BracedListStyle: true
DerivePointerAlignment: true
DisableFormat: false
ExperimentalAutoDetectBinPacking: false
ForEachMacros: [ foreach, Q_FOREACH, BOOST_FOREACH ]
IncludeCategories:
  - Regex: '^<.*\\.h>'
    Priority: 1
  - Regex: '^<.*'
    Priority: 2
  - Regex: '.*'
    Priority: 3
IncludeIsMainRegex: '([-_](test|unittest))?$'
IndentCaseLabels: true
IndentWrappedFunctionNames: false
KeepEmptyLinesAtTheStartOfBlocks: false
MacroBlockBegin: ''
MacroBlockEnd: ''
MaxEmptyLinesToKeep: 1
NamespaceIndentation: None
ObjCBlockIndentWidth: 2
ObjCSpaceAfterProperty: false
ObjCSpaceBeforeProtocolList: false
PenaltyBreakBeforeFirstCallParameter: 1
PenaltyBreakComment: 300
PenaltyBreakFirstLessLess: 120
PenaltyBreakString: 1000
PenaltyExcessCharacter: 1000000
PenaltyReturnTypeOnItsOwnLine: 200
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
endif()

# 创建.clang-tidy配置文件
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/.clang-tidy)
    file(WRITE ${CMAKE_SOURCE_DIR}/.clang-tidy
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

CheckOptions:
  - key: readability-identifier-naming.NamespaceCase
    value: lower_case
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.StructCase
    value: CamelCase
  - key: readability-identifier-naming.FunctionCase
    value: camelBack
  - key: readability-identifier-naming.VariableCase
    value: lower_case
  - key: readability-identifier-naming.PrivateMemberSuffix
    value: _
  - key: readability-identifier-naming.ProtectedMemberSuffix
    value: _
  - key: readability-identifier-naming.MacroDefinitionCase
    value: UPPER_CASE
  - key: readability-identifier-naming.EnumConstantCase
    value: CamelCase
  - key: readability-identifier-naming.ConstexprVariableCase
    value: CamelCase
  - key: readability-identifier-naming.GlobalConstantCase
    value: CamelCase
  - key: readability-identifier-naming.MemberConstantCase
    value: CamelCase
  - key: readability-identifier-naming.StaticConstantCase
    value: CamelCase
...
")
endif()

# 组合静态分析目标
add_custom_target(static-analysis)
if(CLANG_TIDY_EXE)
    add_dependencies(static-analysis clang-tidy)
endif()
if(CPPCHECK_EXE)
    add_dependencies(static-analysis cppcheck)
endif()

message(STATUS "Static analysis tools configured")
message(STATUS "  Available targets: static-analysis, format, format-check")
if(CLANG_TIDY_EXE)
    message(STATUS "  clang-tidy: enabled")
endif()
if(CPPCHECK_EXE)
    message(STATUS "  cppcheck: enabled")
endif()
if(CLANG_FORMAT_EXE)
    message(STATUS "  clang-format: enabled")
endif()
