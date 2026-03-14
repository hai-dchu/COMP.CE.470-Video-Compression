#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <map>
#include <iomanip>
#include <cctype>
#include <cstdint>
#include <vector>
#include <algorithm>

void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " <task> [arguments...]\n"
              << "Tasks:\n"
              << "  count <file_path> (<top_n>)   Count occurrences of each character in the file.\n"
              << "       <top_n> (optional) - Number of top characters to display (default: 10, 0 for all).\n"
              << "  pairs <file_path> (<top_n>)  Count occurrences of character pairs in the file.\n"
              << "       <top_n> (optional) - Number of top pairs to display (default: 10, 0 for all).\n";
}

int count_chars_in_file(const std::string& filepath, size_t top_n = 10) {
    std::ifstream file(filepath, std::ios::binary); // Binary mode to preserve exact byte content

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filepath << "'\n";
        return 1;
    }

    // Using a map to accumulate counts by character value
    std::map<unsigned char, size_t> frequency_map;
    char c;

    // Read character by character, including whitespace
    while (file.get(c)) {
        frequency_map[static_cast<unsigned char>(c)]++;
    }

    if (frequency_map.empty()) {
        std::cout << "File is empty.\n";
        return 0;
    }

    // Helper to produce a printable representation for a single byte
    auto repr = [](unsigned char ch) -> std::string {
        if (std::isprint(ch) && ch != ' ') return std::string(1, static_cast<char>(ch));
        switch (ch) {
            case ' ':  return "[SPACE]";
            case '\t': return "[TAB]";
            case '\n': return "[LF]";
            case '\r': return "[CR]";
            default:   return "[NON-PR]";
        }
    };

    // Move map into a vector and sort by count descending
    std::vector<std::pair<unsigned char, size_t>> vec;
    vec.reserve(frequency_map.size());
    for (const auto& kv : frequency_map) vec.emplace_back(kv.first, kv.second);

    std::sort(vec.begin(), vec.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    size_t to_show = (top_n == 0) ? vec.size() : std::min(top_n, vec.size());

    std::cout << "Character Frequency Report for: " << filepath << "\n";
    std::cout << std::string(40, '-') << "\n";
    std::cout << std::left << std::setw(10) << "Char" 
              << std::setw(10) << "Hex" 
              << std::setw(15) << "Count" << "\n";
    std::cout << std::string(40, '-') << "\n";

    for (size_t i = 0; i < to_show; ++i) {
        unsigned char key = vec[i].first;
        size_t count = vec[i].second;

        std::cout << std::left << std::setw(10);
        if (std::isprint(key) && key != ' ') {
            std::cout << std::string(1, static_cast<char>(key));
        } else {
            switch (key) {
                case ' ':  std::cout << "[SPACE]"; break;
                case '\t': std::cout << "[TAB]"; break;
                case '\n': std::cout << "[LF]"; break;
                case '\r': std::cout << "[CR]"; break;
                default:   std::cout << "[NON-PR]"; break;
            }
        }

        std::cout << "0x" << std::uppercase << std::hex << std::setw(8) << static_cast<int>(key)
                  << std::dec << std::setw(15) << count << "\n";
    }

    return 0;
}

int count_pairs_in_file(const std::string& filepath, size_t top_n = 10) {
    std::ifstream file(filepath, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filepath << "'\n";
        return 1;
    }

    // Map keyed by a 16-bit value: high byte = previous char, low byte = current char
    std::map<uint16_t, size_t> pairs_map;
    char prev_char;

    // Read first character to seed pairs
    if (!file.get(prev_char)) {
        std::cout << "File is empty.\n";
        return 0;
    }

    char c;
    while (file.get(c)) {
        uint16_t key = (static_cast<uint16_t>(static_cast<unsigned char>(prev_char)) << 8)
                       | static_cast<uint16_t>(static_cast<unsigned char>(c));
        pairs_map[key]++;
        prev_char = c;
    }

    if (pairs_map.empty()) {
        std::cout << "No character pairs found (file has less than 2 bytes).\n";
        return 0;
    }

    // Helper to produce a printable representation for a single byte
    auto repr = [](unsigned char ch) -> std::string {
        if (std::isprint(ch) && ch != ' ') return std::string(1, static_cast<char>(ch));
        switch (ch) {
            case ' ':  return "[SPACE]";
            case '\t': return "[TAB]";
            case '\n': return "[LF]";
            case '\r': return "[CR]";
            default:   return "[NON-PR]";
        }
    };

    std::cout << "Character Pair Frequency Report for: " << filepath << "\n";
    std::cout << std::string(40, '-') << "\n";
    std::cout << std::left << std::setw(25) << "Pair"
              << std::setw(15) << "Count" << "\n";
    std::cout << std::string(40, '-') << "\n";

    // Move map into a vector and sort by count descending
    std::vector<std::pair<uint16_t, size_t>> pairs_vec;
    pairs_vec.reserve(pairs_map.size());
    for (const auto& kv : pairs_map) pairs_vec.emplace_back(kv.first, kv.second);

    std::sort(pairs_vec.begin(), pairs_vec.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    size_t to_show = (top_n == 0) ? pairs_vec.size() : std::min(top_n, pairs_vec.size());

    for (size_t i = 0; i < to_show; ++i) {
        uint16_t key = pairs_vec[i].first;
        size_t count = pairs_vec[i].second;
        unsigned char a = static_cast<unsigned char>(key >> 8);
        unsigned char b = static_cast<unsigned char>(key & 0xFF);
        std::string pair_repr = repr(a) + repr(b);

        std::cout << std::left << std::setw(25) << pair_repr
                  << std::setw(15) << count << "\n";
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string_view task(argv[1]);

    if (task == "count") {
        if (argc < 3) {
            std::cerr << "Error: Missing input file for 'count' task.\n";
            print_usage(argv[0]);
            return 1;
        }
        size_t top_n = 10;
        if (argc >= 4) {
            try {
                unsigned long val = std::stoul(argv[3]);
                top_n = static_cast<size_t>(val);
            } catch (...) {
                std::cerr << "Error: invalid number for top matches: '" << argv[3] << "'\n";
                return 1;
            }
        }
        return count_chars_in_file(argv[2], top_n);
    } else if (task == "pairs") {
        if (argc < 3) {
            std::cerr << "Error: Missing input file for 'pairs' task.\n";
            print_usage(argv[0]);
            return 1;
        }
        size_t top_n = 10;
        if (argc >= 4) {
            try {
                unsigned long val = std::stoul(argv[3]);
                top_n = static_cast<size_t>(val);
            } catch (...) {
                std::cerr << "Error: invalid number for top matches: '" << argv[3] << "'\n";
                return 1;
            }
        }
        return count_pairs_in_file(argv[2], top_n);
    } else {
        std::cerr << "Error: Unknown task '" << task << "'\n";
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}