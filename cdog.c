
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

#define MAX_EVENTS       1024                                     /* Max N° to process */
#define LEN_NAME         16                                       /* Assuming filename won't exceed 16 Byte */
#define EVENT_SIZE       (sizeof(struct inotify_event))
#define BUF_LEN          (MAX_EVENTS * (EVENT_SIZE + LEN_NAME))   /* Buffer to store the data of events */
#define OPT_MV_PATH      "--moveto="                              /* Option to move files to path instead of deleting them */
#define OPT_LOG_NAME     "--logto="                               /* Option to specify log file */
#define LOG_NAME_DEFAULT "watchdog.log"                           /* Option to specify log file */

FILE *fp_log = NULL;                                              /* File pointer for loggin purposes */
char *fp_log_name = LOG_NAME_DEFAULT;                             /* Log default filename */

int wd_poll[MAX_EVENTS];                                          /* All watchers poll */
char* wd_path[MAX_EVENTS];                                        /* All watchers paths */
int wd_index = -1;                                                /* Watch counter */

int fd_main;                                                      /* Main inotify's file descriptor */
char *moveto = NULL;                                              /* path to directory where EVENTUALLY store unwanted files */

char *unwanted_files[] = {".gif", ".php"};                        /* ... */

void usage();                                                     /* HELP Function */
int bark_at(int file_descriptor, char *door);                     /* Add watch */
void memorize_wd(int wd, char *path);                             /* Save wd and path relations */
void pin_subdirectories(int file_descriptor, char *root);
void wdog_log(char *message);
void wdog_exit_success(char *message);
void wdog_exit_failure(char *message);
void clean_garbage();
char *pathdup_addsubdir(char *path, char* subdir);

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
            wdog_exit_success("\n\nExiting process\n\n");
            break;
        default:
            printf("Caught signal %d\n",s);
            break;
    }
}

