#ifndef POSIX_PROC_H
#define POSIX_PROC_H
#include <fs/fd.h>
#include <posix/types.h>
#include <vec.h>

// Internal representation of a process
typedef struct
{
    pid_t pid;
    int current_fd;
    Vec(File) fds;
    Vnode *cwd;
} Proc;

#endif /* POSIX_PROC_H */
