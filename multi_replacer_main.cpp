#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std;

// -------------------- OS Detection --------------------
#if defined(_WIN32) || defined(_WIN64)
    #define OS_WINDOWS
    #include <conio.h>
    #include <windows.h>
    #define CLEAR_SCREEN() system("cls")
    #define SCROLLABLE_CLEAR() system("cls")
    #define OPEN_FILE(file) ShellExecute(NULL, "open", file.string().c_str(), NULL, NULL, SW_SHOWNORMAL)
    #define OPEN_FOLDER(folder) ShellExecute(NULL, "open", folder.string().c_str(), NULL, NULL, SW_SHOWNORMAL)
    #define SLEEP(ms) Sleep(ms)
#else
    #define OS_LINUX
    #include <termios.h>
    #include <unistd.h>
    #include <cstdlib>
    #define CLEAR_SCREEN() cout << "\033[2J\033[1;1H"
    #define SCROLLABLE_CLEAR() cout << "\033[2J\033[3J\033[1;1H"
    #define OPEN_FILE(file) system(("xdg-open \"" + file.string() + "\" &").c_str())
    #define OPEN_FOLDER(folder) system(("xdg-open \"" + folder.string() + "\" &").c_str())
    #define SLEEP(ms) usleep(ms * 1000)
#endif

// -------------------- Cross-platform getche --------------------
char getche_x() {
#ifdef OS_WINDOWS
    return getche();
#else
    struct termios oldt, newt;
    char c;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    c = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    cout << c;
    return c;
#endif
}

// -------------------- Structures --------------------
struct LiteralRule {
    string old_word;
    string new_word;
    unsigned int occ;
};

struct RegexRule {
    string pattern_str;
    regex pattern;
    string replacement;
    unsigned int occ;
};

// -------------------- File Loader --------------------
vector<fs::path> load_files(const fs::path& dir) {
    vector<fs::path> files;
    if(!fs::exists(dir)) return files;
    for(const auto& entry : fs::directory_iterator(dir))
        if(entry.is_regular_file()) files.push_back(entry.path());
    return files;
}

// -------------------- Queue Helper --------------------
short file_exists(const fs::path& file, const vector<fs::path>& list) {
    for(size_t i = 0; i < list.size(); i++)
        if(list[i] == file) return i;
    return -1;
}

void add_or_remove_file(const fs::path& file, vector<fs::path>& queue) {
    short pos = file_exists(file, queue);
    if(pos != -1) {
        queue.erase(queue.begin() + pos);
    }
    else {
        queue.push_back(file);
    }
}

// -------------------- Check and Create Missing Files --------------------
bool check_and_create_files() {
    vector<string> missing_files;
    
    if(!fs::exists("input")) {
        missing_files.push_back("input folder");
    }
    
    if(!fs::exists("output")) {
        missing_files.push_back("output folder");
    }
    
    if(!fs::exists("Tables")) {
        missing_files.push_back("Tables folder");
    }
    
    if(!missing_files.empty()) {
        SCROLLABLE_CLEAR();
        cout << "════════════════════════════════════════\n";
        cout << "       MISSING FILES/FOLDERS DETECTED\n";
        cout << "════════════════════════════════════════\n\n";
        
        cout << "The following files/folders are missing:\n\n";
        for(const auto& file : missing_files) {
            cout << "✗ " << file << "\n";
        }
        
        cout << "\n════════════════════════════════════════\n";
        cout << "✏️ Create missing files/folders?\n";
        cout << " 1 - Yes\n";
        cout << " 2 - No (Exit)\n\n";
        cout << "Choice: ";
        
        char choice = getche_x();
        cout << endl;
        
        if(choice == '2') {
            cout << "Exiting program. Please create the files manually.\n";
            return false;
        }
        
        for(const auto& file : missing_files) {
            if(file == "Tables folder") {
                if(fs::create_directories("Tables")) {
                    cout << "✓ Created Tables/ folder\n";
                }
            }
            else if(file.find("folder") != string::npos) {
                string folder_name = file.substr(0, file.find(" "));
                if(fs::create_directories(folder_name)) {
                    cout << "✓ Created folder: " << folder_name << "/\n";
                }
            }
        }
        
        cout << "\nAll missing files/folders have been created!\n";
        cout << "You can add your own table files to the Tables/ folder.\n";
        cout << "\nPress Enter to continue...";
        cin.ignore();
        cin.get();
    }
    
    return true;
}

// -------------------- Display Table Preview with Direction --------------------
void display_table_preview(const fs::path& table_path, bool reverse = false) {
    ifstream file(table_path);
    if(!file) {
        cout << "Cannot open table file.\n";
        return;
    }
    
    cout << "\n📋 Table Preview (first 5 rows):\n";
    cout << "────────────────────────────────\n";
    
    string line;
    int line_count = 0;
    while(getline(file, line) && line_count < 5) {
        if(!line.empty() && line[0] != '#') {
            if(reverse) {
                // عرض الاتجاه العكسي
                auto delim = line.find("-->");
                if(delim != string::npos) {
                    string oldw = line.substr(0, delim);
                    string neww = line.substr(delim + 3);
                    cout << "  " << neww << "-->" << oldw << "\n";
                } else {
                    cout << "  " << line << "\n";
                }
            } else {
                // عرض الاتجاه الطبيعي
                cout << "  " << line << "\n";
            }
            line_count++;
        }
    }
    
    if(!file.eof() && line_count >= 5) {
        cout << "  ... (more rows)\n";
    }
    
    cout << "────────────────────────────────\n";
}

