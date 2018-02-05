//COSC 361 Spring 2017
//FUSE Project Template
//French Toast Mafia
//Jacob Vargo
//Guarab KC

#ifndef __cplusplus
#error "You must compile this using C++"
#endif
#include <fuse.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <map>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fs.h>

using namespace std;
class fuse_node{
public:
	NODE node;
	vector<BLOCK> blocks;
};
BLOCK_HEADER block_header;
map<string, fuse_node> nodes;
	
//Use debugf() and NOT printf() for your messages.
//Uncomment #define DEBUG in block.h if you want messages to show

//Here is a list of error codes you can return for
//the fs_xxx() functions
//
//EPERM          1      /* Operation not permitted */
//ENOENT         2      /* No such file or directory */
//ESRCH          3      /* No such process */
//EINTR          4      /* Interrupted system call */
//EIO            5      /* I/O error */
//ENXIO          6      /* No such device or address */
//ENOMEM        12      /* Out of memory */
//EACCES        13      /* Permission denied */
//EFAULT        14      /* Bad address */
//EBUSY         16      /* Device or resource busy */
//EEXIST        17      /* File exists */
//ENOTDIR       20      /* Not a directory */
//EISDIR        21      /* Is a directory */
//EINVAL        22      /* Invalid argument */
//ENFILE        23      /* File table overflow */
//EMFILE        24      /* Too many open files */
//EFBIG         27      /* File too large */
//ENOSPC        28      /* No space left on device */
//ESPIPE        29      /* Illegal seek */
//EROFS         30      /* Read-only file system */
//EMLINK        31      /* Too many links */
//EPIPE         32      /* Broken pipe */
//ENOTEMPTY     36      /* Directory not empty */
//ENAMETOOLONG  40      /* The name given is too long */

//Use debugf and NOT printf() to make your
//debug outputs. Do not modify this function.
#if defined(DEBUG)
int debugf(const char *fmt, ...)
{
	int bytes = 0;
	va_list args;
	va_start(args, fmt);
	bytes = vfprintf(stderr, fmt, args);
	va_end(args);
	return bytes;
}
#else
int debugf(const char *fmt, ...)
{
	return 0;
}
#endif

//////////////////////////////////////////////////////////////////
//
// START HERE W/ fs_drive()
//
//////////////////////////////////////////////////////////////////
//Read the hard drive file specified by dname
//into memory. You may have to use globals to store
//the nodes and / or blocks.
//Return 0 if you read the hard drive successfully (good MAGIC, etc).
//If anything fails, return the proper error code (-EWHATEVER)
//Right now this returns -EIO, so you'll get an Input/Output error
//if you try to run this program without programming fs_drive.
//////////////////////////////////////////////////////////////////
int fs_drive(const char *dname)
{
	NODE t_node;
	BLOCK t_block;
	vector<NODE> v_nodes;
	vector<BLOCK> v_blocks, blocks;
	string name;
	fuse_node class_node;
	FILE *f;
	uint64_t i, block_offset;

	debugf("fs_drive: %s\n", dname);
	f = fopen(dname, "r");
	if (f == NULL)
		return -ENOENT;
	if (fread(&block_header, sizeof(BLOCK_HEADER), 1, f) != 1)
		return -EIO;
	if (strcmp(block_header.magic, "COSC_361") != 0)
		return -EIO;
	
	for (i = 0; i < block_header.nodes; i++){
		if (fread(&(t_node), ONDISK_NODE_SIZE, 1, f) != 1)
			return -EIO;
		if (S_ISREG(t_node.mode)){
			if (fread(&(t_node.blocks), sizeof(uint64_t), 1, f) != 1)
				return -EIO;
		}
		else t_node.blocks = NULL;
		v_nodes.push_back(t_node);
	}

	for (i = 0; i < block_header.blocks; i++){
		t_block.data = new char[block_header.block_size];
		if (fread(t_block.data, block_header.block_size, 1, f) != 1){
			return -EIO;
		}
		blocks.push_back(t_block);
	}
	
	for (i = 0; i < v_nodes.size(); i++){
		name = v_nodes[i].name;
		class_node.node = v_nodes[i];
		block_offset = (uint64_t) (v_nodes[i].blocks);
		v_blocks.clear();
		if (S_ISREG(v_nodes[i].mode)){
			for (unsigned int j = block_offset; j < block_offset + (v_nodes[i].size / block_header.block_size + 1); j++){
				v_blocks.push_back(blocks[j]);
			}
		}
		class_node.blocks = v_blocks;
		nodes.insert(make_pair(name, class_node));
	}

	fclose(f);
	return 0;
}

