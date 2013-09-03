
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
#include <iso646.h>

#define MAX_EVENTS 1024                                     /* Max N° to process */
#define LEN_NAME   16                                       /* Assuming filename won't exceed 16 Byte */
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN    (MAX_EVENTS * (EVENT_SIZE + LEN_NAME))   /* Buffer to store the data of events */
#define MV_PATH    "--moveto="                              /* Option to move files to path instead of deleting them */

void usage();                                               /* HELP Function */
int bark_at(int fd, char *door);                            /* Add watch */

int main (int argc, char **argv)
{
    int length, wd, fd;
    int ac, i = 0;
    char buffer[BUF_LEN];
    char *tmp_path;

    /* Initialize Inotify */
    fd = inotify_init();
    if (fd < 0) {
        perror("Can't initialize inotify");
        close(fd);
        exit(EXIT_FAILURE);
    }

    for (ac=1;ac<argc;ac++)
    {
        if (0 == strncmp(argv[ac], MV_PATH, sizeof(MV_PATH) -1))
        {
            tmp_path = &argv[ac][sizeof(MV_PATH)-1];
            printf("Set option to move files to %s\n", tmp_path);
            ;/* DA IMPLEMENTARE */
        }
        else if (0 == strcmp(argv[ac], "--help"))
        {
            usage();
            close(fd);
            exit(EXIT_SUCCESS);
        }
        else
        {
            /* Add inotify wath to path */
            bark_at(fd, argv[ac]);
            printf("Added path: %s\n", argv[ac]);
            ;/* DA IMPLEMENTARE */
        }
    }

    /* Main Loop */
    while (1)
    {
        i = 0;
        length = read(fd, buffer, BUF_LEN);

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
    inotify_rm_watch(fd, wd);
    close(fd);

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

int bark_at(int fd, char *door)
{
    if (-1 == inotify_add_watch(fd, door, IN_CREATE | IN_MODIFY))
    {
        printf("Couldn't add watch to %s\n", door);
        return 1;
    }
    else
    {
        printf("Watching: %s\n", door);
        return 0;
    }
}
