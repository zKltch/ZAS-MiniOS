#ifndef SCRIPT_ENGINE_H
#define SCRIPT_ENGINE_H

#include <stdint.h>

/*
 * cat
 * echo
 * sh
 * mkdir
 * cd
 * ls
*/

// static inline void read_space(const char **s)
// => Filter Space 

// static inline void filter_args(char *dst, const char *src, uint32_t *len)
// => Get Length

void cat_file(const char *FileName);

void echo_write_file(const char *args);

void cmd_script(const char *script_name);

void create_dir(const char *DirName); // Use Create File To Make Management Simple 

void change_directory(const char *DirName);

void cmd_ls(void);

#endif
