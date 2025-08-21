// -*- indent-tabs-mode: nil; tab-width: 4 -*-
#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

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
    auto env = getenv("XDG_DATA_DIRS");
    if (env != nullptr) {
        dirs = split_string(env, ':');
    }
    dirs.push_back("/usr/share");
    string dict_file;
    for (auto dir : dirs) {
        if (filesystem::exists(dir) && 
            filesystem::exists(dir.append("/bash-pinyin-completion/char-pinyin.txt"))) {
            dict_file = dir;
            break;
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
    in.close();
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

string run_command(string cmd) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <pinyin> <compgen option>" << endl;
        return 0;
    }
    read_dict();
    auto input = string(argv[1]);
    if (input.front() == '~') {
        auto home = getenv("HOME");
        input.erase(input.begin());
        input = home + input;
    }
    auto split = split_string(input, '/');
    string final;
    if (input.back() == '/') {
        final = "";
    } else {
        final = split.back();
        split.pop_back();
    }
    auto pinyin = string_pinyin(final);
    auto compgen_opts = string(argv[2]);
    // Navigate to the dir we need to complete
    auto dest = filesystem::path(input.c_str()).parent_path();
    if (!dest.empty()) {
        if (!filesystem::exists(dest) || !filesystem::is_directory(dest)) {
            return 0;
        }
        try {
            filesystem::current_path(dest);
        } catch (filesystem::filesystem_error const& code) {
            return 0;
        }
    }
    // Call compgen at that dir
    auto compreply = split_string(run_command("/usr/bin/env bash -c \"compgen " + compgen_opts + "\""), '\n');
    for (auto reply : compreply) {
        auto reply_pinyin = string_pinyin(reply);
        if (reply_pinyin == reply) continue;
        if (reply_pinyin.compare(0, pinyin.size(), pinyin) == 0) {
            for (auto substr : split) {
                cout << substr << "/";
            }
            cout << reply << endl;
        }
    }
    return 0;
}
// vi: shiftwidth=4
