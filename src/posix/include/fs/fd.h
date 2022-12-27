#ifndef POSIX_FD_H
#define POSIX_FD_H
#include <fs/vfs.h>

typedef struct file File;

typedef struct
{
    int (*read)(File *file, void *buf, size_t bytes, off_t offset);
    int (*write)(File *file, void *buf, size_t bytes, off_t offset);
} FileOps;

/* File description */
typedef struct file
{
    int fd;
    Vnode *vnode;
    size_t position;
    FileOps ops;
} File;

#endif /* POSIX_FD_H */