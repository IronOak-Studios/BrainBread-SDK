/*
 * LD_PRELOAD shim for 32-bit GoldSrc (HLDS) on 64-bit Linux hosts.
 *
 * The GoldSrc engine (engine_i486.so) uses 32-bit _stat / __xstat calls
 * whose struct stat cannot represent inode numbers larger than 2^32.
 * Container and overlay filesystems (overlayfs, Docker's storage driver,
 * etc.) routinely hand out 64-bit inodes, causing _stat to fail with
 * EOVERFLOW.  The engine then refuses to read liblist.gam and cannot
 * discover the game DLL.
 *
 * This shim interposes __xstat and __lxstat, forwarding to their stat64
 * counterparts so that the call succeeds (inode is truncated, but the
 * engine never inspects it).
 *
 * Build (done automatically by the Makefile):
 *   gcc -m32 -shared -fPIC -o stat_fix.so stat_fix.c
 *
 * Usage:
 *   LD_LIBRARY_PATH="$HLDS_DIR" \
 *   /lib32/ld-linux.so.2 --preload /path/to/stat_fix.so \
 *       "$HLDS_DIR/hlds_linux" -game brainbread +map <map> ...
 *
 * Or simply use the provided linux/hlds_run.sh wrapper.
 */
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

/* Override __xstat — glibc's _stat() implementation on i386. */
int __xstat(int ver, const char *path, struct stat *buf)
{
    struct stat64 buf64;
    int ret;

    ret = stat64(path, &buf64);
    if (ret != 0)
        return ret;

    memset(buf, 0, sizeof(*buf));
    buf->st_dev     = buf64.st_dev;
    buf->st_ino     = (ino_t)buf64.st_ino;
    buf->st_mode    = buf64.st_mode;
    buf->st_nlink   = buf64.st_nlink;
    buf->st_uid     = buf64.st_uid;
    buf->st_gid     = buf64.st_gid;
    buf->st_rdev    = buf64.st_rdev;
    buf->st_size    = (off_t)buf64.st_size;
    buf->st_blksize = buf64.st_blksize;
    buf->st_blocks  = (blkcnt_t)buf64.st_blocks;
    buf->st_atime   = buf64.st_atime;
    buf->st_mtime   = buf64.st_mtime;
    buf->st_ctime   = buf64.st_ctime;

    return 0;
}

/* Override __lxstat — glibc's lstat() implementation on i386. */
int __lxstat(int ver, const char *path, struct stat *buf)
{
    struct stat64 buf64;
    int ret;

    ret = lstat64(path, &buf64);
    if (ret != 0)
        return ret;

    memset(buf, 0, sizeof(*buf));
    buf->st_dev     = buf64.st_dev;
    buf->st_ino     = (ino_t)buf64.st_ino;
    buf->st_mode    = buf64.st_mode;
    buf->st_nlink   = buf64.st_nlink;
    buf->st_uid     = buf64.st_uid;
    buf->st_gid     = buf64.st_gid;
    buf->st_rdev    = buf64.st_rdev;
    buf->st_size    = (off_t)buf64.st_size;
    buf->st_blksize = buf64.st_blksize;
    buf->st_blocks  = (blkcnt_t)buf64.st_blocks;
    buf->st_atime   = buf64.st_atime;
    buf->st_mtime   = buf64.st_mtime;
    buf->st_ctime   = buf64.st_ctime;

    return 0;
}
