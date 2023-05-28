#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include "fileLib.h"

#define MAX_BUFFER_SIZE 1024
#define MAX_FILES 1024
#define MAX_DOC 1024
#define PATH_MAX 256
#define NAME_LENTH 256

struct node* root;

struct node* get_node(const char *path){
    puts("get_document");
    printf("%s\n",path);
    struct node* target_doc = root;
    if(strlen(path) == 1)return root;
    int len = strlen(path);
    for(int l = 1,r; l < len; l = r + 1){
        r = l;
        while(r < len && path[r] != '/')r++;
        char *name = (char*)malloc(sizeof(char)*(r - l + 1));
        for(int i = 0; i < r - l; i++)name[i] = path[i + l];
        name[r - l] = '\0';
        target_doc = find_node(target_doc,name);
        free(name);
        if(!target_doc)return NULL;
        
    }
    return target_doc;
}

static int create_node(struct node* fa, char* name){
    struct node* new_file = (struct node*)malloc(sizeof(struct node));
    if(strlen(name) >= 255)return -EFBIG;
    strcpy(new_file->name,name);
    new_file->contents = "";
    new_file->father = fa;
    new_file->next = NULL;
    new_file->prev = NULL;
    new_file->sons = NULL;
    new_file->type = file;
    if(insert_file(fa,new_file) == 0)return 0;
    else return -EEXIST;
}

static void *chat_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	(void) conn;
	cfg->kernel_cache = 1;
    root = (struct node*) malloc(sizeof(struct node));
    root->contents = NULL;
    root->father = NULL;
    strcpy(root->name,"root");
    root->next = NULL;
    root->prev = NULL;
    root->sons = NULL;
    root->type = directory;

	return NULL;
}


static int chat_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    (void) fi;
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));

    struct node *target_file = get_node(path);
    if(target_file != NULL)printf("%s\n",target_file->name);
    if(target_file == NULL){
        res = -ENOENT;
    }
    else if(target_file->type == directory){// a directory
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else{   //a file
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        if(target_file->contents)stbuf->st_size = strlen(target_file->contents);
        else stbuf->st_size = 0;
    }
    return res;
}

static int chat_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flag)
{
    (void)offset;
    (void)fi;

    struct node* tmp = get_node(path);
    if(tmp == NULL)return -ENOENT;
    if(tmp->type == file)return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    struct node* head = tmp->sons;
    if(head == NULL)puts("!!!");
    while(head){
        printf("%s\n",head->name);
        filler(buf, head->name, NULL, 0, 0);
        head = head->next;
    }
    return 0;
}


static int chat_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi)
{
    puts("read");

    printf("size: %lu buf:%ld\n",size, sizeof(buf));
    struct node* target_file = get_node(path);
    if(target_file == NULL)return -ENOENT;
    if(target_file->type == directory)return -ENOENT;

    if(target_file->contents == NULL)return 0;
    printf("name: %s\n",target_file->name);

    //printf("%s size: %ld offset: %ld\n",target_file->contents, size,offset);
    int len = strlen(target_file->contents);
    if(offset < len){
        if(offset + size > len)size = len - offset;
        memcpy(buf,target_file->contents + offset, size);
    }
    else size = 0;
    return size;
}

static int chat_write(const char *path, const char *buf, size_t size,
                      off_t offset, struct fuse_file_info *fi)
{
    puts("write");
    printf("offset: %lu\n",offset);
    struct node* target_file = get_node(path);
    if(target_file == NULL || target_file->type == directory)return -ENOENT;
    struct node* bro = find_node(root,target_file->name);
    if(bro == NULL){
        char* new_content = (char*)malloc(sizeof(char)*(size + offset + 1));
        memcpy(new_content,target_file->contents,offset);
        memcpy(new_content + offset,buf,size);
        target_file->contents = new_content;
    }
    else{
        struct node* bro_target_file = find_node(bro,target_file->father->name);
        if(bro_target_file == NULL)create_node(bro,target_file->father->name);
        bro_target_file = find_node(bro,target_file->father->name);

        offset = strlen(bro_target_file->contents);
        char* new_content = (char*)malloc(sizeof(char)*(size + offset + 1));
        memcpy(new_content,bro_target_file->contents,offset);
        memcpy(new_content + offset,buf,size);
        bro_target_file->contents = new_content;
    }
    
    return size;
}

