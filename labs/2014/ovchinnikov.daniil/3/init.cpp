#include <iostream>
#include "fs.h"

int main (const int argc, const char *argv[]) try {
    if (argc < 2) {
        std::cout << "Usage: init root" << std::endl;
        return 0;
    } else {
        FS(argv[1]).init();
    }
    return 0;
} catch (const char * msg) {
    std::cerr << msg << std::endl;
    return 1;
}
