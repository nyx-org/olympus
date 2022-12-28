#include <bootstrap.h>
#include <ichor/debug.h>
#include <ichor/port.h>
#include <ichor/syscalls.h>
#include <posix.h>
#include <stdc-shim/string.h>

typedef long off_t;
typedef long off64_t;
typedef long ino_t;
typedef unsigned int mode_t;
typedef int uid_t;
typedef int gid_t;
typedef int pid_t;
typedef int tid_t;
typedef long time_t;
typedef long blkcnt_t;
typedef long blksize_t;
typedef unsigned long dev_t;
typedef unsigned long nlink_t;

struct timespec
{
    time_t tv_sec;
    long tv_nsec;
};

struct stat
{
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    dev_t st_rdev;
    off_t st_size;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
    blksize_t st_blksize;
    blkcnt_t st_blocks;
};

static Port posix_port = PORT_NULL;

#define open(path, mode) posix_open(posix_port, sys_getpid(), path, mode)
#define stat(fd, path, out) posix_stat(posix_port, sys_getpid(), fd, path, out, sizeof(*out))
#define read(fd, buf, nbyte) posix_read(posix_port, sys_getpid(), fd, buf, nbyte)
#define write(fd, buf, nbyte) posix_write(posix_port, sys_getpid(), fd, buf, nbyte)
#define seek(fd, offset, whence) posix_seek(posix_port, sys_getpid(), fd, offset, whence)

#define debug ichor_debug

void server_main()
{
    while (true)
    {
        if ((posix_port = bootstrap_look_up("org.nyx.posix")) != PORT_NULL)
        {
            ichor_debug("Found posix!, port: %d", posix_port);
            break;
        }
    }

    int fd = open("/usr/hello.txt", 0);

    char buf[8192];
    struct stat stat;

    stat(fd, "", &stat);
    write(fd, "lol", 3);
    seek(fd, 0, 0);
    read(fd, buf, stat.st_size);
    debug("file content: %s", buf);

    sys_exit(0);
}
