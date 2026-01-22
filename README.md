# 📝 File Multi-Replacer - Complete Text Replacement Tool

## 🚀 Overview

**File Multi-Replacer** is a powerful, cross-platform command-line application designed for batch text replacement in files. It supports both literal text replacement and regular expression patterns, with dynamic table selection and bidirectional conversion capabilities.

![Terminal Screenshot](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-blue)

## ✨ Key Features

### 🔄 **Dual Replacement Modes**
- **Literal Replacement**: Simple word-to-word replacement
- **Regex Replacement**: Advanced pattern-based replacement using regular expressions

### 📊 **Dynamic Table System**
- Multiple replacement tables stored in `Tables/` folder
- Bidirectional conversion support (Forward/Reverse)
- Automatic table preview before selection
- Organized categorization of rules

### 🎯 **Smart Processing**
- Line-by-line processing with detailed progress display
- Batch processing of multiple files
- Two processing modes: Detailed (shows all changes) and Quick (shows only file names)
- Automatic report generation for each processed file

### 🌐 **Cross-Platform Compatibility**
- Works on Windows, Linux, and macOS
- Native file/folder opening support for each OS
- Consistent user experience across platforms

### 📈 **Comprehensive Reporting**
- Detailed statistics on replacements
- Separate report files for each processed file
- File tree visualization
- Console preview of results

## 📁 Folder Structure

```
project/
├── input/          # Place your source files here
├── output/         # Processed files and reports go here
├── Tables/         # Replacement tables storage
│   ├── chinese_to_emoji_complete.txt
│   ├── chinese_to_pinyin_complete.txt
│   ├── arabic_pinyin.txt
│   ├── html_remover.txt
│   └── standard_table.txt
└── multi_replacer_main.cpp  # Main program
```

## 🛠️ Installation & Compilation

### **Prerequisites**
- C++17 compatible compiler (GCC, Clang, or MSVC)
- Standard C++ libraries with filesystem support

### **Compilation Commands**

**Linux/macOS:**
```bash
g++ -std=c++17 -o multi_replacer multi_replacer_main.cpp
```

**Windows (MinGW):**
```bash
g++ -std=c++17 -o multi_replacer.exe multi_replacer_main.cpp
```

**Windows (Visual Studio):**
```bash
cl /std:c++17 /EHsc multi_replacer_main.cpp
```

## 📖 Usage Guide

### **1. First Run Setup**
On first run, the program will check for required folders and create them if missing:
- `input/` - for source files
- `output/` - for processed results
- `Tables/` - for replacement tables

### **2. Main Workflow**

#### **Step 1: Choose Replacement Tables**
```
════════════════════════════════════════
        CHOOSE REPLACEMENT TABLES
════════════════════════════════════════

Current selection:
───────────────────
Literal Table: (none)
Regex Table:   (none)

════════════════════════════════════════
Options:
1 - Choose Literal Table
2 - Choose Regex Table
3 - Clear Both Tables
4 - Start Processing
5 - Return to Main Menu
```

#### **Step 2: Select Processing Mode**
```
════════════════════════════════════════
        PROCESSING OPTIONS
════════════════════════════════════════

Show detailed processing progress?
1 - Yes (show line-by-line replacements)
2 - No (only show file names)
```

#### **Step 3: View Results**
```
════════════════════════════════════════
📄 INPUT FILE: "sample.txt"
════════════════════════════════════════
0001 | Original text line 1
0002 | Original text line 2
------------------------------------------------------------

════════════════════════════════════════
📝 PROCESSED: "sample_processed.txt"
════════════════════════════════════════
0001 | Processed text line 1
0002 | Processed text line 2
------------------------------------------------------------
```

### **3. Table File Format**

#### **Literal Tables** (example: `chinese_to_emoji.txt`)
```text
# Comments start with #
# Format: source-->replacement

你-->👉
好-->👍
爱-->❤️
谢谢-->🙏🙏
```

#### **Regex Tables** (must contain "regex" in filename)
```text
# Format: pattern-->replacement

\d+-->[NUMBER]
[A-Za-z]+@[A-Za-z]+\.[A-Za-z]+-->[EMAIL]
http[s]?://[^\s]+-->[URL]
```

### **4. Bidirectional Conversion**

The program supports **bidirectional conversion** for literal tables:

- **Forward**: `你-->👉` (Chinese to Emoji)
- **Reverse**: `👉-->你` (Emoji to Chinese)

When selecting a table, choose the direction that fits your needs.

## 📚 Available Tables

### **Pre-configured Tables**

