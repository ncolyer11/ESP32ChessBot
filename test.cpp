#include <iostream>
#include <string>

int main() {
    std::string line;
    if (!std::getline(std::cin, line)) return 0;
    // strip UTF-8 BOM
    if (line.size() >= 3
     && (unsigned char)line[0] == 0xEF
     && (unsigned char)line[1] == 0xBB
     && (unsigned char)line[2] == 0xBF)
        line.erase(0,3);
    // strip CR
    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    std::cout << "Cleaned: [" << line << "] (" << line.size() << " bytes)\n";
    return 0;
}
