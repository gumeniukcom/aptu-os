#include "FS/filesystem.hpp"

int main(int argc, char *argv[])
{
    if(argc != 4) {
        std::cout << "usage: import <root path> <host_file> <fs_file>" << std::endl;
        return 1;
    }

    FileSystem fs(argv[1]);
    
    fs.load();

    if(fs.importFile(argv[2], argv[3]) == 0) {
        std::cout << "error import" << std::endl;
        return 1;
    }
    
    fs.save();

    return 0;
}
