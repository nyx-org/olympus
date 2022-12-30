#include <bootstrap.h>
#include <ichor/alloc.h>
#include <ichor/debug.h>
#include <ichor/port.h>
#include <ichor/syscalls.h>
#include <posix.h>
#include <stdc-shim/string.h>

#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
#define DT_WHT 14
typedef long off_t;
typedef long off64_t;
typedef long ino_t;
struct dirent
{
    ino_t d_ino;
    off_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[1024];
};
static Port posix_port = PORT_NULL;

#define open(path, mode) posix_open(posix_port, sys_getpid(), path, mode)
#define stat(fd, path, out) posix_stat(posix_port, sys_getpid(), fd, path, out, sizeof(*out))
#define read(fd, buf, nbyte) posix_read(posix_port, sys_getpid(), fd, buf, nbyte)
#define write(fd, buf, nbyte) posix_write(posix_port, sys_getpid(), fd, buf, nbyte)
#define seek(fd, offset, whence) posix_seek(posix_port, sys_getpid(), fd, offset, whence)
#define close(fd) posix_close(posix_port, sys_getpid(), fd)
#define readdir(fd, buf, max_size, bytes_read) posix_readdir(posix_port, sys_getpid(), fd, buf, max_size, bytes_read, sizeof(size_t))
#define mmap(fd, space, hint, size, prot, flags, offset, out) posix_mmap(posix_port, sys_getpid(), fd, space, hint, size, prot, flags, offset, out, sizeof(void *))
#define PROT_NONE 0x00
#define PROT_READ 0x01
#define PROT_WRITE 0x02
#define PROT_EXEC 0x04

#define MAP_FAILED ((void *)(-1))
#define MAP_FILE 0x00
#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02
#define MAP_FIXED 0x10
#define MAP_ANON 0x20
#define MAP_ANONYMOUS 0x20
#define debug ichor_debug

void server_main()
{
   /* while (true)
    {
        if ((posix_port = bootstrap_look_up("org.nyx.posix")) != PORT_NULL)
        {
            break;
        }
    }

    int fd = open("/usr", 0);
    void *out = NULL;

    ichor_debug("fd is %d", fd);
    struct dirent dirents[4];

    Task my_task;
    sys_get_current_task(&my_task);

    mmap(0, my_task.space, NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, 0, (void *)&out);

    size_t bytes_read;
    readdir(fd, dirents, sizeof(dirents), &bytes_read);

    for (size_t i = 0; i < sizeof(dirents) / sizeof(*dirents); i++)
    {
        debug("%s: %s", dirents[i].d_type == DT_DIR ? "dir" : "file", dirents[i].d_name);
    }

    close(fd);

    sys_exit(0);*/

    sys_exit(0);
}