// -------------------- Filter Tables --------------------
vector<fs::path> filter_tables(const vector<fs::path>& all_tables, bool literal_mode) {
    vector<fs::path> filtered_tables;
    
    for(const auto& table : all_tables) {
        string filename = table.filename().string();
        transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
        
        bool is_regex_file = (filename.find("regex") != string::npos);
        
        if(literal_mode) {
            // في وضع الجداول الحرفية: نستبعد الملفات التي تحتوي على "regex"
            if(!is_regex_file) {
                filtered_tables.push_back(table);
            }
        } else {
            // في وضع الجداول النمطية: نعرض فقط الملفات التي تحتوي على "regex"
            if(is_regex_file) {
                filtered_tables.push_back(table);
            }
        }
    }
    
    return filtered_tables;
}

// -------------------- Choose Single Table with Direction --------------------
pair<string, bool> choose_single_table(bool literal_mode) {
    fs::path tables_dir = fs::current_path() / "Tables";
    
    if(!fs::exists(tables_dir)) {
        cout << "Tables/ directory not found!\n";
        return make_pair("", false);
    }
    
    vector<fs::path> all_table_files;
    if(fs::exists(tables_dir)) {
        for(const auto& entry : fs::directory_iterator(tables_dir)) {
            if(entry.is_regular_file()) {
                string ext = entry.path().extension().string();
                for(auto& c : ext) c = tolower(c);
                if(ext == ".txt") {
                    all_table_files.push_back(entry.path());
                }
            }
        }
    }
    
    if(all_table_files.empty()) {
        cout << "No table files found in Tables/ folder.\n";
        cout << "Please add .txt files to the Tables/ folder.\n";
        cout << "Press Enter to continue...";
        cin.ignore();
        cin.get();
        return make_pair("", false);
    }
    
    // تصفية الجداول حسب النوع
    vector<fs::path> filtered_tables = filter_tables(all_table_files, literal_mode);
    
    if(filtered_tables.empty()) {
        if(literal_mode) {
            cout << "No literal tables found!\n";
            cout << "Literal tables should not contain 'regex' in their name.\n";
        } else {
            cout << "No regex tables found!\n";
            cout << "Regex tables should contain 'regex' in their name.\n";
        }
        cout << "Press Enter to continue...";
        cin.ignore();
        cin.get();
        return make_pair("", false);
    }
    
    while(true) {
        SCROLLABLE_CLEAR();
        cout << "════════════════════════════════════════\n";
        if(literal_mode) {
            cout << "        CHOOSE LITERAL TABLE\n";
        } else {
            cout << "        CHOOSE REGEX TABLE\n";
        }
        cout << "════════════════════════════════════════\n\n";
        
        cout << "Available tables:\n\n";
        for(size_t i = 0; i < filtered_tables.size(); i++) {
            cout << "    " << i+1 << " - " << filtered_tables[i].filename().string() << "\n";
        }
        
        cout << "\n════════════════════════════════════════\n";
        cout << "Select table (0 for no table, Enter to cancel): ";
        
        string input;
        getline(cin, input);
        
        if(input.empty()) {
            return make_pair("", false); // User cancelled
        }
        
        try {
            int choice = stoi(input);
            
            if(choice == 0) {
                return make_pair("", false); // No table selected
            }
            else if(choice > 0 && choice <= static_cast<int>(filtered_tables.size())) {
                fs::path selected_table = filtered_tables[choice-1];
                
                // عرض معاينة للجدول مع اختيار الاتجاه
                display_table_preview(selected_table, false);
                
                cout << "\nChoose direction:\n";
                cout << "1 - Forward (as shown)\n";
                cout << "2 - Reverse (swap sides)\n\n";
                cout << "Choice: ";
                
                char dir_choice = getche_x();
                cout << endl;
                
                bool reverse_direction = (dir_choice == '2');
                
                if(reverse_direction) {
                    // عرض الاتجاه العكسي للمستخدم
                    cout << "\nReverse direction preview:\n";
                    display_table_preview(selected_table, true);
                }
                
                cout << "\nUse this table?\n";
                cout << "1 - Yes\n";
                cout << "2 - No, choose another\n\n";
                cout << "Choice: ";
                
                char confirm = getche_x();
                cout << endl;
                
                if(confirm == '1') {
                    return make_pair(selected_table.filename().string(), reverse_direction);
                }
            }
        }
        catch(const invalid_argument& e) {
            // تجاهل الإدخال غير الصحيح
        }
    }
}

