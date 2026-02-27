#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: bench <command> [args...]\n";
        std::cerr << "Commands: record, report\n";
        return 1;
    }

    std::string_view command = argv[1];

    if (command == "record") {
        for (int i = 2; i < argc; ++i) {
            std::cout << argv[i] << "\n";
        }
    } else if (command == "report") {
        for (int i = 2; i < argc; ++i) {
            std::cout << argv[i] << "\n";
        }
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        std::cerr << "Commands: record, report\n";
        return 1;
    }

    return 0;
}
