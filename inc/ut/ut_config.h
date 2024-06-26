#pragma once
#include "ut/ut_std.h"

begin_c

// Persistent storage for configuration and other small data
// related to specific application.
// on Unix-like system ~/.name/key files are used.
// On Window User registry (could be .dot files/folders).
// "name" is customary basename of "ut_args.v[0]"

typedef struct {
    errno_t (*save)(const char* name, const char* key,
                    const void* data, int32_t bytes);
    int32_t (*size)(const char* name, const char* key);
    // load() returns number of actual loaded bytes:
    int32_t (*load)(const char* name, const char* key,
                    void* data, int32_t bytes);
    errno_t (*remove)(const char* name, const char* key);
    errno_t (*clean)(const char* name); // remove all subkeys
    void (*test)(void);
} ut_config_if;

extern ut_config_if ut_config;

end_c

