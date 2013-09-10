#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
/* "readdir" etc. are defined here. */
#include <dirent.h>
/* limits.h defines "PATH_MAX". */
#include <limits.h>

/* List the files in "dir_name". */

static void
list_dir (const char * dir_name)
{
    DIR * d;

    /* Open the directory specified by "dir_name". */

    d = opendir (dir_name);

    /* Check it was opened. */
    if (! d) {
        fprintf (stderr, "Cannot open directory '%s': %s\n",
                 dir_name, strerror (errno));
        exit (EXIT_FAILURE);
    }

    printf("\nDT_UNKNOWN : %d\n", DT_UNKNOWN);
    printf("DT_DIR : %d\n", DT_DIR);
    printf("DT_BLK : %d\n", DT_BLK);
    printf("DT_CHR : %d\n", DT_CHR);
    printf("DT_FIFO : %d\n", DT_FIFO);
    printf("DT_LNK : %d\n", DT_LNK);
    printf("DT_REG : %d\n", DT_REG);
    printf("DT_SOCK : %d\n", DT_SOCK);


    while (1) {
        struct dirent * entry;
        const char * d_name;

        entry = readdir (d);
        if (! entry) break;
        d_name = entry->d_name;

        printf("\nd_type : %d", entry->d_type);

        printf ("\n%s/%s", dir_name, d_name);

        if (entry->d_type == DT_UNKNOWN) {
            puts(" -> DT_UNKNOWN\n");
        }
        if (entry->d_type & DT_DIR) {
            puts(" -> DT_DIR\n");
        }
        if (entry->d_type & DT_BLK) {
            puts(" -> DT_BLK\n");
        }
        if (entry->d_type & DT_CHR) {
            puts(" -> DT_CHR\n");
        }
        if (entry->d_type & DT_FIFO) {
            puts(" -> DT_FIFO\n");
        }
        if (entry->d_type & DT_LNK) {
            puts(" -> DT_LNK\n");
        }
        if (entry->d_type & DT_REG) {
            puts(" -> DT_REG\n");
        }
        if (entry->d_type & DT_SOCK) {
            puts(" -> DT_SOCK\n");
        }

    }
    /* After going through all the entries, close the directory. */
    if (closedir (d)) {
        fprintf (stderr, "Could not close '%s': %s\n",
                 dir_name, strerror (errno));
        exit (EXIT_FAILURE);
    }
}

int main ()
{
    list_dir ("/var/www/GitHub/watchdog");
    return 0;
}