static int chat_makedir(const char *path, mode_t mode){
    puts("makedir");
    char* new_path = (char*)malloc(sizeof(char)*(strlen(path)+1));
    strcpy(new_path,path);
    new_path[strlen(new_path)-1] = '\0';
    for(int i = strlen(new_path)-1;i;i--){
        if(new_path[i]!='/')new_path[i] = '\0';
        else break;
    }
    printf("%s\n",new_path);
    struct node* fa = get_node(new_path);
    free(new_path);

    if(fa == NULL || fa->type == file)return -ENOENT;
    char doc_name[strlen(path) + 1];
    strcpy(doc_name,path);
    if(doc_name[strlen(doc_name)-1] == '/')doc_name[strlen(doc_name)-1] = '\0';
    char* name = strrchr(doc_name,'/') + 1;
    struct node* newdir = (struct node *)malloc(sizeof(struct node));
    strcpy(newdir->name,name);
    newdir->contents = NULL;
    newdir->father = fa;
    newdir->type = directory;
    newdir->sons = NULL;
    newdir->next = NULL;
    newdir->prev = NULL;
    if(insert_file(fa,newdir)==0)return 0;
    else return -EEXIST;
}

static int chat_rm(const char *path){
    struct node* rm = get_node(path);
    if(rm == NULL)return -ENOENT;
    if(rm->type == directory)return -ENOENT;
    if(remove_file(rm->father,rm->name) == 0)return 0;
    else return -ENOENT;
}

static int chat_create(const char *path, mode_t mode,struct fuse_file_info *fi){
    puts("chat_create");
    printf("%s\n",path);
    char* new_path = (char*)malloc(sizeof(char)*(strlen(path)+1));
    strcpy(new_path,path);
    new_path[strlen(new_path)-1] = '\0';
    for(int i = strlen(new_path)-1;i;i--){
        if(new_path[i]!='/')new_path[i] = '\0';
        else break;
    }
    printf("%s\n",new_path);
    struct node* fa = get_node(new_path);
    free(new_path);
    if(fa == NULL){
        puts("???");
        return -ENOENT;
    }
    char file_name[strlen(path) + 1];
    strcpy(file_name,path);
    if(file_name[strlen(file_name)-1] == '/')return -EISDIR;
    char* name = strrchr(file_name,'/') + 1;
    return create_node(fa,name);
}

static int chat_rmdir(const char *path){
    struct node* rm  = get_node(path);
    if(rm == NULL)return -ENOENT;
    if(rm->type == file)return -ENOTDIR;
    if(remove_file(rm->father,rm->name) == 0)return 0;
    else return -ENOENT;
}

static int chat_utimens (const char *path, const struct timespec tv[2],
			 struct fuse_file_info *fi){
    return 0;
}

/*
    =====  OPERATION: ====
    cd      : getattr + readdir
    ls      : readdir
    mkdir   : mkdir
    touch   : getattr + mknod
    cat     : getattr + read
    echo    : getattr + write
    mkdir   : mkdir
    rmdir   : rmdir
    rm      : unlink
*/

static struct fuse_operations chat_oper = {
    .init = chat_init,          //init file system
    .getattr = chat_getattr,    // cd
    .readdir = chat_readdir,    // ls
    .mkdir = chat_makedir,      // mkdir
    .read = chat_read,          //cat
    .write = chat_write,        //echo
    .unlink = chat_rm,          //rm
    .create = chat_create,      //touch
    .rmdir = chat_rmdir,        //rmdir
    .utimens = chat_utimens
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &chat_oper, NULL);
}