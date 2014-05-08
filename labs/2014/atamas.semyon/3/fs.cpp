#include "fs.h"


#include <math.h>
#include <cstring>
#include <fstream>
#include <stdexcept>

using std::string;

FS::FS(const char *root)
    : root(root),
      config(read_config()),
      blocks(config.block_size){
	if( !std::ifstream(get_block_name(0)).good() ){
		initialized = false;
	} else{
		initialized = true;
	}
}

FS::~FS() {
}

void FS::init(){
	for (int i = 0; i < config.block_no; ++i) {
	    const string block_name = get_block_name(i);
	    std::ofstream out(block_name, std::ios::trunc);
	    out.seekp(config.block_size - 1);
	    out.write("", 1);
	    if(out.bad()){
	    	throw std::runtime_error("Can't allocate blocks");
	    }
	    out.close();
	}
}

void FS::format(){
    if (!initialized) {throw "Not initialized";}
    meta.block_map_size = config.block_no/8 + 1;
    meta.block_map = new char[meta.block_map_size];
    memset(meta.block_map, 0, sizeof(char)*meta.block_map_size);
    int free_space_size = config.block_size - sizeof(BlockDescriptor);
    int no_of_blocks = (sizeof(meta) + meta.block_map_size)/free_space_size + 1;
    meta.root_block = no_of_blocks;
    for(int i = 0; i<no_of_blocks; ++i) reserve_block(i);

    FileDescriptor root;
    root.set_filename("/");
    root.first_block = meta.root_block;
    Block * root_block = 0;
    write_data(&root, sizeof(FileDescriptor), root_block);

    write_meta();
}

void FS::import(std::string host_file, std::string fs_file){
	read_meta();
	FileDescriptor file_d = get_file(fs_file, true, true);
	file_d.directory = false;
	Block * fblock = new Block(file_d.first_block, config.block_size, root);

	FILE * file = fopen(host_file.c_str(), "rb");
	fseek(file, 0, SEEK_END);
	int fileLen=ftell(file);
	file_d.size = fileLen;
	fseek(file, 0, SEEK_SET);
	char * buffer = new char[fileLen];
	if(!fread(buffer, fileLen, 1, file)) throw std::runtime_error("Can't read file");
	fclose(file);

	fblock->move_to_begin();
	write_data(&file_d, sizeof(FileDescriptor), fblock);
	write_data(buffer, fileLen, fblock);
	write_meta();
}

void FS::fexport(std::string fs_file, std::string host_file){
	read_meta();

	FileDescriptor file = get_file(fs_file, false, true);
	if( blocks[file.first_block] == 0 ){
		blocks[file.first_block] = new Block(file.first_block, config.block_size, root);
	}
	char * buffer = new char[file.size];
	read_data(buffer, file.size, blocks[file.first_block]);

	FILE * File = fopen(host_file.c_str(), "wb");
	fwrite(buffer, sizeof(char), file.size, File );
}


void FS::copy(std::string src, std::string dest){
	read_meta();
	FileDescriptor src_file = get_file(src, false, true);
	FileDescriptor dst_fold = get_file(dest, false, false);
	copy(src_file, dst_fold);
	write_meta();
}

void FS::copy(FileDescriptor src_file, FileDescriptor dst_fold){
	Block * src_block = new Block(src_file.first_block, config.block_size, root);
	read_data(&src_file, sizeof(FileDescriptor), src_block);
	char * data = new char[src_file.size];
	read_data(data, src_file.size, src_block);

	Block * dst_block = new Block(dst_fold.first_block, config.block_size, root);
	for(auto it = DirIterator(*this, dst_fold); it != DirIterator(*this); ++it){
		FileDescriptor file = *it;
		if (std::string(file.filename) == std::string(src_file.filename) ) throw std::runtime_error("File already exist");
	}

	FileDescriptor file(src_file);
	Block * file_block = get_free_block();
	file.first_block = file_block->get_index();
	file.parent_file = dst_fold.first_block;
	file.prev_file = -1;
	file.next_file = -1;
	if(dst_fold.first_child != -1){
		Block * nb = new Block(dst_fold.first_child, config.block_size, root);
		FileDescriptor nd;
		read_data(&nd, sizeof(FileDescriptor), nb);
		nd.prev_file = file.first_block;
		update_descriptor(nd,nb);
		file.next_file = nd.first_block;
	}
	dst_fold.first_child = file.first_block;
	update_descriptor(dst_fold, dst_block);

	write_data(&file, sizeof(FileDescriptor), file_block);
	write_data(data, src_file.size, file_block);
	if(src_file.directory){
		for(auto it = DirIterator(*this, src_file); it != DirIterator(*this); ++it){
			FileDescriptor f = *it;
			copy(f, file);
		}
	}
}

