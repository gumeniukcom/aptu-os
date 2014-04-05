#include <iostream>
#include "fs.h"

using namespace std;

int main (const int argc, const char *argv[]) try {
    if (argc < 4) {
        std::cout << "Usage: import root host_file fs_file" << std::endl;
        return 0;
    } else {
        FS(argv[1]).import(argv[2], argv[3]);
    }
    return 0;
} catch (const char * msg) {
    std::cerr << msg << std::endl;
    return 1;
}
