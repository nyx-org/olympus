#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <posix/dirent.h>
#include <posix/posix.h>

extern VnodeOperations tmpfs_ops;

static int tmpfs_make_vnode(Vnode **out, TmpNode *node)
{
    if (node->vnode)
    {
        *out = node->vnode;
        return 0;
    }
    else
    {
        Vnode *vn = ichor_malloc(sizeof(Vnode));
        node->vnode = vn;
        vn->type = node->attr.type;
        vn->ops = tmpfs_ops;
        vn->data = node;
        *out = vn;
        return 0;
    }
}

static TmpNode *tmpfs_make_node(TmpNode *dir, VnodeType type, const char *name, Vattr *attr)
{
    TmpNode *n = ichor_malloc(sizeof(TmpNode));
    TmpDirent *dirent = ichor_malloc(sizeof(TmpDirent));

    char *allocated_name_str = ichor_malloc(strlen(name));
    strncpy(allocated_name_str, name, strlen(name));

    dirent->name = allocated_name_str;
    dirent->node = n;

    n->attr = attr ? *attr : (Vattr){0};

    n->attr.type = type;
    n->attr.size = 0;
    n->vnode = NULL;

    switch (type)
    {
    case VREG:
        n->reg.buffer = NULL;
        break;

    case VDIR:
        vec_init(&n->dir.entries);
        n->dir.parent = dir;
        break;

    default:
        break;
    }

    vec_push(&dir->dir.entries, dirent);

    return n;
}

static TmpDirent *lookup_dirent(TmpNode *vn, const char *name)
{
    for (size_t i = 0; i < vn->dir.entries.length; i++)
    {
        if (strncmp(vn->dir.entries.data[i]->name, name, strlen(vn->dir.entries.data[i]->name)) == 0)
            return vn->dir.entries.data[i];
    }

    return NULL;
}

static int tmpfs_lookup(Vnode *vn, Vnode **out, const char *name)
{

    TmpNode *node = (TmpNode *)(vn->data);
    TmpDirent *ent;

    if (node->attr.type != VDIR)
    {
        return -ENOTDIR;
    }

    ent = lookup_dirent(node, name);

    if (!ent)
    {
        return -ENOENT;
    }

    return tmpfs_make_vnode(out, ent->node);
}

static int tmpfs_create(Vnode *vn, Vnode **out, const char *name, Vattr *attr)
{

    // ichor_debug("creating file in %p", vn);

    if (vn->type != VDIR)
    {
        return -ENOTDIR;
    }

    TmpNode *n;

    n = tmpfs_make_node((TmpNode *)vn->data, VREG, name, attr);

    return tmpfs_make_vnode(out, n);
}

static int tmpfs_read(Vnode *vn, void *buf, size_t nbyte, size_t off)
{
    TmpNode *tn = (TmpNode *)vn->data;

    if (tn->attr.type != VREG)
    {
        return tn->attr.type == VDIR ? -EISDIR : -EINVAL;
    }

    if (off + nbyte > tn->attr.size)
    {
        nbyte = (tn->attr.size <= off) ? 0 : tn->attr.size - off;
    }

    if (nbyte == 0)
        return 0;

    memcpy(buf, tn->reg.buffer + off, nbyte);

    return nbyte;
}

static int tmpfs_write(Vnode *vn, void *buf, size_t nbyte, size_t off)
{
    TmpNode *tn = (TmpNode *)vn->data;

    bool resize = false;

    if (tn->attr.type != VREG)
    {
        return tn->attr.type == VDIR ? -EISDIR : -EINVAL;
    }

    if (off + nbyte > tn->attr.size)
    {
        tn->attr.size = off + nbyte;
        resize = true;
    }

    if (!tn->reg.buffer)
    {
        tn->reg.buffer = ichor_malloc(tn->attr.size);
    }
    else if (resize)
    {
        tn->reg.buffer = ichor_realloc(tn->reg.buffer, tn->attr.size);
    }

    memcpy(tn->reg.buffer + off, buf, nbyte);

    return nbyte;
}

static int tmpfs_make_new_dir(Vnode *vn, Vnode **out, const char *name, Vattr *vattr)
{
    TmpNode *n;
    if (vn->type != VDIR)
    {
        return -ENOTDIR;
    }

    n = tmpfs_make_node(vn->data, VDIR, name, vattr);

    if (!n)
    {
        POSIX_PANIC("tmpfs_make_node returned NULL");
    }

    return tmpfs_make_vnode(out, n);
}

static int tmpfs_mkdir(Vnode *vn, Vnode **out, const char *name,
                       Vattr *vattr)
{
    int r;
    Vnode *n;
    TmpNode *tn;

    r = tmpfs_make_new_dir(vn, out, name, vattr);

    r = tmpfs_make_new_dir(*out, &n, ".", NULL);

    tn = (TmpNode *)(*out)->data;

    tn->attr.type = VDIR;
    n->type = VDIR;

    n->data = tn;

    r = tmpfs_make_new_dir(*out, &n, "..", NULL);

    n->data = tn->dir.parent;

    return r;
}

static int tmpfs_getattr(Vnode *vn, Vattr *attr)
{
    TmpNode *tn = (TmpNode *)vn->data;

    if (!attr)
        return -EINVAL;

    *attr = tn->attr;
    return 0;
}

static int tmpfs_readdir(Vnode *vn, void *buf, size_t max_size, size_t *bytes_read)
{
    TmpNode *tn = (TmpNode *)vn->data;

    if (tn->attr.type != VDIR)
    {
        return -ENOTDIR;
    }

    struct dirent *dent = buf;
    size_t bytes_written = 0;

    for (size_t i = 0; i < tn->dir.entries.length; i++)
    {
        if (bytes_written + sizeof(struct dirent) > max_size)
        {
            break;
        }

        TmpDirent *tdent = tn->dir.entries.data[i];

        if (tdent->node->attr.type == VDIR)
            dent->d_type = DT_DIR;
        else if (tdent->node->attr.type == VREG)
            dent->d_type = DT_REG;
        else
            dent->d_type = DT_UNKNOWN;
        dent->d_ino = (uintptr_t)tdent->node;
        dent->d_off++;
        dent->d_reclen = sizeof(struct dirent);

        strncpy(dent->d_name, tdent->name, strlen(tdent->name));

        dent = (void *)dent + sizeof(struct dirent);

        bytes_written += sizeof(struct dirent);
    }

    if (bytes_read)
        *bytes_read = bytes_written;

    return 0;
}

VnodeOperations tmpfs_ops = {
    .create = tmpfs_create,
    .mkdir = tmpfs_mkdir,
    .read = tmpfs_read,
    .write = tmpfs_write,
    .getattr = tmpfs_getattr,
    .readdir = tmpfs_readdir,
    .lookup = tmpfs_lookup};

void tmpfs_init(void)
{
    TmpNode *root_node = ichor_malloc(sizeof(TmpNode));
    root_node->attr.type = VDIR;
    root_node->vnode = NULL;

    Vnode *n;

    tmpfs_make_vnode(&root_vnode, root_node);

    root_node->vnode = root_vnode;
    root_vnode->type = VDIR;

    tmpfs_make_new_dir(root_vnode, &n, "..", NULL);

    n->data = root_node;

    tmpfs_make_new_dir(root_vnode, &n, ".", NULL);

    n->data = root_node;
}