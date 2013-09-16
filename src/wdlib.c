
/**
 * Watch Dog to prune unwanted files
 *
 * Alessio Peternelli <alessio.peternelli@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <iso646.h>                                               /* OPTIONAL HEADER for using lessical logical operator */
#include "wdlib.h"


int   fd_main;                                                    /* Main inotify's file descriptor */
int   wd_poll[MAX_EVENTS];                                        /* All watchers poll */
char* wd_path[MAX_EVENTS];                                        /* All watchers paths */
int   wd_index = -1;                                              /* Watch counter */


int dog_init()
{
    /**
     * Initialize Inotify
     */
    fd_main = inotify_init();
    if (fd_main < 0) {
        perror("inotify init error");
        clean_garbage();
        exit(EXIT_FAILURE);
    }

    /**
     * Initialize wd stacks
     */
    for (int i = 0; i <= MAX_EVENTS; i++) {
        wd_poll[i] = -1;
        wd_path[i] = NULL;
    }

    return fd_main;
}


/**
 * Add a inotify watch to path
 * return the watch descriptor
 */
int bark_at(int file_descriptor, char* door)
{
    if (wd_index == (MAX_EVENTS-1)) {
        perror("MAX_EVENTS reached, can't add more watch descriptors");

        return -1;
    }

    int watch_descriptor;
    watch_descriptor = inotify_add_watch(file_descriptor, door, IN_CREATE bitor IN_DELETE);
    if (watch_descriptor == -1) {
        printf("[%s] Couldn't add watch to %s\n", getcurrenttime(), door);
    }
    else
    {
        printf("[%s] Watching path %s\n", getcurrenttime(), door);
        memorize_wd(watch_descriptor, door);
    }

    return watch_descriptor;
}

/**
 * Add to stack: watch descriptor and path
 * Increment index
 */
void memorize_wd(int wd, char* path)
{
    wd_index += 1;

    if (wd_index < MAX_EVENTS) {
        wd_poll[wd_index] = wd;
        wd_path[wd_index] = strdup(path);
    }
    else {
        perror("MAX_EVENTS reached, can't add more watch descriptors");
    }
}

/**
 * Traverse all subdirectories and add a inotify watch to every sub directory
 * return the list of all watch descriptors
 */
void pin_subdirectories(int file_descriptor, char* root)
{
    int watch_descriptor;
    char* abs_dir;
    struct dirent *entry;
    DIR *dp;

    dp = opendir(root);
    if (dp == NULL)
    {
        printf("[%s] Could not open dir %s\n", getcurrenttime(), root);
        clean_garbage();
        exit(EXIT_FAILURE);
    }

    watch_descriptor = bark_at(file_descriptor, root);
    if (watch_descriptor == -1) {
        perror("Unable to add watch");
        clean_garbage();
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dp)))
    {
        if ((entry->d_type & DT_DIR) and (0 != strcmp(entry->d_name, ".") and 0 != strcmp(entry->d_name, "..")))
        {
            /* Add watches to the level 1 subdirs */
            abs_dir = pathdup_addsubdir(root, entry->d_name);
            pin_subdirectories(file_descriptor, abs_dir);
            free(abs_dir);
        }
    }

    closedir(dp);
}

/**
 * Given path, verify for separator, append subidr, return pointer to new allocated memory
 */
char* pathdup_addsubdir(char* path, char* subdir)
{
    int addslash = (path[strlen(path)-1] != '/');
    char* newpath = NULL;
    int k = addslash ? 2 : 1;

    newpath = malloc(strlen(path) + strlen(subdir) + k);

    strcpy(newpath, path);
    if (addslash) {
        strcat(newpath, "/");
    }

    strcat(newpath, subdir);

    return newpath;
}

/**
 * Find watcher's index from descriptor
 */
int find_wd_index(int wd)
{
    for (int j = 0; j <= wd_index; j++) {
        if (wd_poll[j] == wd) {
            return j;
        }
    }

    return -1;
}

/**
 * Proxy to wd_path
 */
char* get_barking_path(int wd)
{
    int index = find_wd_index(wd);

    return wd_path[index];
}

/**
 * Remove inotify watcher, free path's memory, reduce index and reorder stacks
 */
void forget_wd(int wd)
{
    int descriptor_index;

    descriptor_index = find_wd_index(wd);
    inotify_rm_watch(fd_main, wd);
    free(wd_path[descriptor_index]);

    for (; descriptor_index <= wd_index; ++descriptor_index)
    {
        wd_poll[descriptor_index - 1] = wd_poll[descriptor_index];
        wd_path[descriptor_index - 1] = wd_path[descriptor_index];
    }
    wd_poll[descriptor_index] = -1;
    wd_path[descriptor_index] = NULL;

    wd_index -= 1;
}


/**
 * Remove inotidy watchers and free allocated memory
 */
void clean_garbage()
{
    if (wd_index >= 0)                                 // if we have at least one watcher ...
    {
        for (int i = 0; i <= wd_index; i++) {
            if (wd_poll[i] >= 0) {
                inotify_rm_watch(fd_main, wd_poll[i]); // remove watcher
            }
            if (wd_path[i] != NULL) {
                free(wd_path[i]);                      // free path
            }
        }
    }

    close(fd_main);
}

char* getcurrenttime()
{
    time_t curtime;
    char* buffer;

    time (&curtime);
    buffer = ctime(&curtime);
    buffer[strlen(buffer)-1] = ';';

    return buffer;
}
