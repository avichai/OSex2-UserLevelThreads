#define FUSE_USE_VERSION 26
// ------------------------------ includes -------------------------------------
#include <fuse.h>
#include <fstream>
#include <iostream>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include "FBR.h"

using namespace std;


// ------------------------------- macros --------------------------------------

#define FAILURE 1
#define SUCCESS 0
#define TIME_FAILURE -1
#define ROOT_DIR_INDEX 1
#define MOUNT_DIR_INDEX 2
#define N_BLOCKS_INDEX 3
#define F_OLD_INDEX 4
#define F_NEW_INDEX 5
#define VALID_N_ARGS 6
#define DECIMAL 10
#define WORKING_DIR "/tmp"
#define LOG_NAME ".filesystem.log"
#define READ_MASK 3



// ------------------------------- struct --------------------------------------
/*
 * Chaching FS data structure.
 */
struct CFSdata
{
	string rootDirPath;
	ofstream logFile;
	Cache* cache;
};

// ------------------------------- globals -------------------------------------
struct fuse_operations caching_oper;
static CFSdata cFSdata;



//TODO delete later (realize what doing)
#define Caching_DATA ((struct CFSdata*) fuse_get_context()->private_data)


// -------------------------- static functions ---------------------------------
/*
 * Returns true iff the path is the log file's path.
 */
static bool isLogFile(string path) {
	string log = "/";
	log += LOG_NAME;
	return path == log;
}

/*
 * Handles exceptions during the main.
 */
static void exceptionHandlerMain(string funcName)
{
	cerr << "System Error: " << funcName << " failed." << endl;
	exit(EXIT_FAILURE);
}

/*
 * Checking the result (of a function)  during the main.
 */
static void checkSysCallMain(int result, string funcName)
{
	if (result != SUCCESS)
	{
		exceptionHandlerMain(funcName);
	}
}

/*
 * Returns zero if the result is zero, and -errno otherwise.
 */
static int checkSysCallFS(int result)
{
	return result == SUCCESS ? SUCCESS : -errno;
}

/*
 * Validates the directory path.
 */
static bool validDir(char* dirPath)
{
	struct stat statBuf;
	checkSysCallMain(stat(dirPath, &statBuf), "stat");
	return (bool) S_ISDIR(statBuf.st_mode);
}

/*
 * Returns true iff the string represents a double, and assigns the double value
 * to the given reference.
 */
static bool isDouble(char* str, double &percentage)
{
	char* end  = 0;
	percentage = strtod(str, &end);
	return (*end == '\0') && (end != str);
}

/*
 * Returns true iff the string represents a positive int, and assigns the int
 * value to the given reference.
 */
static bool isPosInt(char* str, unsigned int &cacheSize)
{
	char* end  = 0;
	int tmpCacheSize = strtol(str, &end, DECIMAL);
	cacheSize = (unsigned int) tmpCacheSize;

	return (*end == '\0') && (end != str) && (tmpCacheSize > 0);
}

/*
 * Validates the fOld and fNew args and assigns old/new number of blocks.
 */
static bool validPercentage(double percentage, unsigned int cacheSize, unsigned int &nBlks)
{
	int tmpNBlks = (int) (percentage * cacheSize);
	nBlks = (unsigned int) tmpNBlks;
	return (percentage > 0) && (percentage < 1) && (tmpNBlks > 0);
}

/*
 * Validates the block args.
 */
static bool validBlockArgs(char* argv[], unsigned int &nOldblks, unsigned int &nNewBlks, unsigned int &cacheSize)
{
//	int nBlocks;
	double fOld, fNew;

	return isPosInt(argv[N_BLOCKS_INDEX], cacheSize) &&
		   isDouble(argv[F_OLD_INDEX], fOld) &&
		   isDouble(argv[F_NEW_INDEX], fNew) &&
		   validPercentage(fOld, cacheSize, nOldblks) &&
		   validPercentage(fNew, cacheSize, nNewBlks) &&
		   fOld + fNew <= 1;


}


/*
 * Validates the program's arguments.
 */
static bool validArgs(int argc, char* argv[], unsigned int &nOldblks, unsigned int &nNewBlks, unsigned int &cacheSize)
{

	return argc == VALID_N_ARGS &&
		   validDir(argv[ROOT_DIR_INDEX]) &&
		   validDir(argv[MOUNT_DIR_INDEX]) &&
		   validBlockArgs(argv, nOldblks, nNewBlks, cacheSize);
}

/*
 * Writes the given function to the log file.
 */
