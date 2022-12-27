#ifndef POSIX_VFS_H
#define POSIX_VFS_H
#include <ichor/base.h>
#include <posix/types.h>

#define VFS_FIND_OR_ERROR (1 << 0)
#define VFS_FIND_AND_CREATE (1 << 1)
#define VFS_FIND_AND_MKDIR (1 << 2)

typedef enum
{
    VNON, /* No type */
    VREG, /* Regular file */
    VDIR, /* Directory */
} VnodeType;

typedef struct vnode Vnode;

typedef struct
{
    VnodeType type;
    size_t size;
    mode_t mode;
    uid_t uid;
    gid_t gid;
} Vattr;

typedef struct
{
    int (*lookup)(Vnode *vn, Vnode **out, const char *name);
    int (*create)(Vnode *vn, Vnode **out, const char *name, Vattr *vattr);
    int (*getattr)(Vnode *vn, Vattr *out);
    int (*open)(Vnode *vn, int mode);
    int (*read)(Vnode *vn, void *buf, size_t nbyte, size_t off);
    int (*write)(Vnode *vn, void *buf, size_t nbyte, size_t off);
    int (*mkdir)(Vnode *vn, Vnode **out, const char *name,
                 Vattr *vattr);

    int (*readdir)(Vnode *vn, void *buf, size_t max_size, size_t *bytes_read);
    int (*close)(Vnode *vn);
} VnodeOperations;

typedef struct vnode
{
    VnodeType type;
    void *data;
    VnodeOperations ops;
} Vnode;

#define VOP_LOOKUP(vn, out, name) vn->ops.lookup(vn, out, name)
#define VOP_CREATE(vn, out, name, vattr) vn->ops.create(vn, out, name, vattr)
#define VOP_GETATTR(vn, out) vn->ops.getattr(vn, out)
#define VOP_OPEN(vn, mode) vn->ops.open(vn, mode)
#define VOP_READ(vn, buf, nbytes, off) vn->ops.read(vn, buf, nbytes, off)
#define VOP_WRITE(vn, buf, nbytes, off) vn->ops.write(vn, buf, nbytes, off)
#define VOP_MKDIR(vn, out, name, vattr) vn->ops.mkdir(vn, out, name, vattr)
#define VOP_READDIR(vn, buf, max_size, bytes_read) vn->ops.readdir(vn, buf, max_size, bytes_read)
#define VOP_CLOSE(vn) vn->ops.close(vn)

Vnode *vfs_open(Vnode *parent, char *path, int flags);
int vfs_read(Vnode *vn, void *buf, size_t nbyte, size_t off);
int vfs_write(Vnode *vn, void *buf, size_t nbyte, size_t off);
int vfs_find_and(Vnode *cwd, Vnode **out, const char *path, int flags, Vattr *attr);
int vfs_getdents(Vnode *vn, void *buf, size_t max_size, size_t *bytes_read);

int vfs_mkdir(Vnode *vn, Vnode **out, const char *name,
              Vattr *vattr);
int vfs_create(Vnode *vn, Vnode **out, const char *name, Vattr *vattr);
extern Vnode *root_vnode;

#endif