// -------------------- Choose Tables Menu --------------------
bool choose_tables_menu(string& literal_table, string& regex_table, 
                       bool& literal_reverse, bool& regex_reverse) {
    while(true) {
        SCROLLABLE_CLEAR();
        cout << "════════════════════════════════════════\n";
        cout << "        CHOOSE REPLACEMENT TABLES\n";
        cout << "════════════════════════════════════════\n\n";
        
        cout << "Current selection:\n";
        cout << "───────────────────\n";
        cout << "Literal Table: " << (literal_table.empty() ? "(none)" : literal_table);
        if(!literal_table.empty()) {
            cout << " [" << (literal_reverse ? "REVERSE" : "FORWARD") << "]";
        }
        cout << "\n";
        
        cout << "Regex Table:   " << (regex_table.empty() ? "(none)" : regex_table);
        if(!regex_table.empty()) {
            cout << " [" << (regex_reverse ? "REVERSE" : "FORWARD") << "]";
        }
        cout << "\n";
        
        cout << "\n════════════════════════════════════════\n";
        cout << "Options:\n";
        cout << "1 - Choose Literal Table\n";
        cout << "2 - Choose Regex Table\n";
        cout << "3 - Clear Both Tables\n";
        cout << "4 - Start Processing\n";
        cout << "5 - Return to Main Menu\n\n";
        cout << "Choice: ";
        
        char choice = getche_x();
        cout << endl;
        
        switch(choice) {
            case '1': {
                auto result = choose_single_table(true);
                if(!result.first.empty()) {
                    literal_table = result.first;
                    literal_reverse = result.second;
                }
                break;
            }
                
            case '2': {
                auto result = choose_single_table(false);
                if(!result.first.empty()) {
                    regex_table = result.first;
                    regex_reverse = result.second;
                }
                break;
            }
                
            case '3':
                literal_table.clear();
                regex_table.clear();
                literal_reverse = false;
                regex_reverse = false;
                cout << "Both tables cleared.\n";
                cout << "Press Enter to continue...";
                cin.ignore();
                cin.get();
                break;
                
            case '4':
                if(literal_table.empty() && regex_table.empty()) {
                    cout << "No tables selected! Please select at least one table.\n";
                    cout << "Press Enter to continue...";
                    cin.ignore();
                    cin.get();
                } else {
                    return true; // Start processing
                }
                break;
                
            case '5':
                return false; // Return to main menu
                
            default:
                cout << "Invalid choice!\n";
                cout << "Press Enter to continue...";
                cin.ignore();
                cin.get();
        }
    }
}

// -------------------- Select Files for Processing --------------------
bool select_files_for_processing(vector<fs::path>& queue) {
    vector<fs::path> files = load_files("input");
    
    if(files.empty()) { 
        cout << "No files found in input/ directory.\n";
        cout << "Please add files to the 'input' folder and try again.\n";
        cout << "\nPress any key to continue...";
        getche_x();
        return false;
    }

    queue.clear();
    string input;
    
    while(true) {
        SCROLLABLE_CLEAR();
        cout << "════════════════════════════════════════\n";
        cout << "        SELECT FILES FOR PROCESSING\n";
        cout << "════════════════════════════════════════\n\n";
        
        cout << "Available files in input/ folder:\n\n";
        for(size_t i = 0; i < files.size(); i++) {
            bool inQueue = file_exists(files[i], queue) != -1;
            cout << "    " << i+1 << " - " << files[i].filename().string();
            if(inQueue) cout << " [SELECTED]";
            cout << "\n";
        }

        cout << "\nSelected files (" << queue.size() << " files):\n";
        if(queue.empty()) {
            cout << "    (no files selected)\n";
        } else {
            for(size_t i = 0; i < queue.size(); i++)
                cout << "    " << i+1 << " - " << queue[i].filename().string() << "\n";
        }

        cout << "\nOptions:\n";
        cout << "  Enter number to select/deselect file\n";
        cout << "  0 = select all files\n";
        cout << "  Enter (empty) = confirm and start processing\n\n";
        cout << "Your choice: ";
        
        getline(cin, input);
        
        if(input.empty()) {
            break;
        }
        
        try {
            int sel = stoi(input);
            
            if(sel == 0) {
                queue = files;
                break;
            }
            else if(sel > 0 && sel <= static_cast<int>(files.size())) {
                add_or_remove_file(files[sel-1], queue);
            }
        }
        catch(const invalid_argument& e) {
        }
    }
    
    return !queue.empty();
}

// -------------------- Display Rules in Console --------------------
void display_rules_in_console(const vector<LiteralRule>& literal_rules, 
                             const vector<RegexRule>& regex_rules,
                             const string& literal_table_name,
                             const string& regex_table_name,
                             bool literal_reverse,
                             bool regex_reverse) {
    SCROLLABLE_CLEAR();
    cout << "════════════════════════════════════════\n";
    cout << "           LOADED REPLACEMENT RULES\n";
    cout << "════════════════════════════════════════\n\n";
    
    if(!literal_table_name.empty()) {
        cout << "📋 LITERAL TABLE: " << literal_table_name;
        cout << " [" << (literal_reverse ? "REVERSE" : "FORWARD") << "]\n";
    }
    
    if(!regex_table_name.empty()) {
        cout << "🔍 REGEX TABLE: " << regex_table_name;
        cout << " [" << (regex_reverse ? "REVERSE" : "FORWARD") << "]\n";
    }
    
    cout << "\n";
    
    if(!literal_rules.empty()) {
        cout << "LITERAL REPLACEMENTS:\n";
        cout << "────────────────────────────────────\n";
        int count = 0;
        for(const auto& rule : literal_rules) {
            cout << "  \"" << rule.old_word << "\"  →  \"" << rule.new_word << "\"\n";
            count++;
            if(count >= 15) {
                cout << "  ... (showing first 15 rules)\n";
                break;
            }
        }
        cout << "\n";
    }
    
    if(!regex_rules.empty()) {
        cout << "REGEX REPLACEMENTS:\n";
        cout << "────────────────────────────────────\n";
        int count = 0;
        for(const auto& rule : regex_rules) {
            cout << "  \"" << rule.pattern_str << "\"  →  \"" << rule.replacement << "\"\n";
            count++;
            if(count >= 10) {
                cout << "  ... (showing first 10 rules)\n";
                break;
            }
        }
        cout << "\n";
    }
    
    if(literal_rules.empty() && regex_rules.empty()) {
        cout << "No replacement rules found!\n";
        cout << "Please select tables from the Tables/ folder.\n";
    }
    
    cout << "════════════════════════════════════════\n";
    cout << "Press Enter to continue...";
    cin.ignore();
    cin.get();
}

