#pragma once
#include "manifest.h"

begin_c

typedef struct file_s file_t;

typedef struct file_stat_s {
    uint64_t created;
    uint64_t accessed;
    uint64_t updated;
    int64_t size; // bytes
} file_stat_t;

typedef struct {
    file_t* const invalid; // (file_t*)-1
    // seek() methods:
    int32_t const seek_set;
    int32_t const seek_cur;
    int32_t const seek_end;
    // open() flags
    int32_t const o_rd; // read only
    int32_t const o_wr; // write only
    int32_t const o_rw; // != (o_rd | o_wr)
    int32_t const o_append;
    int32_t const o_create; // opens existing or creates new
    int32_t const o_excl;   // create fails if file exists
    int32_t const o_trunc;  // open always truncates to empty
    int32_t const o_sync;
    errno_t (*open)(file_t* *file, const char* filename, int32_t flags);
    bool    (*is_valid)(file_t* file); // checks both null and invalid
    errno_t (*seek)(file_t* file, int64_t *position, int32_t method);
    void    (*stat)(file_t* file, file_stat_t* stat);
    errno_t (*read)(file_t* file, void* data, int64_t bytes, int64_t *transferred);
    errno_t (*write)(file_t* file, const void* data, int64_t bytes, int64_t *transferred);
    errno_t (*flush)(file_t* file);
    void    (*close)(file_t* file);
    errno_t (*write_fully)(const char* filename, const void* data,
                           int64_t bytes, int64_t *transferred);
    bool (*exists)(const char* pathname); // does not guarantee any access writes
    bool (*is_folder)(const char* pathname);
    bool (*is_symlink)(const char* pathname);
    errno_t (*mkdirs)(const char* pathname); // tries to deep create all folders in pathname
    errno_t (*rmdirs)(const char* pathname); // tries to remove folder and its subtree
    errno_t (*create_tmp)(char* file, int32_t count); // create temporary file
    errno_t (*chmod777)(const char* pathname); // and whole subtree new files and folders
    // cwd() shall return absolute pathname of the current working directory or null
    const char* (*cwd)(char* wd, int32_t count);
    errno_t (*setcwd)(const char* wd); // set working directory
    errno_t (*symlink)(const char* from, const char* to); // sym link "ln -s" *)
    errno_t (*link)(const char* from, const char* to); // hard link like "ln"
    errno_t (*unlink)(const char* pathname); // delete file or empty folder
    errno_t (*copy)(const char* from, const char* to); // allows overwriting
    errno_t (*move)(const char* from, const char* to); // allows overwriting
    void (*test)(void);
} files_if;

// *) symlink on Win32 is only allowed in Admin elevated
//    processes and in Developer mode.

extern files_if files;

end_c