//////////////////////////////////////////////////////////////////
//Open a file <path>. This really doesn't have to do anything
//except see if the file exists. If the file does exist, return 0,
//otherwise return -ENOENT
//////////////////////////////////////////////////////////////////
int fs_open(const char *path, struct fuse_file_info *fi)
{
	debugf("fs_open: %s\n", path);
	map<string, fuse_node>::iterator it;
	
	it = nodes.find(path);
	if (it == nodes.end())
		return -ENOENT;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Read a file <path>. You will be reading from the block and
//writing into <buf>, this buffer has a size of <size>. You will
//need to start the reading at the offset given by <offset>.
//////////////////////////////////////////////////////////////////
int fs_read(const char *path, char *buf, size_t size, off_t offset,
	    struct fuse_file_info *fi)
{
	debugf("fs_read: %s\n", path);
	map<string, fuse_node>::iterator it;
	PBLOCK t_block;
	int i;
	
	if (fi->flags & O_RDONLY){
		return -EROFS;
	}
	it = nodes.find(path);
	if (it == nodes.end()){
		return -EIO;
	}
	
	i = offset / block_header.block_size;
	offset = offset % block_header.block_size;
	buf[0] = '\0';
	while (i < it->second.blocks.size()){
		strncat(buf, it->second.blocks[i].data + offset, block_header.block_size - offset);
		offset = 0;
		i++;
	}
	return strlen(buf);
}

//////////////////////////////////////////////////////////////////
//Write a file <path>. If the file doesn't exist, it is first
//created with fs_create. You need to write the data given by
//<data> and size <size> into this file block. You will also need
//to write data starting at <offset> in your file. If there is not
//enough space, return -ENOSPC. Finally, if we're a read only file
//system (fi->flags & O_RDONLY), then return -EROFS
//If all works, return the number of bytes written.
//////////////////////////////////////////////////////////////////
int fs_write(const char *path, const char *data, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
	debugf("fs_write: %s\n", path);
	map<string, fuse_node>::iterator it;
	BLOCK t_block;
	int mysize, i, bytes_to_write, bytes_written;
	
	if (fi->flags & O_RDONLY){
		return -EROFS;
	}
	it = nodes.find(path);
	if (it == nodes.end()){
		printf("fs_create failed\n");
		return -EIO;
	}
	/*
	if (offset + size >= (it->second.node.size / block_header.block_size + 1) * block_header.block_size){
		return -ENOSPC;
	}
	*/
	
	i = offset / block_header.block_size;
	offset = offset % block_header.block_size;
	mysize = size;
	bytes_written = 0;
	while (mysize > 0){
		printf("Write: size: %d  i: %d  num_blocks: %d\n", mysize, i, it->second.blocks.size());
		if (i >= it->second.blocks.size()){
			//make another block to store data
			printf("Write: added block\n");
			t_block.data = new char[block_header.block_size];
			it->second.blocks.push_back(t_block);
			block_header.blocks++;
		}
		if (mysize > block_header.block_size - offset)
			bytes_to_write = block_header.block_size - offset;
		else
			bytes_to_write = mysize;
		printf("Write: writing: %d  offset: %d total: %d\n%s\n", bytes_to_write, offset, bytes_written, data);
		strncpy(it->second.blocks[i].data + offset, data + bytes_written, bytes_to_write);
		it->second.node.size += bytes_to_write;
		mysize -= bytes_to_write;
		bytes_written += bytes_to_write;
		offset = 0;
		i++;
	}
	return bytes_written;
}

//////////////////////////////////////////////////////////////////
//Create a file <path>. Create a new file and give it the mode
//given by <mode> OR'd with S_IFREG (regular file). If the name
//given by <path> is too long, return -ENAMETOOLONG. As with
//fs_write, if we're a read only file system
//(fi->flags & O_RDONLY), then return -EROFS.
//Otherwise, return 0 if all succeeds.
//////////////////////////////////////////////////////////////////
int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	debugf("fs_create: %s\n", path);
	map<string, fuse_node>::iterator it;
	BLOCK t_block;
	fuse_node nnode;
	
	if (fi->flags & O_RDONLY)
		return -EROFS;
	if(strlen(path) > NAME_SIZE)
		return -ENAMETOOLONG;
	it = nodes.find(path);
	if (it != nodes.end())
		return -EEXIST;
	
	nnode.node.mode = mode;
	nnode.node.size = 0;
	nnode.node.uid = getuid();
	nnode.node.gid = getgid();
	nnode.node.atime = time(NULL);
	nnode.node.mtime = time(NULL);
	nnode.node.ctime = time(NULL);
	t_block.data = new char[block_header.block_size];
	nnode.blocks.push_back(t_block);
	nodes.insert(make_pair(path, nnode));
	block_header.nodes++;
	block_header.blocks++;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Get the attributes of file <path>. A static structure is passed
//to <s>, so you just have to fill the individual elements of s:
//s->st_mode = node->mode
//s->st_atime = node->atime
//s->st_uid = node->uid
//s->st_gid = node->gid
// ...
//Most of the names match 1-to-1, except the stat structure
//prefixes all fields with an st_*
//Please see stat for more information on the structure. Not
//all fields will be filled by your filesystem.
//////////////////////////////////////////////////////////////////
int fs_getattr(const char *path, struct stat *s)
{
	debugf("fs_getattr: %s\n", path);
	map<string, fuse_node>::iterator it;
	NODE node;
	
	it = nodes.find(path);
	if (it == nodes.end())
		return -ENOENT;
	node = it->second.node;
	s->st_mode = node.mode;
	s->st_nlink = 1;
	s->st_uid = node.uid;
	s->st_gid = node.gid;
	s->st_size = node.size;
	s->st_atime = node.atime;
	s->st_mtime = node.mtime;
	s->st_ctime = node.ctime;
	
	return 0;
}

//////////////////////////////////////////////////////////////////
//Read a directory <path>. This uses the function <filler> to
//write what directories and/or files are presented during an ls
//(list files).
//
//filler(buf, "somefile", 0, 0);
//
//You will see somefile when you do an ls
//(assuming it passes fs_getattr)
//////////////////////////////////////////////////////////////////
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	       off_t offset, struct fuse_file_info *fi)
{
	map<string, fuse_node>::iterator it;
	map<string, string>::iterator it2;
	map<string, string> files_read;
	char temp[1000];
	string ptr;
	debugf("fs_readdir: %s\n", path);

	//filler(buf, <name of file/directory>, 0, 0)
	filler(buf, ".", 0, 0);
	filler(buf, "..", 0, 0);
	
	for (it = nodes.begin(); it != nodes.end(); it++){
		if (strncmp(it->first.c_str(), path, strlen(path)) == 0 && strcmp(it->first.c_str(), path) != 0){
			strcpy(temp, it->first.c_str()+strlen(path));
			if (strtok(temp, "/") != NULL){
				ptr = strtok(temp, "/");
			} 
			else{
				strcpy(temp, it->first.c_str());
				ptr = temp;
			}
			it2 = files_read.find(ptr);
			printf("readdir: filename: %s ptr: %s\n", it->first.c_str(), ptr.c_str());
			if (it2 == files_read.end()){
				files_read.insert(make_pair(ptr, ptr));
				printf("readdir: filename: %s\n", ptr.c_str());
				filler(buf, ptr.c_str(), 0, 0);
			}
		}
	}

	//You MUST make sure that there is no front slashes in the name (second parameter to filler)
	//Otherwise, this will FAIL.

	return 0;
}

