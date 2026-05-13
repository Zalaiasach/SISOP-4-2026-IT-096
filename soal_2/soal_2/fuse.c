#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>

static char dir_asli[4096];

void xor_cipher(char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        data[i] ^= 0x76;
    }
}

static int xmp_getattr(const char *path, struct stat *stbuf) {
    char fpath[1024];
    if (strcmp(path, "/") == 0) {
        sprintf(fpath, "%s", dir_asli);
    } else {
        sprintf(fpath, "%s%s.enc", dir_asli, path);
        if (access(fpath, F_OK) == -1) {
            sprintf(fpath, "%s%s", dir_asli, path); 
        }
    }
    int res = lstat(fpath, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];
    sprintf(fpath, "%s%s", dir_asli, path);
    DIR *dp = opendir(fpath);
    struct dirent *de;
    if (dp == NULL) return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        char name[256];
        strcpy(name, de->d_name);
        char *ext = strstr(name, ".enc");
        if (ext) *ext = '\0';

        if (filler(buf, name, &st, 0)) break;
    }
    closedir(dp);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    char fpath[1024];
    sprintf(fpath, "%s%s.enc", dir_asli, path);
    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;
    int res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;
    else xor_cipher(buf, res);
    close(fd);
    return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];
    sprintf(fpath, "%s%s.enc", dir_asli, path);
    int fd = open(fpath, O_WRONLY);
    if (fd == -1) return -errno;
    
    char *enc_buf = malloc(size);
    memcpy(enc_buf, buf, size);
    xor_cipher(enc_buf, size);
    
    int res = pwrite(fd, enc_buf, size, offset);
    free(enc_buf);
    if (res == -1) res = -errno;
    close(fd);
    return res;
}

static int xmp_mkdir(const char *path, mode_t mode) {
    char fpath[1024];
    sprintf(fpath, "%s%s", dir_asli, path);
    int res = mkdir(fpath, mode);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char fpath[1024];
    sprintf(fpath, "%s%s.enc", dir_asli, path);
    int res = open(fpath, fi->flags, mode);
    if (res == -1) return -errno;
    fi->fh = res;
    return 0;
}

static int xmp_rmdir(const char *path) {
    char fpath[1024];
    sprintf(fpath, "%s%s", dir_asli, path);
    int res = rmdir(fpath);
    if (res == -1) return -errno;
    return 0;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .read    = xmp_read,
    .write   = xmp_write,
    .mkdir   = xmp_mkdir,
    .create  = xmp_create,
    .rmdir   = xmp_rmdir,
};

int main(int argc, char *argv[]) {
    realpath("encrypted_storage", dir_asli);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}