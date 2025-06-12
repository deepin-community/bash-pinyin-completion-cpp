#include <iostream>
#include <unordered_map>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <string>

using namespace std;

vector<string> split_string(string str, char delim) {
    vector<string> vec;
    stringstream in(str);
    string token;
    while (getline(in, token, delim)) {
        vec.push_back(token);
    }
    return vec;
}

vector<string> read_utf8(string str) {
    vector<string> chars;
    for (size_t i = 0; i < str.size();) {
        unsigned char c = str[i];
        if (c <= 0x7F) { // 1-byte character
            chars.push_back(str.substr(i, 1));
            i += 1;
        } else if ((c >> 5) == 0x06) { // 2-byte character
            chars.push_back(str.substr(i, 2));
            i += 2;
        } else if ((c >> 4) == 0x0E) { // 3-byte character
            chars.push_back(str.substr(i, 3));
            i += 3;
        } else if ((c >> 3) == 0x1E) { // 4-byte character
            chars.push_back(str.substr(i, 4));
            i += 4;
        } else {
            // Invalid UTF-8 sequence, handle error or skip
            i +=1;
        }
    }
    return chars;
}

unordered_map<string, string> dict;

unordered_map<string, string> read_dict() {
    vector<string> dirs;
    auto env = getenv("XDG_DATA_DIR");
    if (env != nullptr) {
        dirs = split_string(env, ':');
    }
    dirs.push_back("/usr/share");
    string dict_file;
    for (auto dir : dirs) {
        if (filesystem::exists(dir) && 
        filesystem::exists(dir.append("/bash-pinyin-completion/char-pinyin.txt"))) {
            dict_file = dir;
        }
    }
    ifstream in(dict_file);
    while (true) {
        string c;
        string pinyin;
        in >> c;
        in >> pinyin;
        dict.emplace(c, pinyin);
        if (in.eof()) break;
        in.get();
        if (in.eof()) break;

    }
    return dict;
}

string string_pinyin(string str) {
    string result = "";
    for (auto c : read_utf8(str)) {
        if (c.length() == 1) {
            result.append(c);
            continue;
        }
        auto pinyin = dict[c];
        if (pinyin.empty()) {
            result.append(c);
        } else {
            result.append(pinyin);
        }
    }
    return result;
}

int main(int argc, char **argv) {
    read_dict();
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <pinyin>" << endl;
        return 0;
    }
    auto input = string_pinyin(argv[1]);
    while (true){
        string line;
        getline(cin, line);
        if (string_pinyin(line).compare(0, input.size(), input) == 0) {
            cout << line << endl;
        }
        if (cin.eof()) return 0;
    }
    return 0;
}