//////////////////////////////////////////////////////////////////
//Open a directory <path>. This is analagous to fs_open in that
//it just checks to see if the directory exists. If it does,
//return 0, otherwise return -ENOENT
//////////////////////////////////////////////////////////////////
int fs_opendir(const char *path, struct fuse_file_info *fi)
{
	debugf("fs_opendir: %s\n", path);
	map<string, fuse_node>::iterator it;
	
	it = nodes.find(path);
	if (it == nodes.end())
		return -ENOENT;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Change the mode (permissions) of <path> to <mode>
//////////////////////////////////////////////////////////////////
int fs_chmod(const char *path, mode_t mode)
{
	map<string, fuse_node>::iterator it;
	
	it = nodes.find(path);
	if (it == nodes.end())
		return -ENOENT;
	it->second.node.mode = mode;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Change the ownership of <path> to user id <uid> and group id <gid>
//////////////////////////////////////////////////////////////////
int fs_chown(const char *path, uid_t uid, gid_t gid)
{
	debugf("fs_chown: %s\n", path);
	map<string, fuse_node>::iterator it;
	
	it = nodes.find(path);
	if (it == nodes.end())
		return -ENOENT;
	it->second.node.uid = uid;
	it->second.node.gid = gid;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Unlink a file <path>. This function should return -EISDIR if a
//directory is given to <path> (do not unlink directories).
//Furthermore, you will not need to check O_RDONLY as this will
//be handled by the operating system.
//Otherwise, delete the file <path> and return 0.
//////////////////////////////////////////////////////////////////
int fs_unlink(const char *path)
{
	debugf("fs_unlink: %s\n", path);
	map<string, fuse_node>::iterator it;
	
	it = nodes.find(path);
	if (it == nodes.end())
		return -ENOENT;
	
	if (S_ISDIR(it->second.node.mode))
		return -EISDIR;
	
	for(int i = 0; i < it->second.blocks.size(); i++){
		delete it->second.blocks[i].data;
		block_header.blocks--;
	}
	it->second.blocks.clear();
	
	//remove it from nodes
	nodes.erase(it);
	block_header.nodes--;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Make a directory <path> with the given permissions <mode>. If
//the directory already exists, return -EEXIST. If this function
//succeeds, return 0.
//////////////////////////////////////////////////////////////////
int fs_mkdir(const char *path, mode_t mode)
{
	debugf("fs_mkdir: %s\n", path);
	map<string, fuse_node>::iterator it;
	it = nodes.find(path);
	if (it != nodes.end())
		return -EEXIST;
	
	fuse_node nnode;
	nnode.node.mode = mode;
	nnode.node.size = 0;
	nnode.node.uid = getuid();
	nnode.node.gid = getgid();
	nnode.node.atime = time(NULL);
	nnode.node.mtime = time(NULL);
	nnode.node.ctime = time(NULL);
	nodes.insert(make_pair(path, nnode));
	block_header.nodes++;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Remove a directory. You have to check to see if it is
//empty first. If it isn't, return -ENOTEMPTY, otherwise
//remove the directory and return 0.
//////////////////////////////////////////////////////////////////
int fs_rmdir(const char *path)
{
	debugf("fs_rmdir: %s\n", path);
	map<string, fuse_node>::iterator it;
	map<string, string>::iterator it2;
	map<string, string> files_read;
	char temp[1000];
	string ptr;
	it = nodes.find(path);
	if (it == nodes.end())
		return -ENOENT;
	
	for (it = nodes.begin(); it != nodes.end(); it++){
		if (strncmp(it->first.c_str(), path, strlen(path)) == 0){
			strcpy(temp, it->first.c_str());
			if (strtok(temp, "/") != NULL){
				ptr = strtok(temp, "/");
			}
			else{
				strcpy(temp, it->first.c_str());
				ptr = temp;
			}
			it2 = files_read.find(ptr);
			if (it2 == files_read.end() && ptr != "/"){
				files_read.insert(make_pair(ptr, ptr));
			}else{
				return -ENOTEMPTY;
			}
		}
	}
	
	nodes.erase(it);
	block_header.nodes--;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Rename the file given by <path> to <new_name>
//Both <path> and <new_name> contain the full path. If
//the new_name's path doesn't exist return -ENOENT. If
//you were able to rename the node, then return 0.
//////////////////////////////////////////////////////////////////
int fs_rename(const char *path, const char *new_name)
{
	debugf("fs_rename: %s -> %s\n", path, new_name);
	map<string, fuse_node>::iterator it;
	fuse_node nnode;
	string name;
	
	it = nodes.find(path);
	if (it == nodes.end())
		return -ENOENT;
	name = it->first;
	nnode = it->second;
	nodes.erase(it);
	strcpy(nnode.node.name, new_name);
	nodes.insert(make_pair(name, nnode));
	
	return 0;
}

//////////////////////////////////////////////////////////////////
//Rename the file given by <path> to <new_name>
//Both <path> and <new_name> contain the full path. If
//the new_name's path doesn't exist return -ENOENT. If
//you were able to rename the node, then return 0.
//////////////////////////////////////////////////////////////////
int fs_truncate(const char *path, off_t size)
{
	debugf("fs_truncate: %s to size %d\n", path, size);
	map<string, fuse_node>::iterator it;
	
	it = nodes.find(path);
	if (it == nodes.end())
		return -ENOENT;
	for(int i = 0; i < it->second.blocks.size(); i++){
		delete it->second.blocks[i].data;
		block_header.blocks--;
	}
	it->second.blocks.clear();
	it->second.node.size = 0;
	return 0;
}

//////////////////////////////////////////////////////////////////
//fs_destroy is called when the mountpoint is unmounted
//this should save the hard drive back into <filename>
//////////////////////////////////////////////////////////////////
void fs_destroy(void *ptr)
{
	const char *filename = (const char *)ptr;
	debugf("fs_destroy: %s\n", filename);
	map<string, fuse_node>::iterator it;
	FILE *f;
	uint64_t blocks_written;
	vector<BLOCK> v_blocks;

	//Save the internal data to the hard drive
	//specified by <filename>
	f = fopen(filename, "w");
	fwrite(&block_header, sizeof(BLOCK_HEADER), 1, f);
	blocks_written = 0;
	for (it = nodes.begin(); it != nodes.end(); it++){
		//save it->second.nodes
		fwrite(&(it->second.node), ONDISK_NODE_SIZE, 1, f);
		if (S_ISREG(it->second.node.mode)){
			fwrite(&(blocks_written), sizeof(uint64_t), 1, f);
			blocks_written += it->second.blocks.size();
		}
	}
	for (it = nodes.begin(); it != nodes.end(); it++){
		//save it->second.blocks
		//printf("Node %s:\n", it->first.c_str());
		for (unsigned i = 0; i < it->second.blocks.size(); i++){
			fwrite(it->second.blocks[i].data, block_header.block_size, 1, f);
			//printf("%s data block %d:\n%s\n", it->first.c_str(), i, it->second.blocks[i].data);
		}
	}
	fclose(f);
	nodes.clear();
}

//////////////////////////////////////////////////////////////////
//int main()
//DO NOT MODIFY THIS FUNCTION
//////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	fuse_operations *fops;
	char *evars[] = { "./fs", "-f", "mnt", NULL };
	int ret;

	if ((ret = fs_drive(HARD_DRIVE)) != 0) {
		debugf("Error reading hard drive: %s\n", strerror(-ret));
		return ret;
	}
	//FUSE operations
	fops = (struct fuse_operations *) calloc(1, sizeof(struct fuse_operations));
	fops->getattr = fs_getattr;
	fops->readdir = fs_readdir;
	fops->opendir = fs_opendir;
	fops->open = fs_open;
	fops->read = fs_read;
	fops->write = fs_write;
	fops->create = fs_create;
	fops->chmod = fs_chmod;
	fops->chown = fs_chown;
	fops->unlink = fs_unlink;
	fops->mkdir = fs_mkdir;
	fops->rmdir = fs_rmdir;
	fops->rename = fs_rename;
	fops->truncate = fs_truncate;
	fops->destroy = fs_destroy;

	debugf("Press CONTROL-C to quit\n\n");

	return fuse_main(sizeof(evars) / sizeof(evars[0]) - 1, evars, fops,
			 (void *)HARD_DRIVE);
}