// -------------------- Load Rules with Direction --------------------
vector<LiteralRule> load_literal_rules(const string& filepath, bool reverse = false) {
    vector<LiteralRule> rules;
    ifstream file(filepath);
    if(!file) {
        cerr << "✗ Warning: Cannot open " << filepath << "\n";
        return rules;
    }
    
    string line;
    string current_comment = "";
    
    while(getline(file, line)) {
        // تجاهل الأسطر الفارغة
        if(line.empty()) continue;
        
        // تخزين التعليقات
        if(line[0] == '#') {
            current_comment = line;
            continue;
        }
        
        // تجاهل أسطر التعليقات التي تحتوي فقط على علامات
        if(line.find("===") != string::npos || line.find("---") != string::npos) {
            continue;
        }
        
        auto delim = line.find("-->");
        if(delim == string::npos) continue;
        
        string oldw = line.substr(0, delim);
        string neww = line.substr(delim + 3);
        
        // إزالة المسافات الزائدة
        while(!oldw.empty() && isspace(oldw.back())) oldw.pop_back();
        while(!oldw.empty() && isspace(oldw.front())) oldw.erase(0, 1);
        while(!neww.empty() && isspace(neww.back())) neww.pop_back();
        while(!neww.empty() && isspace(neww.front())) neww.erase(0, 1);
        
        if(reverse) {
            // عكس الجانبين
            rules.push_back({neww, oldw, 0});
        } else {
            rules.push_back({oldw, neww, 0});
        }
        
        // إعادة تعيين التعليق الحالي
        current_comment = "";
    }
    return rules;
}

vector<RegexRule> load_regex_rules(const string& filepath, bool reverse = false) {
    vector<RegexRule> rules;
    ifstream file(filepath);
    if(!file) {
        cerr << "✗ Warning: Cannot open " << filepath << "\n";
        return rules;
    }
    
    string line;
    while(getline(file, line)) {
        if(line.empty()) continue;
        if(line[0] == '#') continue; // تجاهل التعليقات
        
        auto delim = line.find("-->");
        if(delim == string::npos) continue;
        
        string pattern_str = line.substr(0, delim);
        string replacement = line.substr(delim + 3);
        
        if(reverse) {
            // للتعابير النمطية، لا نعكس عادةً
            // لأن العكس قد لا يكون منطقياً للتعابير النمطية
            cerr << "⚠ Warning: Regex tables don't support reverse direction properly.\n";
            cerr << "  Using forward direction for regex pattern: " << pattern_str << "\n";
        }
        
        try {
            rules.push_back({pattern_str, regex(pattern_str), replacement, 0});
        }
        catch(const regex_error& e) {
            cerr << "Regex error in pattern: " << pattern_str << " - " << e.what() << "\n";
        }
    }
    return rules;
}

// -------------------- Get Base Filename --------------------
string get_base_filename(const fs::path& filepath) {
    string filename = filepath.filename().string();
    size_t dot_pos = filename.find_last_of('.');
    if(dot_pos != string::npos) {
        return filename.substr(0, dot_pos);
    }
    return filename;
}

