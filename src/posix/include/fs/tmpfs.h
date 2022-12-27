#ifndef POSIX_TMPFS_H
#define POSIX_TMPFS_H
#include <fs/vfs.h>
#include <vec.h>

typedef struct tmpnode TmpNode;

typedef struct tmp_dirent
{
    Vec(struct tmp_dirent) entries;
    const char *name;
    TmpNode *node;
} TmpDirent;

typedef struct tmpnode
{
    Vnode *vnode;
    Vattr attr;

    union
    {
        /* VDIR */
        struct
        {
            Vec(TmpDirent *) entries;
            struct tmpnode *parent;
        } dir;

        /* VREG */
        struct
        {
            void *buffer;
        } reg;
    };
} TmpNode;

void tmpfs_init(void);

#endif /* POSIX_TMPFS_H */
