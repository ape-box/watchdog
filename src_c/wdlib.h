
/**
 * Watch Dog to prune unwanted files
 *
 * Alessio Peternelli <alessio.peternelli@gmail.com>
 */

#define MAX_EVENTS       1024                                     /* Max N° to process */
#define LEN_NAME         16                                       /* Assuming filename won't exceed 16 Byte */
#define EVENT_SIZE       (sizeof(struct inotify_event))
#define BUF_LEN          (MAX_EVENTS * (EVENT_SIZE + LEN_NAME))   /* Buffer to store the data of events */
#define OPT_MV_PATH      "--moveto="                              /* Option to move files to path instead of deleting them */


int   wd_poll[MAX_EVENTS];                                        /* All watchers poll */
char* wd_path[MAX_EVENTS];                                        /* All watchers paths */
int   wd_index = -1;                                              /* Watch counter */

int   fd_main;                                                    /* Main inotify's file descriptor */
char* moveto = NULL;                                              /* path to directory where EVENTUALLY store unwanted files */


int   bark_at(int file_descriptor, char* door);                   /* Add watch */
void  memorize_wd(int wd, char* path);                            /* Save wd and path relations */
void  forget_wd(int wd);                                          /* remove watcher, free path's memory and reorder stack */
void  pin_subdirectories(int file_descriptor, char* root);
char* pathdup_addsubdir(char* path, char* subdir);
int   find_wd_index(int wd);
void  clean_garbage();
char* getcurrenttime();