// -------------------- Process File with Line-by-Line Display --------------------
void process_file_with_display(const fs::path& filepath, 
                              vector<LiteralRule>& literal_rules, 
                              vector<RegexRule>& regex_rules,
                              const fs::path& output_dir,
                              bool show_progress = true) 
{
    ifstream infile(filepath);
    if(!infile) { 
        cerr << "✗ Cannot open " << filepath << endl; 
        return; 
    }

    string base_name = get_base_filename(filepath);
    fs::path outpath = output_dir / (base_name + "_processed.txt");
    fs::path reportpath = output_dir / (base_name + "_report.txt");

    ofstream outfile(outpath);
    ofstream reportfile(reportpath);

    for(auto& r : regex_rules) r.occ = 0;
    for(auto& r : literal_rules) r.occ = 0;

    string line;
    vector<string> original_lines;
    vector<string> processed_lines;
    
    while(getline(infile, line)) {
        if(!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        original_lines.push_back(line);
    }
    infile.close();
    
    if(show_progress) {
        cout << "\n════════════════════════════════════════\n";
        cout << "⚙️ PROCESSING: " << filepath.filename() << "\n";
        cout << "════════════════════════════════════════\n\n";
        cout << "Line-by-line processing:\n";
        cout << "────────────────────────\n";
    }
    
    int line_num = 1;
    int total_replacements_in_file = 0;
    
    for(const auto& orig_line : original_lines) {
        string processed_line = orig_line;
        int replacements_in_line = 0;
        
        for(auto& r : regex_rules) {
            smatch match;
            string temp_line = processed_line;
            size_t offset = 0;
            while(regex_search(temp_line, match, r.pattern)) {
                processed_line.replace(offset + match.position(0), match.length(0), r.replacement);
                offset += match.position(0) + r.replacement.length();
                temp_line = processed_line.substr(offset);
                r.occ++;
                replacements_in_line++;
            }
        }

        for(auto& r : literal_rules) {
            size_t pos = 0;
            while((pos = processed_line.find(r.old_word, pos)) != string::npos) {
                processed_line.replace(pos, r.old_word.length(), r.new_word);
                pos += r.new_word.length();
                r.occ++;
                replacements_in_line++;
            }
        }
        
        processed_lines.push_back(processed_line);
        outfile << processed_line << "\n";
        
        if(show_progress && replacements_in_line > 0) {
            cout << "\nL" << setw(3) << dec << line_num << " ORIGINAL:\n";
            cout << "  ";
            for(char c : orig_line) {
                if(c == '\t') {
                    cout << "\\t";
                } else if(c == '\n') {
                    cout << "\\n";
                } else if(c >= 32 && c <= 126) {
                    cout << c;
                } else {
                    cout << "\\x" << hex << setw(2) << setfill('0') << (int)(unsigned char)c;
                }
            }
            
            cout << "\nL" << setw(3) << dec << line_num << " PROCESSED:\n";
            cout << "  ";
            for(char c : processed_line) {
                if(c == '\t') {
                    cout << "\\t";
                } else if(c == '\n') {
                    cout << "\\n";
                } else if(c >= 32 && c <= 126) {
                    cout << c;
                } else {
                    cout << "\\x" << hex << setw(2) << setfill('0') << (int)(unsigned char)c;
                }
            }
            
            cout << "\n  └─ [" << dec << replacements_in_line << " replacement";
            if(replacements_in_line > 1) cout << "s";
            cout << "]\n";
            cout << string(60, '-') << "\n";
        }
        
        total_replacements_in_file += replacements_in_line;
        line_num++;
    }

    reportfile << "📊 FILE PROCESSING REPORT\n";
    reportfile << string(50, '=') << "\n\n";
    reportfile << "Original File: 📄 " << filepath.filename() << "\n";
    reportfile << "Processed File: 📝 " << outpath.filename() << "\n";
    reportfile << "Report File: 📊 " << reportpath.filename() << "\n";
    reportfile << string(50, '-') << "\n\n";
    
    reportfile << "ORIGINAL FILE INFO:\n";
    reportfile << string(40, '-') << "\n";
    reportfile << "Total lines: 🖩 " << original_lines.size() << "\n";
    reportfile << "Total replacements: 🖩 " << total_replacements_in_file << "\n";
    
    if(!regex_rules.empty()) {
        reportfile << "\nREGEX REPLACEMENTS:\n";
        reportfile << string(40, '-') << "\n";
        bool has_regex = false;
        for(const auto& r : regex_rules) {
            if(r.occ > 0) {
                reportfile << "Pattern:  " << r.pattern_str << "\n";
                reportfile << "Replace:  " << r.replacement << "\n";
                reportfile << "Count:    " << r.occ << "\n";
                reportfile << string(40, '-') << "\n";
                has_regex = true;
            }
        }
        if(!has_regex) {
            reportfile << "No regex replacements applied.\n";
            reportfile << string(40, '-') << "\n";
        }
    }

    if(!literal_rules.empty()) {
        reportfile << "\nLITERAL REPLACEMENTS:\n";
        reportfile << string(40, '-') << "\n";
        bool has_literal = false;
        for(const auto& r : literal_rules) {
            if(r.occ > 0) {
                reportfile << "Find:     " << r.old_word << "\n";
                reportfile << "Replace:  " << r.new_word << "\n";
                reportfile << "Count:    " << r.occ << "\n";
                reportfile << string(40, '-') << "\n";
                has_literal = true;
            }
        }
        if(!has_literal) {
            reportfile << "No literal replacements applied.\n";
            reportfile << string(40, '-') << "\n";
        }
    }
    
    reportfile << "\n" << string(50, '=') << "\n";
    reportfile << "SUMMARY:\n";
    reportfile << "Total replacements: " << (total_replacements_in_file > 0 ? "Applied" : "None") << "\n";
    reportfile << string(50, '=') << "\n";

    if(show_progress) {
        cout << "\n════════════════════════════════════════\n";
        cout << "PROCESSING COMPLETE!\n";
        cout << "════════════════════════════════════════\n";
        
        int regex_total = 0;
        int literal_total = 0;
        
        if(!regex_rules.empty()) {
            bool has_regex = false;
            for(const auto& r : regex_rules) {
                if(r.occ > 0) has_regex = true;
            }
            
            if(has_regex) {
                cout << "\nRegex Replacements Summary:\n";
                cout << "─────────────────────────────\n";
                for(const auto& r : regex_rules) {
                    if(r.occ > 0) {
                        cout << "  FIND:    \"" << r.pattern_str << "\"\n";
                        cout << "  REPLACE: \"" << r.replacement << "\"\n";
                        cout << "  COUNT:   " << r.occ << " time";
                        if(r.occ > 1) cout << "s";
                        cout << "\n";
                        cout << "  ─────────────────────────────\n";
                        regex_total += r.occ;
                    }
                }
            }
        }
        
        if(!literal_rules.empty()) {
            bool has_literal = false;
            for(const auto& r : literal_rules) {
                if(r.occ > 0) has_literal = true;
            }
            
            if(has_literal) {
                cout << "\nLiteral Replacements Summary:\n";
                cout << "──────────────────────────────\n";
                for(const auto& r : literal_rules) {
                    if(r.occ > 0) {
                        cout << "  FIND:    \"" << r.old_word << "\"\n";
                        cout << "  REPLACE: \"" << r.new_word << "\"\n";
                        cout << "  COUNT:   " << r.occ << " time";
                        if(r.occ > 1) cout << "s";
                        cout << "\n";
                        cout << "  ──────────────────────────────\n";
                        literal_total += r.occ;
                    }
                }
            }
        }
        
        cout << "\n════════════════════════════════════════\n";
        cout << "📊 FILE STATISTICS:\n";
        cout << "├── Total lines processed: 🖩 " << original_lines.size() << "\n";
        cout << "├── Total replacements: 🖩 " << total_replacements_in_file << "\n";
        if(regex_total > 0) {
            cout << "│   ├── Regex replacements: 🖩 " << regex_total << "\n";
        }
        if(literal_total > 0) {
            cout << "│   ├── Literal replacements: 🖩 " << literal_total << "\n";
        }
        cout << "├── Output file: 📝 " << outpath.filename() << "\n";
        cout << "└── Report file: 📊 " << reportpath.filename() << "\n";
        cout << "════════════════════════════════════════\n";
        
        cout << "\n════════════════════════════════════════\n";
        cout << "📊 REPORT PREVIEW (first 10 lines):\n";
        cout << "════════════════════════════════════════\n";
        
        ifstream report_preview(reportpath);
        if(report_preview) {
            string report_line;
            int preview_lines = 0;
            while(getline(report_preview, report_line) && preview_lines < 10) {
                cout << report_line << "\n";
                preview_lines++;
            }
            if(!report_preview.eof()) {
                cout << "... (full report in file)\n";
            }
        } else {
            cout << "Could not open report file for preview.\n";
        }
        cout << "════════════════════════════════════════\n\n";
    } else {
        cout << "✓ " << filepath.filename() << " processed (" 
             << total_replacements_in_file << " replacements)\n";
    }
}

// -------------------- Display File Tree --------------------
void display_file_tree(const vector<fs::path>& processed_files,
                      const fs::path& output_dir) {
    SCROLLABLE_CLEAR();
    cout << "════════════════════════════════════════════════════════════\n";
    cout << "                    PROCESSING COMPLETE!\n";
    cout << "════════════════════════════════════════════════════════════\n\n";
    
    cout << "📁 INPUT FOLDER: input/\n";
    for(const auto& file : processed_files) {
        cout << "    ├── 📝 " << file.filename() << "\n";
    }
    
    cout << "\n📁 OUTPUT FOLDER: output/\n";
    
    vector<fs::path> output_files;
    vector<fs::path> report_files;
    
    for(const auto& original_file : processed_files) {
        string base_name = get_base_filename(original_file);
        fs::path processed_file = output_dir / (base_name + "_processed.txt");
        fs::path report_file = output_dir / (base_name + "_report.txt");
        
        if(fs::exists(processed_file)) {
            output_files.push_back(processed_file);
            try {
                auto file_size = fs::file_size(processed_file);
                cout << "    ├── 📝 " << processed_file.filename().string() 
                     << " (" << file_size << " bytes)\n";
            } catch(...) {
                cout << "    ├── 📝 " << processed_file.filename().string() << "\n";
            }
        }
        
        if(fs::exists(report_file)) {
            report_files.push_back(report_file);
            try {
                auto file_size = fs::file_size(report_file);
                cout << "    ├── 📝 " << report_file.filename().string() 
                     << " (" << file_size << " bytes)\n";
            } catch(...) {
                cout << "    ├── " << report_file.filename().string() << "\n";
            }
        }
    }
    
    cout << "\n";
    cout << "📊 SUMMARY:\n";
    cout << "├── Total input files: " << processed_files.size() << "\n";
    cout << "├── Total processed files: " << output_files.size() << "\n";
    cout << "└── Total report files: " << report_files.size() << "\n";
    
    cout << "\n════════════════════════════════════════════════════════════\n\n";
}

// -------------------- Post Processing Menu --------------------
void post_processing_menu(const vector<fs::path>& processed_files,
                         const fs::path& output_dir) {
    char choice;
    do {
        display_file_tree(processed_files, output_dir);
        
        cout << "OPTIONS:\n";
        cout << "🖥️  1 - View file contents on console\n";
        cout << "📄 2 - Open processed files in text editor\n";
        cout << "📊 3 - Open report files in text editor\n";
        cout << "📁 4 - Open output folder\n";
        cout << "↩️  5 - Return to main menu\n";
        cout << "🚪 6 - Exit program\n\n";
        
        cout << "Choice: ";
        choice = getche_x();
        cout << endl;
        
        switch(choice) {
            case '1': {
                SCROLLABLE_CLEAR();
                cout << "════════════════════════════════════════════════════════════\n";
                cout << "                FILE CONTENTS ON CONSOLE\n";
                cout << "════════════════════════════════════════════════════════════\n\n";
                
                for(const auto& original_file : processed_files) {
                    string base_name = get_base_filename(original_file);
                    fs::path processed_file = output_dir / (base_name + "_processed.txt");
                    fs::path report_file = output_dir / (base_name + "_report.txt");
                    
                    cout << "\n════════════════════════════════════════\n";
                    cout << "📄 INPUT FILE: " << original_file.filename() << "\n";
                    cout << "════════════════════════════════════════\n";
                    
                    ifstream orig(original_file);
                    string line;
                    int line_num = 1;
                    while(getline(orig, line) && line_num <= 5) {
                        cout << setw(4) << line_num << " | " << line << "\n";
                        line_num++;
                    }
                    if(!orig.eof()) {
                        cout << "   ... (more lines)\n";
                    }
                    cout << string(60, '-') << "\n";
                    
                    cout << "\n════════════════════════════════════════\n";
                    cout << "📝 PROCESSED: " << processed_file.filename() << "\n";
                    cout << "════════════════════════════════════════\n";
                    
                    ifstream proc(processed_file);
                    line_num = 1;
                    while(getline(proc, line) && line_num <= 5) {
                        cout << setw(4) << line_num << " | " << line << "\n";
                        line_num++;
                    }
                    if(!proc.eof()) {
                        cout << "   ... (more lines)\n";
                    }
                    cout << string(60, '-') << "\n";
                    
                    cout << "\n════════════════════════════════════════\n";
                    cout << "📊 REPORT: " << report_file.filename() << "\n";
                    cout << "════════════════════════════════════════\n";
                    
                    ifstream rep(report_file);
                    line_num = 1;
                    while(getline(rep, line) && line_num <= 10) {
                        cout << setw(4) << line_num << " | " << line << "\n";
                        line_num++;
                    }
                    if(!rep.eof()) {
                        cout << "   ... (more lines)\n";
                    }
                    cout << string(60, '-') << "\n";
                    
                    cout << "\n════════════════════════════════════════════════════════════\n\n";
                }
                
                cout << "\nPress Enter to return to menu...";
                cin.ignore();
                cin.get();
                break;
            }
            
            case '2': {
                vector<fs::path> output_files;
                for(const auto& original_file : processed_files) {
                    string base_name = get_base_filename(original_file);
                    fs::path processed_file = output_dir / (base_name + "_processed.txt");
                    if(fs::exists(processed_file)) {
                        output_files.push_back(processed_file);
                    }
                }
                
                if(output_files.empty()) {
                    cout << "No processed files found.\n";
                } else {
                    cout << "Opening processed files in default text editor...\n";
                    for(const auto& file : output_files) {
                        OPEN_FILE(file);
                        SLEEP(100);
                    }
                }
                cout << "Press Enter to continue...";
                cin.ignore();
                cin.get();
                break;
            }
            
            case '3': {
                vector<fs::path> report_files;
                for(const auto& original_file : processed_files) {
                    string base_name = get_base_filename(original_file);
                    fs::path report_file = output_dir / (base_name + "_report.txt");
                    if(fs::exists(report_file)) {
                        report_files.push_back(report_file);
                    }
                }
                
                if(report_files.empty()) {
                    cout << "No report files found.\n";
                } else {
                    cout << "Opening report files in default text editor...\n";
                    for(const auto& file : report_files) {
                        OPEN_FILE(file);
                        SLEEP(100);
                    }
                }
                cout << "Press Enter to continue...";
                cin.ignore();
                cin.get();
                break;
            }
            
            case '4':
                OPEN_FOLDER(output_dir);
                cout << "Opening output folder...\n";
                cout << "Press Enter to continue...";
                cin.ignore();
                cin.get();
                break;
                
            case '5':
                return;
                
            case '6':
                SCROLLABLE_CLEAR();
                cout << "════════════════════════════════════════\n";
                cout << "           Goodbye! 😉\n";
                cout << "════════════════════════════════════════\n";
                exit(0);
                
            default:
                cout << "Invalid choice!\n";
                cout << "Press Enter to continue...";
                cin.ignore();
                cin.get();
        }
    } while(true);
}

// -------------------- Process Selected Files --------------------
void process_selected_files(const string& literal_table, const string& regex_table,
                          bool literal_reverse, bool regex_reverse,
                          vector<LiteralRule>& literal_rules, vector<RegexRule>& regex_rules) {
    vector<fs::path> queue;
    
    if(!select_files_for_processing(queue)) {
        return;
    }

    SCROLLABLE_CLEAR();
    cout << "════════════════════════════════════════\n";
    cout << "        PROCESSING OPTIONS\n";
    cout << "════════════════════════════════════════\n\n";
    cout << "Show detailed processing progress?\n";
    cout << "1 - Yes (show line-by-line replacements)\n";
    cout << "2 - No (only show file names)\n\n";
    cout << "Choice: ";
    char show_progress_choice = getche_x();
    cout << endl;
    bool show_progress = (show_progress_choice == '1');
    
    SCROLLABLE_CLEAR();
    if(show_progress) {
        cout << "════════════════════════════════════════\n";
        cout << "        DETAILED PROCESSING MODE\n";
        cout << "════════════════════════════════════════\n\n";
        cout << "Processing " << queue.size() << " file(s) with line-by-line display...\n\n";
    } else {
        cout << "════════════════════════════════════════\n";
        cout << "        QUICK PROCESSING MODE\n";
        cout << "════════════════════════════════════════\n\n";
        cout << "Processing " << queue.size() << " file(s)...\n";
        cout << "┌──────────────────────────────────────┐\n";
    }
    
    size_t file_count = 0;
    for(const auto& f : queue) {
        file_count++;
        
        if(!show_progress) {
            cout << "│ File " << setw(2) << file_count << " of " << setw(2) << queue.size();
            cout << " [" << setw(3) << (file_count * 100 / queue.size()) << "%]\n";
            cout << "│ Processing: " << f.filename() << "\n";
            cout << "├──────────────────────────────────────┤\n";
        }
        
        process_file_with_display(f, literal_rules, regex_rules, "output", show_progress);
        
        if(!show_progress && file_count < queue.size()) {
            cout << "├──────────────────────────────────────┤\n";
        }
    }
    
    if(!show_progress) {
        cout << "└──────────────────────────────────────┘\n";
    }
    
    cout << "\nPress Enter to continue to post-processing menu...";
    cin.ignore();
    cin.get();
    
    post_processing_menu(queue, "output");
}

// -------------------- Main --------------------
int main() {
    if(!check_and_create_files()) {
        return 1;
    }
    
    string literal_table;
    string regex_table;
    bool literal_reverse = false;
    bool regex_reverse = false;
    vector<LiteralRule> literal_rules;
    vector<RegexRule> regex_rules;
    
    // بدء البرنامج مباشرة باختيار الجداول
    bool start_processing = choose_tables_menu(literal_table, regex_table, 
                                               literal_reverse, regex_reverse);
    
    // تحميل القواعد من الجداول المختارة
    if(!literal_table.empty()) {
        literal_rules = load_literal_rules("Tables/" + literal_table, literal_reverse);
    }
    if(!regex_table.empty()) {
        regex_rules = load_regex_rules("Tables/" + regex_table, regex_reverse);
    }
    
    // إذا اختار المستخدم "Start Processing" مباشرة
    if(start_processing) {
        process_selected_files(literal_table, regex_table, 
                              literal_reverse, regex_reverse,
                              literal_rules, regex_rules);
    }
    
    while(true) {
        SCROLLABLE_CLEAR();
        cout << "════════════════════════════════════════\n";
        cout << "      FILE MULTI REPLACER\n";
        cout << "════════════════════════════════════════\n\n";
        
        cout << "📋 1 - Choose Tables (Literal & Regex)\n";
        cout << "👁️  2 - View Selected Rules\n";
        cout << "⚙️  3 - Start Processing\n";
        cout << "🚪 4 - Exit\n\n";
        
        cout << "Current tables:\n";
        cout << "────────────────\n";
        cout << "Literal: " << (literal_table.empty() ? "(none)" : literal_table);
        if(!literal_table.empty()) {
            cout << " [" << (literal_reverse ? "REVERSE" : "FORWARD") << "]";
        }
        cout << "\n";
        
        cout << "Regex:   " << (regex_table.empty() ? "(none)" : regex_table);
        if(!regex_table.empty()) {
            cout << " [" << (regex_reverse ? "REVERSE" : "FORWARD") << "]";
        }
        cout << "\n";
        
        cout << "\n════════════════════════════════════════\n";
        cout << "Choice: ";
        char choice = getche_x();
        cout << endl;
        
        switch(choice) {
            case '1': {
                // اختيار الجداول
                bool start_from_menu = choose_tables_menu(literal_table, regex_table,
                                                         literal_reverse, regex_reverse);
                
                // إعادة تحميل القواعد من الجداول المختارة
                literal_rules.clear();
                regex_rules.clear();
                
                if(!literal_table.empty()) {
                    literal_rules = load_literal_rules("Tables/" + literal_table, literal_reverse);
                }
                if(!regex_table.empty()) {
                    regex_rules = load_regex_rules("Tables/" + regex_table, regex_reverse);
                }
                
                // إذا اختار المستخدم "Start Processing" من قائمة الجداول
                if(start_from_menu) {
                    process_selected_files(literal_table, regex_table, 
                                          literal_reverse, regex_reverse,
                                          literal_rules, regex_rules);
                }
                break;
            }
                
            case '2':
                // عرض القواعد الجديدة
                display_rules_in_console(literal_rules, regex_rules, 
                                        literal_table, regex_table,
                                        literal_reverse, regex_reverse);
                break;
                
            case '3':
                if(literal_table.empty() && regex_table.empty()) {
                    cout << "No tables selected! Please select at least one table first.\n";
                    cout << "Press Enter to continue...";
                    cin.ignore();
                    cin.get();
                } else {
                    process_selected_files(literal_table, regex_table, 
                                          literal_reverse, regex_reverse,
                                          literal_rules, regex_rules);
                }
                break;
                
            case '4':
                SCROLLABLE_CLEAR();
                cout << "════════════════════════════════════════\n";
                cout << "           Goodbye! 😉\n";
                cout << "════════════════════════════════════════\n";
                exit(0);
                
            default:
                cout << "Invalid choice!\n";
                cout << "Press Enter to continue...";
                cin.ignore();
                cin.get();
        }
    }

    return 0;
}