void FS::ls(std::string filename){
	read_meta();
	FileDescriptor file = get_file(filename, false, true);

	std::cout << file.filename << ' ';
	std::cout << file.size << ' ';
	std::cout << ctime(&(file.last_update));
	if( file.directory ){
		for(auto it = DirIterator(*this, file); it != DirIterator(*this); ++it){
			FileDescriptor file = *it;
			std::cout << file.filename << std::endl;
		}
	}
}

void FS::rm(std::string path){
	read_meta();
	rm(get_file(path, false, true));
	write_meta();
}

void FS::rm(FileDescriptor file){
	Block * parent_block = new Block(file.parent_file, config.block_size, root);
	FileDescriptor parent_descriptor;
	read_data(&parent_descriptor, sizeof(FileDescriptor), parent_block);
	if(parent_descriptor.first_child == file.first_block){
		parent_descriptor.first_child = file.next_file;
		update_descriptor(parent_descriptor, parent_block);
	}

	if(file.prev_file != -1){
		Block * left_file_block = new Block(file.prev_file, config.block_size, root);
		FileDescriptor left_file_descriptor;
		read_data(&left_file_descriptor, sizeof(FileDescriptor), left_file_block);
		left_file_descriptor.next_file = file.next_file;
		update_descriptor(left_file_descriptor, left_file_block);
	}

	if(file.next_file != -1){
		Block * right_file_block = new Block(file.next_file, config.block_size, root);
		FileDescriptor right_file_descriptor;
		read_data(&right_file_descriptor, sizeof(FileDescriptor), right_file_block);
		right_file_descriptor.prev_file = file.prev_file;
		update_descriptor(right_file_descriptor, right_file_block);
	}


	Block * curr_block = new Block(file.first_block, config.block_size, root);
	if(file.directory){
		for(auto it = DirIterator(*this, file); it != DirIterator(*this); ++it){
			FileDescriptor file = *it;
			rm(file);
		}
		free_block(curr_block->get_index());
	} else{
		int size = file.size + sizeof(file);
		while( size > 0 ){
			size -= curr_block->_descriptor.data_written;
			free_block(curr_block->get_index());
			if(size > 0){
				curr_block = new Block(curr_block->_descriptor.next, config.block_size, root);
			}
		}
	}
}


void FS::mkdir(std::string path){
	read_meta();
	get_file(path, true, false);
}

void FS::move(std::string src, std::string dst){
	read_meta();
	FileDescriptor f_src = get_file(src, false, true);
	Block * src_block = new Block(f_src.first_block, config.block_size, root);

	Block * parent_block = new Block(f_src.parent_file, config.block_size, root);
	FileDescriptor parent_descriptor;
	read_data(&parent_descriptor, sizeof(FileDescriptor), parent_block);
	if(parent_descriptor.first_child == f_src.first_block){
		parent_descriptor.first_child = f_src.next_file;
		update_descriptor(parent_descriptor, parent_block);
	}

	FileDescriptor f_dst = get_file(dst, false, false);
	Block * dst_block = new Block(f_dst.first_block, config.block_size, root);

	f_src.parent_file = f_dst.first_block;
	if( f_dst.first_child == -1){
		f_dst.first_child = f_src.first_block;
		f_src.next_file = -1;
		f_src.prev_file = -1;
	} else{
		Block * child_block = new Block(f_dst.first_child, config.block_size, root);
		f_dst.first_child = f_src.first_block;
		FileDescriptor child_d;
		read_data(&child_d, sizeof(FileDescriptor), child_block);
		child_d.prev_file = f_src.first_block;
		f_src.prev_file = -1;
		f_src.next_file = child_d.first_block;
		update_descriptor(child_d, child_block);
	}
	update_descriptor(f_src, src_block);
	update_descriptor(f_dst, dst_block);

}

FileDescriptor FS::get_file(std::string path, bool create, bool file_available){

	if( path[path.size() - 1] != '/' ){
		path.push_back('/');
	}

	int pos;
	pos = path.find('/');
	if (pos != 0){ throw std::runtime_error("Bad path"); }
	path.erase(0, 1);

	FileDescriptor curr_dir;
	Block * curr_dir_block = new Block(meta.root_block, config.block_size, root);
	read_data(&curr_dir, sizeof(curr_dir), curr_dir_block);

	while(path.size() != 0){
		pos = path.find('/');
		std::string dir_name = path.substr(0, pos);

		bool found = false;
		for(auto it = DirIterator(*this, curr_dir); it != DirIterator(*this); ++it){
			FileDescriptor file = *it;
			if(std::string(file.filename) == dir_name){
				found = true;
				if(!file.directory){
					if(file_available && pos == (int)path.size() - 1){
						return file;
					}else{
						throw std::runtime_error("File in path");
					}
				}
				curr_dir = file;
			}
		}
		if(!found){
			if( create ){
				Block * dir_block = get_free_block();
				FileDescriptor dir;
				dir.set_filename(dir_name);
				dir.first_block = dir_block->get_index();
				dir.parent_file = curr_dir.first_block;
				if( curr_dir.first_child == -1 ){
					curr_dir.first_child = dir.first_block;
				} else{
					Block * neighbour_file_block = new Block(curr_dir.first_child, config.block_size, root);
					curr_dir.first_child = dir.first_block;
					FileDescriptor neighbour_file_descriptor;
					read_data(&neighbour_file_descriptor, sizeof(FileDescriptor), neighbour_file_block);
					dir.next_file = neighbour_file_descriptor.first_block;
					neighbour_file_descriptor.prev_file = dir.first_block;
					root_descriptor.first_child = dir.first_block;
					update_descriptor(neighbour_file_descriptor, neighbour_file_block);
				}
				update_descriptor(curr_dir, curr_dir_block);
				curr_dir = dir;

				write_data(&dir, sizeof(FileDescriptor), dir_block);
			} else{
				throw std::runtime_error("No such file or directory");
			}
		}

		curr_dir_block = new Block(curr_dir.first_block, config.block_size, root);
		path.erase(0, pos + 1);
	}
	write_meta();
	return curr_dir;
}

