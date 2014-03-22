#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include <iostream>

#include <string>
#include <fstream>
#include <exception>
#include <vector>
#include <iterator>
using namespace std;

#include "types.hpp"
#include "directory.hpp"

typedef vector<char> bitmap;

class ofilebuf;

class filesystem {

public:
    explicit filesystem(const string &path,
            bool try_read_metadata = true);

    ~filesystem()
    { write_metadata(); write_bitmap(); }

    bool is_valid() const
        { return m_block_size >= 1024 && m_blocks_count > 0; }
    bool is_formatted() const
        { return is_valid() && m_root.is_valid(); }

    void init();
    void format();
    void import(const string &from_path, const string &to_path);
    //void export(const string &to_path, const string &from_path);
    string ls(const string &path);

private:
    friend class ofilebuf;

    const string CONFIG_FILENAME   = "config";
    const string METADATA_FILENAME = "metadata";
    const string BITMAP_FILENAME   = "bitmap";

    static const block_num NO_FREE_BLOCKS = -1;
    static const char FREE = 0x00;
    static const char USED = 0x01;

    string m_path;
    bytes m_block_size;
    size_t m_blocks_count;

    directory m_root;
    bitmap m_bitmap;

    bytes block_size() const
        { return m_block_size; }
    string block_path(block_num n)
        { return m_path + to_string(n); }

    block_num next_free_block_num() const;
    void mark_block(block_num n, char t)
        { m_bitmap[n] = t; }

    directory *find_last(vector<string> path);

    void read_config();
    void read_metadata();
    void read_bitmap();

    void write_metadata();
    void write_bitmap();

public:
    struct error: runtime_error
    { explicit error(const string &m) : runtime_error(m) {} };
};

#endif /* end of include guard: FILESYSTEM_HPP */
