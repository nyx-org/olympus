#ifndef POSIX_H
#define POSIX_H
#include <ichor/alloc.h>
#include <ichor/charon.h>
#include <ichor/debug.h>
#include <ichor/error.h>
#include <ichor/port.h>
#include <ichor/syscalls.h>
#include <posix/errno.h>
#include <posix/proc.h>
#include <posix/stat.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define POSIX_CHECK_ERRNO() (sys_errno == ERR_SUCCESS ? (void)0 : ichor_debug("posix: Error: %d", sys_errno))
#define POSIX_PANIC(string) ({\
    ichor_debug(string);    \
    sys_exit(-1); })

int posix_sys_open(Proc *proc, const char *path, int mode);
int posix_sys_read(Proc *proc, int fd, void *buf, size_t bytes);
int posix_sys_write(Proc *proc, int fd, void *buf, size_t bytes);
int posix_sys_seek(Proc *proc, int fd, off_t offset, int whence);
int posix_sys_readdir(Proc *proc, int fd, void *buf, size_t max_size, size_t *bytes_read);
int posix_sys_close(Proc *proc, int fd);
int posix_sys_stat(Proc *proc, int fd, const char *path, struct stat *out);

#endif /* POSIX_H */