static int writeFuncToLog(string funcName)
{
	// openning the log file
	cFSdata.logFile.open(cFSdata.rootDirPath +  LOG_NAME, ios::app);        // todo: add ios::app flag
	if (cFSdata.logFile.fail())
	{
		return -errno; 		// todo how to handle this exception (not like this!).
	}


	time_t seconds = time(NULL);
	if (seconds == (time_t) TIME_FAILURE)
	{
		return -errno;
	}
	cFSdata.logFile << seconds << " " << funcName << endl;

	// closing the log file
	cFSdata.logFile.close();		// todo: check if close fails

	return SUCCESS;
}

/*
 * Return the full path of the given relative path.
 */
static string getFullPath(string path)
{
	return cFSdata.rootDirPath  +  path;
}


// ------------------------------ functions ------------------------------------
/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int caching_getattr(const char *path, struct stat *statbuf)
{
	cerr << "!!!!!!!!!!!!!!!!!!!! getattr called !!!!!!!!!!!!!!!!!!!!!" << endl;		//todo
	if (isLogFile(path)) {
		return -ENOENT;
	}
	if (writeFuncToLog("caching_getattr") != SUCCESS)
	{
		return -errno;
	}

	string fullPath = getFullPath(string(path));
	return checkSysCallFS(lstat(fullPath.c_str(), statbuf));
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int caching_fgetattr(const char *path, struct stat *statbuf,
					 struct fuse_file_info *fi)
{
	cerr << "!!!!!!!!!!!!!!!!!!!! fgetattr called !!!!!!!!!!!!!!!!!!!!!" << endl;		//todo

	// On FreeBSD, trying to do anything with the mountpoint ends up
	// opening it, and then using the FD for an fgetattr.  So in the
	// special case of a path of "/", I need to do a getattr on the
	// underlying root directory instead of doing the fgetattr().
	if (!strcmp(path, "/")) {
		return caching_getattr(path, statbuf);
	}
	if (isLogFile(path)) {
		return -ENOENT;
	}
	if (writeFuncToLog("caching_fgetattr") != SUCCESS)
	{
		return -errno;
	}

	return checkSysCallFS(fstat(fi->fh, statbuf));
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int caching_access(const char *path, int mask)
{
	cerr << "!!!!!!!!!!!!!!!!!!!! caching_access called !!!!!!!!!!!!!!!!!!!!!" << endl;		//todo
	if (isLogFile(path)) {
		return -ENOENT;
	}
	if (writeFuncToLog("caching_access") != SUCCESS)
	{
		return -errno;
	}

	string fullPath = getFullPath(string(path));
	return checkSysCallFS(access(fullPath.c_str(), mask));
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * initialize an arbitrary filehandle (fh) in the fuse_file_info
 * structure, which will be passed to all file operations.

 * pay attention that the max allowed path is PATH_MAX (in limits.h).
 * if the path is longer, return error.

 * Changed in version 2.2
 */
int caching_open(const char *path, struct fuse_file_info *fi)
{
	cerr << "!!!!!!!!!!!!!!!!!!!! caching_open called !!!!!!!!!!!!!!!!!!!!!" << endl;		//todo
	if (isLogFile(path)) {
		return -ENOENT;
	}
	if (writeFuncToLog("caching_open") != SUCCESS)
	{
		return -errno;
	}


	string fullPath = getFullPath(string(path));
	int fd;
	// if the open call succeeds, my retstat is the file descriptor,
	// else it's -errno.  I'm making sure that in that case the saved
	// file descriptor is exactly -1.
	if ((fi->flags & READ_MASK) != O_RDONLY) {
		return -EACCES;		// todo check ret val
	}
	fd = open(fullPath.c_str(), O_RDONLY | O_DIRECT | O_SYNC);
	if (fd < SUCCESS) {
		return -errno;
	}

	fi->fh = fd;
	// fi->direct_io = true;				todo maybe change this.

	return SUCCESS;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error. For example, if you receive size=100, offest=0,
 * but the size of the file is 10, you will init only the first
   ten bytes in the buff and return the number 10.

   In order to read a file from the disk,
   we strongly advise you to use "pread" rather than "read".
   Pay attention, in pread the offset is valid as long it is
   a multipication of the block size.
   More specifically, pread returns 0 for negative offset
   and an offset after the end of the file
   (as long as the the rest of the requirements are fulfiiled).
   You are suppose to preserve this behavior also in your implementation.

 * Changed in version 2.2
 */
int caching_read(const char *path, char *buf, size_t size,
				 off_t offset, struct fuse_file_info *fi)

{
	cerr << "!!!!!!!!!!!!!!!!!!!! caching_read called !!!!!!!!!!!!!!!!!!!!!" << endl;		//todo
	if (isLogFile(path)) {
		return -ENOENT;
	}
	if (writeFuncToLog("caching_read") != SUCCESS)
	{
		return -errno;
	}
//	return cFSdata.cache->readData(buf, size, offset, (int) fi->fh, path);

//	cerr << "ret " << cFSdata.cache->readData(buf, size, offset, (int) fi->fh, path) << endl;
	cerr << strlen(buf)  << endl;//todo
	string a = "1234";
	strcpy(buf, a.c_str());

	return 0;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int caching_flush(const char *path, struct fuse_file_info *fi)
{
	cerr << "!!!!!!!!!!!!!!!!!!!! caching_flush called !!!!!!!!!!!!!!!!!!!!!" << endl;		//todo
	if (isLogFile(path)) {
		return -ENOENT;
	}
	return checkSysCallFS(writeFuncToLog("caching_flush"));
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int caching_release(const char *path, struct fuse_file_info *fi)
{
	cerr << "!!!!!!!!!!!!!!!!!!!! caching_release called !!!!!!!!!!!!!!!!!!!!!" << endl;		//todo
	if (isLogFile(path)) {
		return -ENOENT;
	}
	if (writeFuncToLog("caching_release") != SUCCESS) {
		return -errno;
	}
	// We need to close the file.  Had we allocated any resources
	// (buffers etc) we'd need to free them here as well.
	return checkSysCallFS(close(fi->fh));
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int caching_opendir(const char *path, struct fuse_file_info *fi)
{
	cerr << "!!!!!!!!!!!!!!!!!!!! caching_opendir called !!!!!!!!!!!!!!!!!!!!!" << endl;		//todo
	if (isLogFile(path)) {
		return -ENOENT;
	}
	if (writeFuncToLog("caching_opendir") != SUCCESS)
	{
		return -errno;
	}
	DIR* dp;
	int retstat = 0;

	string fullPath = getFullPath(string(path));

	// since opendir returns a pointer, takes some custom handling of
	// return status.
	dp = opendir(fullPath.c_str());
	if (dp == NULL) {
		retstat = -errno;
	}

	fi->fh = (intptr_t) dp;

	return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * Introduced in version 2.3
 */
int caching_readdir(const char *path, void *buf,
					fuse_fill_dir_t filler,
					off_t offset, struct fuse_file_info *fi)
{
	cerr << "!!!!!!!!!!!!!!!!!!!! readdir called !!!!!!!!!!!!!!!!!!!!!" << endl;		//todo	int retstat = 0;
	if (isLogFile(path)) {
		return -ENOENT;
	}
	if (writeFuncToLog( "caching_readdir") != SUCCESS) {
		return -errno;
	}

	DIR *dp;
	struct dirent *de;

	// once again, no need for fullpath -- but note that I need to cast fi->fh
	dp = (DIR *) (uintptr_t) fi->fh;

	// Every directory contains at least two entries: . and ..  If my
	// first call to the system readdir() returns NULL I've got an
	// error; near as I can tell, that's the only condition under
	// which I can get an error from readdir()
	de = readdir(dp);
	if (de == 0) {
		return -errno;
	}

	// This will copy the entire directory into the buffer.  The loop exits
	// when either the system readdir() returns NULL, or filler()
	// returns something non-zero.  The first case just means I've
	// read the whole directory; the second means the buffer is full.
	do {
		if (string(de->d_name) == LOG_NAME) {
			continue;
		}
		if (filler(buf, de->d_name, NULL, 0) != 0) {
			return -ENOMEM;
		}
	} while ((de = readdir(dp)) != NULL);


	return SUCCESS;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int caching_releasedir(const char *path, struct fuse_file_info *fi)
{
	cerr << "!!!!!!!!!!!!!!!!!!!! caching_releasedir called !!!!!!!!!!!!!!!!!!!!!" << endl;		//todo	int retstat = 0;
	if (isLogFile(path)) {
		return -ENOENT;
	}
	if (writeFuncToLog("caching_releasedir") != SUCCESS)
	{
		return -errno;
	}

	return checkSysCallFS(closedir((DIR *) (uintptr_t) fi->fh));
}

/**
 * Rename a file
 * */
int caching_rename(const char *path, const char *newpath)
{
	cerr << "!!!!!!!!!!!!!!!!!!!! caching_rename called !!!!!!!!!!!!!!!!!!!!!" << endl;		//todo	int retstat = 0;
	if (isLogFile(path)) {
		return -ENOENT;
	}
	if (writeFuncToLog("caching_rename") != SUCCESS) {
		return -errno;
	}
	string fullPath = getFullPath(string(path));
	string fullNewPath = getFullPath(string(newpath));

	return checkSysCallFS(rename(fullPath.c_str(), fullNewPath.c_str()));
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *

If a failure occurs in this function, do nothing (absorb the failure
and don't report it).
For your task, the function needs to return NULL always
(if you do something else, be sure to use the fuse_context correctly).
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *caching_init(struct fuse_conn_info *conn)
{
	cerr << "!!!!!!!!!!!!!!!!!!!! caching_init called !!!!!!!!!!!!!!!!!!!!!" << endl;		//todo	int retstat = 0;

	writeFuncToLog("caching_init");

	return Caching_DATA;		// TODO what is this line?
}


/**
 * Clean up filesystem
 *
 * Called on filesystem exit.

If a failure occurs in this function, do nothing
(absorb the failure and don't report it).

 * Introduced in version 2.3
 */
void caching_destroy(void *userdata)
{
	cerr << "!!!!!!!!!!!!!!!!!!!! caching_destroy called !!!!!!!!!!!!!!!!!!!!!" << endl;		//todo	int retstat = 0;
	writeFuncToLog("caching_destroy");
	delete cFSdata.cache;
}

/**
 * Ioctl from the FUSE sepc:
 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
 * 64bit environment.  The size and direction of data is
 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
 * data will be NULL, for _IOC_WRITE data is out area, for
 * _IOC_READ in area and if both are set in/out area.  In all
 * non-NULL cases, the area is of _IOC_SIZE(cmd) bytes.
 *
 * However, in our case, this function only needs to print
 cache table to the log file .
 *
 * Introduced in version 2.8
 */
int caching_ioctl (const char *, int cmd, void *arg,
				   struct fuse_file_info *, unsigned int flags, void *data)
{
	// todo implement
	return 0;
}


// Initialise the operations.
// You are not supposed to change this function.
void init_caching_oper()
{

	caching_oper.getattr = caching_getattr;
	caching_oper.access = caching_access;
	caching_oper.open = caching_open;
	caching_oper.read = caching_read;
	caching_oper.flush = caching_flush;
	caching_oper.release = caching_release;
	caching_oper.opendir = caching_opendir;
	caching_oper.readdir = caching_readdir;
	caching_oper.releasedir = caching_releasedir;
	caching_oper.rename = caching_rename;
	caching_oper.init = caching_init;
	caching_oper.destroy = caching_destroy;
	caching_oper.ioctl = caching_ioctl;
	caching_oper.fgetattr = caching_fgetattr;


	caching_oper.readlink = NULL;
	caching_oper.getdir = NULL;
	caching_oper.mknod = NULL;
	caching_oper.mkdir = NULL;
	caching_oper.unlink = NULL;
	caching_oper.rmdir = NULL;
	caching_oper.symlink = NULL;
	caching_oper.link = NULL;
	caching_oper.chmod = NULL;
	caching_oper.chown = NULL;
	caching_oper.truncate = NULL;
	caching_oper.utime = NULL;
	caching_oper.write = NULL;
	caching_oper.statfs = NULL;
	caching_oper.fsync = NULL;
	caching_oper.setxattr = NULL;
	caching_oper.getxattr = NULL;
	caching_oper.listxattr = NULL;
	caching_oper.removexattr = NULL;
	caching_oper.fsyncdir = NULL;
	caching_oper.create = NULL;
	caching_oper.ftruncate = NULL;
}

//basic main. You need to complete it.
int main(int argc, char* argv[])
{
	// validate and assigns args
	unsigned int nOldBlks, nNewBlks, cacheSize;
	if (!validArgs(argc, argv, nOldBlks, nNewBlks, cacheSize))
	{
		cout << "Usage: CachingFileSystem rootdir mountdir numberOfBlocks "
				"fOld fNew" << endl;
		return FAILURE;
	}

	// init the CFS data
	cFSdata.rootDirPath = string(argv[ROOT_DIR_INDEX]) + "/";
	struct stat fi;
	checkSysCallMain(stat(WORKING_DIR, &fi), "stat");
	size_t blkSize = (size_t) fi.st_blksize;
	try {
		cFSdata.cache = new Cache(blkSize, nOldBlks, nNewBlks, cacheSize);
	} catch (bad_alloc) {
		exceptionHandlerMain("new");
	}

	init_caching_oper();
	argv[1] = argv[2];
	for (int i = 2; i< (argc - 1); i++){
		argv[i] = NULL;
	}
        argv[3] = (char*) "-f";
	argv[2] = (char*) "-s";
	argc = 4;

	return fuse_main(argc, argv, &caching_oper, NULL);
}