Config FS::read_config() {
    Config config;
    std::ifstream config_file;
    config_file.open(string(root) + "/config");
    if ( !config_file.good()) {
        throw std::runtime_error("Can't open config file");
    }
    config_file  >> config.block_size >> config.block_no;
    if (!config_file.good()
        || config.block_size < 1024
        || config.block_no < 1
        || config.block_size * config.block_no > 50 * 1024 * 1024) {
        throw std::runtime_error("Bad config file");
    }
    return config;
}

void FS::reserve_block(int n){
    int pos = n/8;
    int off = n%8;
    meta.block_map[pos] |= 1 << off;
}

void FS::free_block(int n){
    int pos = n/8;
    int off = n%8;
    meta.block_map[pos] |= 0 << off;
}

std::string FS::get_block_name(int block_id){
    return string(root) +"/"+ std::to_string(block_id);
}

void FS::write_meta(){
    int ind = 0;
    int meta_size = sizeof(meta);
    int size = meta.block_map_size;
    Block * curr_block = new Block(ind, config.block_size);


    curr_block->write(&meta, meta_size );
    while(size != 0){
        int written = curr_block->write(meta.block_map, size);
        size -= written;
		curr_block->save_block(root);
        if(size != 0){
        	curr_block->_descriptor.next = ind + 1;
        	curr_block = new Block(ind, config.block_size, root);
        }
    }
}


void FS::read_meta(){
	Block * curr_block = new Block(0, config.block_size, "fs");
	int meta_size = sizeof(meta);
	curr_block->read(&meta, meta_size);
	meta.block_map = new char[meta.block_map_size];
	read_data(meta.block_map, meta.block_map_size, curr_block);
}

void FS::read_data(void * data, int size, Block * curr_block){
	char * dt = (char *)data;
	while(size != 0){
		int readen = curr_block->read(dt, size);
		size -= readen;
		dt += readen;
	    if(size != 0){
			curr_block = new Block(curr_block->_descriptor.next, config.block_size, root);
	    }
	}
}

void FS::write_data(void * data, int size, Block * first_block = 0){
	char * dt = (char *)data;
	Block * last_block = 0;
    if(first_block == 0) first_block = get_free_block();
    last_block = first_block;
    Block * curr_block = last_block;
    while(size != 0){
        int written = curr_block->write(dt, size);
        size -= written;
        dt += written;
        if(size != 0){
			last_block = get_free_block();
			curr_block->_descriptor.next = last_block->get_index();
			curr_block->save_block(root);
			curr_block = last_block;
        } else{
        	curr_block->save_block(root);
        }
    }
}

Block * FS::get_free_block(){
    for(int i=0; i<config.block_no; i++) {
        int pos = i/8;
        int off = i%8;
        if((meta.block_map[pos] & (1 << off)) == 0) {
            meta.block_map[pos] |= 1 << off;
            return new Block(i, config.block_size);
        }
    }
    throw std::runtime_error("Out of memory");
}

DirIterator::DirIterator(FS & fs, FileDescriptor dir):
_fs(fs){
	if(dir.first_child != -1){
		p = new FileDescriptor;
		Block * block = _fs.get_block(dir.first_child, _fs.root);
		block->reopen();
		_fs.read_data(p, sizeof(FileDescriptor), block);
	} else{
		p = 0;
	}
}

DirIterator& DirIterator::operator++(){
	if(p->next_file != -1){
		Block * block = _fs.get_block(p->next_file, _fs.root);
		block->reopen();
		_fs.read_data(p, sizeof(FileDescriptor), block );
		return *this;
	} else{
		p = 0;
		return *this;
	}
}

void FS::update_descriptor(FileDescriptor & fd, Block * block){
	if(block->_descriptor.data_written > 0){
		int written = block->_descriptor.data_written;
		block->move_to_begin();
		block->write(&fd, sizeof(fd));
		block->_descriptor.data_written = written;
		block->save_block(root);
	} else{
		block->write(&fd, sizeof(fd));
	}
}
