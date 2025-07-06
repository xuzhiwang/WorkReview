#!/bin/bash

# 文档完整性验证脚本

set -e

# 颜色输出
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

print_info "验证文档完整性..."

# 检查主要文档文件
REQUIRED_DOCS=(
    "COMPLETE_DEVELOPMENT_GUIDE.md"
    "DEVELOPMENT_GUIDE_INDEX.md"
    "README.md"
)

for doc in "${REQUIRED_DOCS[@]}"; do
    if [[ -f "$doc" ]]; then
        print_success "✓ $doc 存在"
    else
        print_error "✗ $doc 缺失"
        exit 1
    fi
done

# 检查文档内容完整性
print_info "检查主文档章节完整性..."

REQUIRED_SECTIONS=(
    "代码质量基础理论"
    "静态代码分析"
    "动态代码分析"
    "代码审查方法论"
    "自动化测试策略"
    "持续集成/持续部署"
    "AI辅助开发"
    "代码重构技术"
    "性能优化方法"
    "安全编程实践"
    "文档驱动开发"
    "监控与可观测性"
    "技术债务管理"
    "实战案例分析"
)

MAIN_DOC="COMPLETE_DEVELOPMENT_GUIDE.md"
missing_sections=0

for section in "${REQUIRED_SECTIONS[@]}"; do
    if grep -q "$section" "$MAIN_DOC"; then
        print_success "✓ 章节: $section"
    else
        print_error "✗ 缺失章节: $section"
        ((missing_sections++))
    fi
done

if [[ $missing_sections -eq 0 ]]; then
    print_success "所有必需章节都存在"
else
    print_error "缺失 $missing_sections 个章节"
    exit 1
fi

# 检查代码示例
print_info "检查代码示例..."

code_blocks=$(grep -c '```cpp' "$MAIN_DOC" || echo 0)
python_blocks=$(grep -c '```python' "$MAIN_DOC" || echo 0)
bash_blocks=$(grep -c '```bash' "$MAIN_DOC" || echo 0)
yaml_blocks=$(grep -c '```yaml' "$MAIN_DOC" || echo 0)

print_info "代码示例统计:"
print_info "  C++ 示例: $code_blocks"
print_info "  Python 示例: $python_blocks"
print_info "  Bash 示例: $bash_blocks"
print_info "  YAML 示例: $yaml_blocks"

total_examples=$((code_blocks + python_blocks + bash_blocks + yaml_blocks))
if [[ $total_examples -gt 50 ]]; then
    print_success "代码示例充足 ($total_examples 个)"
else
    print_warning "代码示例较少 ($total_examples 个)"
fi

# 检查链接完整性
print_info "检查内部链接..."

# 提取所有内部链接
internal_links=$(grep -o '\[.*\](\.\/.*\.md[^)]*)' "$MAIN_DOC" || echo "")
broken_links=0

if [[ -n "$internal_links" ]]; then
    while IFS= read -r link; do
        # 提取文件路径
        file_path=$(echo "$link" | sed 's/.*(\.\///' | sed 's/[#)].*$//')
        if [[ -f "$file_path" ]]; then
            print_success "✓ 链接有效: $file_path"
        else
            print_error "✗ 链接失效: $file_path"
            ((broken_links++))
        fi
    done <<< "$internal_links"
else
    print_info "未找到内部链接"
fi

if [[ $broken_links -eq 0 ]]; then
    print_success "所有内部链接都有效"
else
    print_error "发现 $broken_links 个失效链接"
fi

# 检查文档大小
print_info "检查文档大小..."

main_doc_size=$(wc -l < "$MAIN_DOC")
main_doc_words=$(wc -w < "$MAIN_DOC")

print_info "主文档统计:"
print_info "  行数: $main_doc_size"
print_info "  字数: $main_doc_words"

if [[ $main_doc_size -gt 3000 ]]; then
    print_success "文档内容充实 ($main_doc_size 行)"
else
    print_warning "文档内容较少 ($main_doc_size 行)"
fi

# 检查目录结构
print_info "检查目录结构..."

if grep -q "📋 目录" "$MAIN_DOC"; then
    print_success "✓ 包含目录结构"
else
    print_warning "缺少目录结构"
fi

# 生成文档报告
print_info "生成文档报告..."

cat > documentation_report.md << EOF
# 文档验证报告

生成时间: $(date)

## 文档统计

- **主文档行数**: $main_doc_size
- **主文档字数**: $main_doc_words
- **代码示例总数**: $total_examples
  - C++ 示例: $code_blocks
  - Python 示例: $python_blocks
  - Bash 示例: $bash_blocks
  - YAML 示例: $yaml_blocks

## 章节完整性

- **必需章节**: ${#REQUIRED_SECTIONS[@]}
- **缺失章节**: $missing_sections
- **完整性**: $((100 - missing_sections * 100 / ${#REQUIRED_SECTIONS[@]}))%

## 链接检查

- **失效链接**: $broken_links

## 建议

EOF

if [[ $missing_sections -eq 0 && $broken_links -eq 0 && $total_examples -gt 50 ]]; then
    echo "- 文档质量优秀，无需改进" >> documentation_report.md
    print_success "文档验证通过！"
else
    echo "- 建议补充缺失章节" >> documentation_report.md
    echo "- 建议修复失效链接" >> documentation_report.md
    echo "- 建议增加更多代码示例" >> documentation_report.md
    print_warning "文档需要改进"
fi

print_info "文档报告已生成: documentation_report.md"

# 检查是否有markdown语法错误
if command -v markdownlint &> /dev/null; then
    print_info "运行Markdown语法检查..."
    if markdownlint "$MAIN_DOC"; then
        print_success "Markdown语法检查通过"
    else
        print_warning "发现Markdown语法问题"
    fi
else
    print_info "未安装markdownlint，跳过语法检查"
fi

print_success "文档验证完成！"