| Table Name | Description | Direction |
|------------|-------------|-----------|
| `chinese_to_emoji_complete.txt` | 600+ Chinese characters to emoji | Bidirectional |
| `chinese_to_pinyin_complete.txt` | Comprehensive Chinese to Pinyin | Bidirectional |
| `arabic_pinyin.txt` | Arabic transliteration to Arabic script | Bidirectional |
| `html_remover.txt` | Remove HTML tags | Forward |
| `standard_table.txt` | Basic replacement examples | Bidirectional |

### **Creating Custom Tables**

1. Create a `.txt` file in the `Tables/` folder
2. Add replacement rules in format: `source-->target`
3. Use `#` for comments
4. For regex tables, include "regex" in the filename

## 🔧 Advanced Features

### **Multiple File Selection**
- Select/deselect individual files
- Select all files with option "0"
- Queue management system

### **Post-Processing Options**
After processing, access:
1. **View file contents** on console
2. **Open processed files** in default editor
3. **Open report files** in default editor
4. **Open output folder**
5. **Return to main menu**

### **Smart Rule Ordering**
The program automatically processes longer patterns first to prevent partial matches, ensuring accurate replacements.

## 📊 Output Files

For each input file, the program generates:

1. **`filename_processed.txt`** - The processed content
2. **`filename_report.txt`** - Detailed processing report including:
   - Original and processed file info
   - Replacement statistics
   - Rule-by-rule count
   - Summary of changes

## 🎨 Use Cases

### **1. Language Conversion**
```bash
# Convert Chinese text to Pinyin
Table: chinese_to_pinyin_complete.txt
Direction: Forward
```

### **2. Emoji Conversion**
```bash
# Convert Chinese text to emoji
Table: chinese_to_emoji_complete.txt
Direction: Forward
```

### **3. Data Cleaning**
```bash
# Remove HTML tags
Table: html_remover.txt
Direction: Forward
```

### **4. Pattern Replacement**
```bash
# Replace patterns with regex
Table: standard_regex_table.txt
Direction: Forward
```

## 📝 Example Workflows

### **Workflow 1: Chinese to Emoji Conversion**
```
1. Place Chinese text files in input/
2. Choose "chinese_to_emoji_complete.txt" as Literal Table
3. Select Forward direction
4. Choose files to process
5. View results in output/ folder
```

### **Workflow 2: Pinyin to Chinese Conversion**
```
1. Place Pinyin text files in input/
2. Choose "chinese_to_pinyin_complete.txt" as Literal Table
3. Select Reverse direction
4. Process files
5. Check converted Chinese text
```

## ⚙️ Configuration Tips

### **Optimal Table Design**
- Place longer patterns before shorter ones
- Group related rules with comments
- Test tables with small samples first
- Use bidirectional rules when applicable

### **Performance Considerations**
- Use Quick mode for large files
- Limit detailed display to files under 1000 lines
- Regular expressions are processed before literal replacements

## 🐛 Troubleshooting

### **Common Issues**

1. **No files in input folder**
   - Ensure files are placed in `input/` folder
   - Check file extensions are supported

2. **Rules not working**
   - Verify table format: `source-->target`
   - Check for extra spaces
   - Ensure correct table selection

3. **Partial replacements**
   - Reorder rules with longer patterns first
   - Check for overlapping patterns

4. **Compilation errors**
   - Ensure C++17 compatibility
   - Check compiler flags

### **Debug Mode**
For detailed debugging:
1. Select Detailed processing mode
2. Review line-by-line output
3. Check generated report files

## 🔄 Updating Tables

To add new replacement tables:
1. Create new `.txt` file in `Tables/` folder
2. Follow the format: `source-->target`
3. Use comments (`#`) for documentation
4. Restart program or return to main menu

## 📈 Statistics Display

The program shows:
- Total lines processed
- Total replacements made
- Breakdown by rule type
- File size information
- Processing time indicators

## 🤝 Contributing

To contribute new tables:
1. Follow the table format guidelines
2. Include comprehensive comments
3. Test with sample files
4. Consider bidirectional use cases

## 📄 License

This tool is provided for educational and practical use. Modify and distribute as needed.

## 💡 Pro Tips

1. **Backup Files**: Always keep backups of original files
2. **Test First**: Process one file first to verify results
3. **Table Previews**: Always preview tables before selection
4. **Report Review**: Check report files for statistics
5. **Bidirectional Use**: Leverage reverse direction for different conversion needs

## 🎯 Quick Start Commands

```bash
# Compile
g++ -std=c++17 -o replacer multi_replacer_main.cpp

# Run
./replacer  # Linux/macOS
replacer.exe  # Windows

# Basic workflow
1. Add files to input/ folder
2. Add tables to Tables/ folder
3. Run program
4. Choose tables
5. Select files
6. Process and review results
```

---

**🚀 Ready to transform your text files with powerful, flexible replacement rules!**