int main (int argc, char **argv)
{
    int length;
    int ac, i = 0;
    char buffer[BUF_LEN];

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sig_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    /**
     * No Arguments ?
     */
    if (argc == 1) {
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
            if (moveto != NULL) {
                free(moveto);
            }

            int string_length = strlen(argv[ac]) - strlen(OPT_MV_PATH) + 1;

            if (argv[ac][strlen(argv[ac]) - 1] != '/') {
                string_length += 1;
            }

            moveto = (char *) malloc(string_length);

            strcpy(moveto, &argv[ac][strlen(OPT_MV_PATH)]);

            if (argv[ac][strlen(argv[ac]) - 1] != '/') {
                strcat(moveto, "/");
            }

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
        if (0 == strcmp(argv[ac], "--help")) {
            usage();
            exit(EXIT_SUCCESS);
        }
    }

    /**
     * Setup LOGGER
     */
    fp_log = fopen(fp_log_name, "a");
    if (fp_log == NULL) {
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
     * +----------------------------+
     * |                            |
     * |     -= | MAIN LOOP | =-    |
     * |     -------------------    |
     * |                            |
     * +----------------------------+
     */
    while (1)
    {
        i = 0;
        length = read(fd_main, buffer, BUF_LEN);
        if (length < 0) {
            wdog_log("error while reading from inotify buffer\n");
        }

        while (i < length)
        {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            if (event->len)
            {

                if (event->mask bitand IN_CREATE)
                {
                    /**
                     * Retrive path using the watch descriptor
                     */
                    int j, descriptor_index = -1;
                    char *new_path, *f_ext = NULL;

                    for (j = 0; j <= wd_index; j++) {
                        if (wd_poll[j] == event->wd) {
                            descriptor_index = j;
                            break;
                        }
                    }
                    new_path = pathdup_addsubdir(wd_path[descriptor_index], event->name);
                    f_ext = &new_path[strlen(new_path) - 4];

                    if (event->mask bitand IN_ISDIR) {
                        wdog_log("Directory created : ");
                        wdog_log(new_path);
                        wdog_log("\n");

                        pin_subdirectories(fd_main, new_path);
                    }
                    else
                    {
                        for (j = 0; j < sizeof(unwanted_files); j++)
                        {
                            if (0 == strcmp(f_ext, unwanted_files[j]))
                            {
                                wdog_log("----------------------------------------------------------------------------------------------------------\n");
                                char *message = "ALLERT UNWANTED FILE : %s\n";
                                char *error = (char *) malloc(strlen(message) + strlen(new_path) + 1);
                                sprintf(error, message, new_path);
                                wdog_log(error);
                                free(error);
                                wdog_log("----------------------------------------------------------------------------------------------------------\n");

                                unlink(new_path);
                            }
                        }
                    }

                    free(new_path);
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
    if (wd_index == (MAX_EVENTS-1)) {
        wdog_log("MAX_EVENTS reached, can't add more watch descriptors\n");
        return -1;
    }

    int watch_descriptor;
    watch_descriptor = inotify_add_watch(file_descriptor, door, IN_CREATE);
    if (watch_descriptor == -1)
    {
        char *message = "Couldn't add watch to %s\n";
        char *error = (char *) malloc(strlen(message) + strlen(door) + 1);
        sprintf(error, message, door);
        wdog_log(error);
        free(error);
    }
    else
    {
        time_t curtime;
        time (&curtime);
        char *msg = ctime(&curtime);
        msg[strlen(msg)-1] = ' ';
        wdog_log(msg);
        wdog_log(door);
        wdog_log(" : Barking!\n");
        memorize_wd(watch_descriptor, door);
    }

    return watch_descriptor;
}

void memorize_wd(int wd, char *path)
{
    wd_index += 1;

    if (wd_index < MAX_EVENTS) {
        wd_poll[wd_index] = wd;
        wd_path[wd_index] = strdup(path);
    }
    else {
        wdog_log("MAX_EVENTS reached, can't add more watch descriptors\n");
    }
}


/**
 * Traverse all subdirectories and add a inotify watch to every sub directory
 * return the list of all watch descriptors
 */
void pin_subdirectories(int file_descriptor, char *root)
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
        wdog_exit_failure(error);
    }

    watch_descriptor = bark_at(file_descriptor, root);
    if (watch_descriptor == -1) {
        wdog_exit_failure("unable to add watch");
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

void wdog_log(char *message)
{
    if (0 != errno)
    {
        if (fp_log != NULL) {
            fprintf(fp_log, "[%s] %s\n", strerror(errno), message);
        }
        else {
            printf("[%s] %s\n", strerror(errno), message);
        }
    }
    else
    {
        if (fp_log != NULL) {
            fprintf(fp_log, "%s", message);
        }
        else {
            printf("%s", message);
        }
    }

    // DEBUG PRINTF
    printf("%s", message);
}

void wdog_exit_success(char *message)
{
    wdog_log(message);
    clean_garbage();
    exit(EXIT_SUCCESS);
}

void wdog_exit_failure(char *message)
{
    wdog_log(message);
    clean_garbage();
    exit(EXIT_FAILURE);
}

void clean_garbage()
{
    int i = 0;
    if (wd_path != NULL)
    {
        for (i = 0; i <= wd_index; i++) {
            free(wd_path[i]);
        }
    }

    if (moveto != NULL) {
        free(moveto);
    }

    if (0 != strcmp(fp_log_name, LOG_NAME_DEFAULT)) {
        free(fp_log_name);
    }

    if (fp_log != NULL) {
        fclose(fp_log);
    }

    close(fd_main);
}

char *pathdup_addsubdir(char *path, char* subdir)
{
    int addslash = (path[strlen(path)-1] != '/');
    char *newpath = NULL;
    int k = addslash ? 2 : 1;

    newpath = malloc(strlen(path) + strlen(subdir) + k);

    strcpy(newpath, path);
    if (addslash) {
        strcat(newpath, "/");
    }

    strcat(newpath, subdir);

    return newpath;
}
