
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
#include <iso646.h>                                             /* OPTIONAL HEADER for using lessical logical operator */

#define MAX_PATH_LEN   1024                                     /* Max path length */
#define MAX_SUBDIR_LVL 4                                        /* Max level of subdirectories to watch */
#define MAX_EVENTS     1024                                     /* Max N° to process */
#define LEN_NAME       16                                       /* Assuming filename won't exceed 16 Byte */
#define EVENT_SIZE     (sizeof(struct inotify_event))
#define BUF_LEN        (MAX_EVENTS * (EVENT_SIZE + LEN_NAME))   /* Buffer to store the data of events */
#define MV_PATH        "--moveto="                              /* Option to move files to path instead of deleting them */

FILE *fp_log;                                                   /* File pointer for loggin purposes */

void usage();                                                   /* HELP Function */
int bark_at(int file_descriptor, char *door);                   /* Add watch */
int[] pin_subdirectories(int file_descriptor, char *root);

int main (int argc, char **argv)
{
    int length, watch_descriptor, file_descriptor;
    int ac, i = 0;
    char buffer[BUF_LEN];
    char *tmp_path;

    /**
     * Initialize Inotify
     */
    file_descriptor = inotify_init();
    if (file_descriptor < 0) {
        perror("inotify init error: ");
        close(file_descriptor);
        exit(EXIT_FAILURE);
    }

    /**
     * Argument parsing
     * -----------------------------------------------------------
     * maybe in a later time it would be nice to implement getopt
     */
    for (ac=1;ac<argc;ac++)
    {
        /**
         * OPTION: move to path instead of deleting
         */
        if (0 == strncmp(argv[ac], MV_PATH, sizeof(MV_PATH) -1))
        {
            tmp_path = &argv[ac][sizeof(MV_PATH)-1];
            printf("Set option to move files to %s\n", tmp_path);
            ;/* DA IMPLEMENTARE */
        }
        /**
         * OPTION: HELP
         */
        else if (0 == strcmp(argv[ac], "--help"))
        {
            usage();
            close(file_descriptor);
            exit(EXIT_SUCCESS);
        }
        /**
         * OPTION: DEFAULT ADD PATH
         */
        else
        {
            /* Add inotify watch to path */
            bark_at(file_descriptor, argv[ac]);
            printf("Added path: %s\n", argv[ac]);
            ;/* DA IMPLEMENTARE */
        }
    }

    /**
     * Main Loop
     * ==========================================================================================================
     */
    while (1)
    {
        i = 0;
        length = read(file_descriptor, buffer, BUF_LEN);

        if (length < 0) {
            perror("error while reading");
        }

        while (i < length)
        {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            if (event->len)
            {
                if (event->mask bitand IN_CREATE)
                {
                    if (event->mask bitand IN_ISDIR) printf("The directory %s was Created.\n", event->name);
                    else printf("The file %s was Created with WD %d\n", event->name, event->wd);
                }

                if (event->mask bitand IN_MODIFY)
                {
                    if (event->mask bitand IN_ISDIR) printf("The directory %s was Modified.\n", event->name);
                    else printf("The file %s was Modified with WD %d\n", event->name, event->wd);
                }

                if (event->mask bitand IN_DELETE)
                {
                    if (event->mask bitand IN_ISDIR) printf("The directory %s was Deleted.\n", event->name);
                    else printf("The file %s was Deleted with WD %d\n", event->name, event->wd);
                }

                i += EVENT_SIZE + event->len;
            }
        }
    }

    /* Clean up */
    inotify_rm_watch(file_descriptor, watch_descriptor);
    close(file_descriptor);

    exit(EXIT_SUCCESS);
}

void usage ()
{
    printf("\
OPTIONS:\
 %s[path] # Specify a path where MOVE INSTEAD OF DELETE unwanted files\n\
 [path]          # Path to watch for unwanted files\n\
\n", MV_PATH);
}

/**
 * Add a inotify watch to path
 * return the watch descriptor
 */
int bark_at(int file_descriptor, char *door)
{
    int watch_descriptor;
    watch_descriptor = inotify_add_watch(file_descriptor, door, IN_CREATE | IN_MODIFY);
    if (watch_descriptor == -1)
    {
        printf("Couldn't add watch to %s\n", door);
    }
    else
    {
        printf("Watching: %s\n", door);
    }

    return watch_descriptor;
}


/**
 * Traverse all subdirectories and add a inotify watch to every sub directory
 * return the list of all watch descriptors
 */
int[] pin_subdirectories(int file_descriptor, char *root)
{
    ;
}

