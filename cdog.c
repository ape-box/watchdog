
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
#define MAX_EVENTS     1024                                     /* Max N° to process */
#define LEN_NAME       16                                       /* Assuming filename won't exceed 16 Byte */
#define EVENT_SIZE     (sizeof(struct inotify_event))
#define BUF_LEN        (MAX_EVENTS * (EVENT_SIZE + LEN_NAME))   /* Buffer to store the data of events */
#define OPT_MV_PATH    "--moveto="                              /* Option to move files to path instead of deleting them */
#define OPT_LOG_NAME   "--logto="                               /* Option to specify log file */

FILE *fp_log = NULL;                                            /* File pointer for loggin purposes */
char *fp_log_name = "watchdog.log";                             /* Log default filename */

int *wd_poll = NULL;                                           /* All watchers poll */
int fd_main;                                                    /* Main inotify's file descriptor */
char *moveto = NULL;                                            /* path to directory where EVENTUALLY store unwanted files */

void usage();                                                   /* HELP Function */
int bark_at(int file_descriptor, char *door);                   /* Add watch */
int pin_subdirectories(int file_descriptor, char *root);
void wdog_log(char *message);
void wdog_exit_success(char *message);
void wdog_exit_failure(char *message);
void clean_garbage();

int main (int argc, char **argv)
{
    int length;
    int ac, i = 0;
    char buffer[BUF_LEN], root[MAX_PATH_LEN];

    /**
     * No Arguments ?
     */
    if (argc == 1)
    {
        usage();
        exit(EXIT_SUCCESS);
    }

    /**
     * Argument parsing for OPTIONS
     * -----------------------------------------------------------
     * maybe in a later time it would be nice to implement getopt
     */
    for (ac = 1; ac < argc; ac++)
    {
        /**
         * OPTION: move to path instead of deleting
         */
        if (0 == strncmp(argv[ac], OPT_MV_PATH, strlen(OPT_MV_PATH)))
        {
            /**
             * if double option: free the first one
             */
            if (moveto != NULL) free(moveto);

            int sl = strlen(argv[ac]) - strlen(OPT_MV_PATH) + 1;
            if (argv[ac][strlen(argv[ac]) - 1] != '/') sl += 1;
            moveto = (char *) malloc(sl);
            strcpy(moveto, &argv[ac][strlen(OPT_MV_PATH)]);
            if (argv[ac][strlen(argv[ac]) - 1] != '/') strcat(moveto, "/");

            continue;
        }

        /**
         * OPTION: change log default file
         */
        if (0 == strncmp(argv[ac], OPT_LOG_NAME, strlen(OPT_LOG_NAME)))
        {
            char *tmp_string;

            tmp_string = strdup(&argv[ac][strlen(OPT_LOG_NAME)]);
            fp_log = fopen(tmp_string, "a");
            if (fp_log != NULL)
            {
                free(fp_log_name);
                fp_log_name = tmp_string;
            }
            fclose(fp_log);
            continue;
        }

        /**
         * OPTION: HELP
         */
        if (0 == strcmp(argv[ac], "--help"))
        {
            usage();
            exit(EXIT_SUCCESS);
        }
    }

    /**
     * Setup LOGGER
     */
    fp_log = fopen(fp_log_name, "a");
    if (fp_log == NULL)
    {
        printf("Error opening log file \"%s\".\nAll output will be redirected to the stdout\n", fp_log_name);
        fp_log = stdout;
    }

    /**
     * Initialize Inotify
     */
    fd_main = inotify_init();
    if (fd_main < 0) wdog_exit_failure("inotify init error");

    /**
     * Argument parsing for PATH TO WATCH
     * -----------------------------------------------------------
     * maybe in a later time it would be nice to implement getopt
     */
    for (ac=1;ac<argc;ac++)
    {
        /**
         * SKIP OPTIONS
         */
        if (0 == strncmp(argv[ac], OPT_MV_PATH, sizeof(OPT_MV_PATH) -1))   continue;
        if (0 == strncmp(argv[ac], OPT_LOG_NAME, sizeof(OPT_LOG_NAME) -1)) continue;
        if (0 == strcmp(argv[ac], "--help"))                               continue;

        /**
         * ADD PATH
         */
        pin_subdirectories(fd_main, argv[ac]);
    }

    /**
     * Main Loop
     * ==========================================================================================================
     */
    while (1)
    {
        i = 0;
        length = read(fd_main, buffer, BUF_LEN);
        if (length < 0) wdog_log("error while reading from inotify buffer");

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

                i += EVENT_SIZE + event->len;
            }
        }
    }

    /* Clean up */
    wdog_exit_success("Exit requested");
}

