#include <fs/vfs.h>
#include <posix/fcntl.h>
#include <posix/posix.h>

int posix_sys_open(Proc *proc, const char *path, int mode)
{
    File new_file;
    Vnode *vn;
    int r;

    r = vfs_find_and(proc->cwd, &vn, path, VFS_FIND_OR_ERROR, NULL);

    if (r < 0 && mode & O_CREAT)
    {
        r = vfs_find_and(proc->cwd, &vn, path, VFS_FIND_AND_CREATE, NULL);

        if (r < 0)
            return r;
    }

    if (vn->ops.open)
    {
        r = VOP_OPEN(vn, mode);

        if (r < 0)
            return r;
    }

    new_file.vnode = vn;
    new_file.position = 0;
    new_file.fd = proc->current_fd++;

    vec_push(&proc->fds, new_file);

    return new_file.fd;
}

int posix_sys_read(Proc *proc, int fd, void *buf, size_t bytes)
{
    File *file = NULL;
    int r;

    for (size_t i = 0; i < proc->fds.length; i++)
    {
        if (fd == proc->fds.data[i].fd)
        {
            file = &proc->fds.data[i];
            break;
        }
    }

    if (!file)
    {
        return -EBADF;
    }

    r = vfs_read(file->vnode, buf, bytes, file->position);

    if (r < 0)
        return r;

    file->position += r;

    return r;
}

int posix_sys_write(Proc *proc, int fd, void *buf, size_t bytes)
{
    File *file = NULL;
    int r;

    for (size_t i = 0; i < proc->fds.length; i++)
    {
        if (fd == proc->fds.data[i].fd)
        {
            file = &proc->fds.data[i];
            break;
        }
    }

    if (!file)
    {
        return -EBADF;
    }

    r = vfs_write(file->vnode, buf, bytes, file->position);

    if (r < 0)
        return r;

    file->position += r;

    return r;
}

int posix_sys_seek(Proc *proc, int fd, off_t offset, int whence)
{
    File *file = NULL;
    int r;

    for (size_t i = 0; i < proc->fds.length; i++)
    {
        if (fd == proc->fds.data[i].fd)
        {
            file = &proc->fds.data[i];
            break;
        }
    }

    if (!file)
    {
        return -EBADF;
    }

    switch (whence)
    {
    case SEEK_SET:
        file->position = offset;
        break;
    case SEEK_CUR:
        file->position += offset;
        break;
    case SEEK_END:
    {
        Vattr attr;
        r = VOP_GETATTR(file->vnode, &attr);

        if (r < 0)
            return -1;
        file->position = attr.size + offset;
        break;
    }
    }

    return file->position;
}

int posix_sys_close(Proc *proc, int fd)
{
    File *file = NULL;
    size_t index = 0;

    for (size_t i = 0; i < proc->fds.length; i++)
    {
        if (fd == proc->fds.data[i].fd)
        {
            file = &proc->fds.data[i];
            index = i;
            break;
        }
    }

    if (!file)
    {
        return -EBADF;
    }

    for (size_t i = index; i < proc->fds.length; ++i)
        proc->fds.data[i] = proc->fds.data[i + 1];

    return 0;
}

int posix_sys_stat(Proc *proc, int fd, const char *path, struct stat *out)
{

    int r;
    Vnode *vn;
    Vattr attr;

    if (fd == AT_FDCWD)
    {

        r = vfs_find_and(proc->cwd, &vn, path, VFS_FIND_OR_ERROR, NULL);

        if (r < 0)
            return r;
    }

    else
    {
        File *file = NULL;

        for (size_t i = 0; i < proc->fds.length; i++)
        {
            if (fd == proc->fds.data[i].fd)
            {
                file = &proc->fds.data[i];
                break;
            }
        }

        if (!file)
            return -EBADF;

        if (path && strlen(path) > 0)
        {
            r = vfs_find_and(file->vnode, &vn, path, VFS_FIND_OR_ERROR, NULL);

            if (r < 0)
                return r;
        }
        else
        {
            vn = file->vnode;
        }
    }

    if (vn->ops.getattr)
        r = vn->ops.getattr(vn, &attr);
    if (r < 0)
        return r;

    memset(out, 0, sizeof(struct stat));
    out->st_mode = attr.mode;

    switch (attr.type)
    {
    case VREG:
        out->st_mode |= S_IFREG;
        break;
    case VDIR:
        out->st_mode |= S_IFDIR;
        break;
    default:
        POSIX_PANIC("Unknown attr");
        break;
    }

    out->st_blksize = 512;
    out->st_size = attr.size;
    out->st_blocks = attr.size / 512;

    return 0;
}

int posix_sys_readdir(Proc *proc, int fd, void *buf, size_t max_size, size_t *bytes_read)
{
    File *file = NULL;
    int r;
    Vattr attr;

    for (size_t i = 0; i < proc->fds.length; i++)
    {
        if (fd == proc->fds.data[i].fd)
        {
            file = &proc->fds.data[i];
            break;
        }
    }

    if (!file)
        return -EBADF;

    r = VOP_GETATTR(file->vnode, &attr);

    if (r < 0)
        return r;

    if (attr.type != VDIR)
        return -ENOTDIR;

    r = VOP_READDIR(file->vnode, buf, max_size, bytes_read);

    return r;
}