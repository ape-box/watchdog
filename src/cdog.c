
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

char* unwanted_files[] = {".gif", ".php"};                        /* ... */
char  moveto[PATH_MAX];                                           /* path to directory where EVENTUALLY store unwanted files */

/**
 * Print help message
 */
void usage ()
{
    printf("\
OPTIONS:\
 %s[path] # Specify a path where MOVE INSTEAD OF DELETE unwanted files\n\
 [path]          # Path to watch for unwanted files\n\
\n", OPT_MV_PATH);
}

void sig_handler(int s)
{
    switch (s)
    {
        case SIGINT:
        case SIGKILL:
        case SIGQUIT:
        case SIGSTOP:
        case SIGTERM:
        case SIGTSTP:
            printf("[%s] Exiting process\n", getcurrenttime());
            clean_garbage();
            exit(EXIT_SUCCESS);
            break;
        default:
            printf("[%s] Caught unused signal: %d\n", getcurrenttime(), s);
            break;
    }
}

int main (int argc, char** argv)
{
    /**
     * See Main Loop
     */
    int  buffer_length, buffer_counter = 0;
    char buffer[BUF_LEN];

    int    inotify_descriptor;
    struct sigaction sigIntHandler;

    /**
     * Setup handler for closing program
     */
    sigIntHandler.sa_handler = sig_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    /**
     * WORK IN PROGRESS
     * -----------------------------------------
     */
    /* get the process id */
    pid_t pid;
    if ((pid = getpid()) < 0) {
      perror("Unable to get pid\n");
    }
    else {
      printf("\nThe process id is %d\n", pid);
    }
    /**
     * -----------------------------------------
     */

    /**
     * No Arguments ?
     * OPTION: HELP
     */
    if (argc == 1 or 0 == strcmp(argv[1], "--help")) {
        usage();
        exit(EXIT_SUCCESS);
    }

    /**
     * Initialize Inotify
     */
    inotify_descriptor = dog_init();

    /**
     * Argument parsing for PATH TO WATCH
     * -----------------------------------------------------------
     * maybe in a later time it would be nice to implement getopt
     */
    for (int ac = 1; ac < argc; ac++)
    {
        /**
         * OPTION: move to path instead of deleting
         */
        if (0 == strncmp(argv[ac], OPT_MV_PATH, strlen(OPT_MV_PATH)))
        {
            strcpy(moveto, &argv[ac][strlen(OPT_MV_PATH)]);
            if (argv[ac][strlen(argv[ac]) - 1] != '/') {
                strcat(moveto, "/");
            }

            continue;
        }
        else {
            /**
             * ADD PATH
             */
            pin_subdirectories(inotify_descriptor, argv[ac]);
        }
    }

    /**
     * +----------------------------+
     * |                            |
     * |     -= | MAIN LOOP | =-    |
     * |     -------------------    |
     * |                            |
     * +----------------------------+
     */
    while (1)
    {

        buffer_length = read(inotify_descriptor, buffer, BUF_LEN);
        if (buffer_length < 0) {
            printf("[%s] Error while reading from inotify buffer\n", getcurrenttime());
        }

        while (buffer_counter < buffer_length)
        {
            struct inotify_event *event = (struct inotify_event *) &buffer[buffer_counter];
            if (event->len)
            {
                char *new_path, *tmp_path, *f_ext = NULL;

                if (event->mask bitand IN_CREATE)
                {
                    /**
                     * Retrive path using the watch descriptor
                     */
                    tmp_path         = get_barking_path(event->wd);
                    new_path         = pathdup_addsubdir(tmp_path, event->name);
                    f_ext            = &new_path[strlen(new_path) - 4];

                    if (event->mask bitand IN_ISDIR) {
                        printf("[%s] Directory created: %s\n", getcurrenttime(), new_path);
                        pin_subdirectories(inotify_descriptor, new_path);
                    }
                    else
                    {
                        for (unsigned int j = 0; j < sizeof(unwanted_files); j++)
                        {
                            int blablazero = 0;
                            if (blablazero == strcmp(f_ext, unwanted_files[j]))
                            {
                                printf("[%s] ALLERT ALLERT ALLERT - UNWANTED FILE : %s\n", getcurrenttime(), new_path);
                                unlink(new_path);
                            }
                        }
                    }

                    free(new_path);
                }

                if (event->mask bitand IN_DELETE and event->mask bitand IN_ISDIR) {
                    forget_wd(event->wd);
                }

                buffer_counter += EVENT_SIZE + event->len;
            }
        }
    }

    /* Clean up - Impossible to reach */
    clean_garbage();
    exit(EXIT_SUCCESS);
}