void usage ()
{
    printf("\
OPTIONS:\
 %s[path] # Specify a path where MOVE INSTEAD OF DELETE unwanted files\n\
 [path]          # Path to watch for unwanted files\n\
\n", OPT_MV_PATH);
}

/**
 * Add a inotify watch to path
 * return the watch descriptor
 */
int bark_at(int file_descriptor, char *door)
{
    int watch_descriptor;
    watch_descriptor = inotify_add_watch(file_descriptor, door, IN_CREATE);
    if (watch_descriptor == -1)
    {
        char *message = "Couldn't add watch to %s\n";
        char *error = (char *) malloc(strlen(message) + strlen(door) + 1);
        sprintf(error, message, door);
        wdog_log(error);
        free(message);
        free(error);
    }
    else
    {
        if (wd_poll == NULL)
        {
            wd_poll = malloc(2 * sizeof(int));
            wd_poll[0] = 1;
        }
        else
        {
            wd_poll[0] += 1;
            wd_poll = (int *) realloc(wd_poll, sizeof(wd_poll) + sizeof(int));
        }
        wd_poll[wd_poll[0]] = watch_descriptor;
    }

    return watch_descriptor;
}


/**
 * Traverse all subdirectories and add a inotify watch to every sub directory
 * return the list of all watch descriptors
 */
int pin_subdirectories(int file_descriptor, char *root)
{
    int watch_descriptor;
    char *abs_dir;
    struct dirent *entry;
    DIR *dp;

    dp = opendir(root);
    if (dp == NULL)
    {
        char *message = "could not open dir %s\n";
        char *error = (char *) malloc(strlen(message) + strlen(root) + 1);
        sprintf(error, message, root);
        free(message);
        wdog_exit_failure(error);
    }

    watch_descriptor = bark_at(file_descriptor, root);
    if (watch_descriptor == -1) wdog_exit_failure("unable to add watch");

    /* Add watches to the level 1 subdirs */
    abs_dir = (char *)malloc(MAX_PATH_LEN);
    while ((entry = readdir(dp)))
    {
        if (entry->d_type == DT_DIR)
        {
            strcpy(abs_dir, root);
            strcat(abs_dir, entry->d_name);

            pin_subdirectories(file_descriptor, abs_dir);
        }
    }

    closedir(dp);
    free(abs_dir);
}

void wdog_log(char *message)
{
    if (fp_log != NULL) fprintf(fp_log, "%s: [%s]\n", message, strerror(errno));
    else printf("%s: [%s]\n", message, strerror(errno));
}

void wdog_exit_success(char *message)
{
    wdog_log(message);

    free(message);
    clean_garbage();

    puts("Byez!");
    exit(EXIT_SUCCESS);
}

void wdog_exit_failure(char *message)
{
    wdog_log(message);

    free(message);
    clean_garbage();

    exit(EXIT_FAILURE);
}

void clean_garbage()
{
    int i = 0;
    if (wd_poll != NULL)
    {
        for (i = 0; i < wd_poll[0]; ++i)
        {
            inotify_rm_watch(fd_main, wd_poll[i]);
        }
    }

    if (moveto != NULL) free(moveto);
    free(fp_log_name);

    if (fp_log != NULL) fclose(fp_log);
    close(fd_main);
}
