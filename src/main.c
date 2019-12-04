#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

static void print_help(void) {
    printf(
        "lank - a symlink utility for all things symlink\n"
        "Usage: lank SRC DEST\n"
        "       lank <OPTIONS>\n"
        "Options:\n"
        "-[-h]elp - this message\n"
        "-[-s]rc - the src symlink file to modify\n"
        "-[-d]est - the new destination for the input symlink\n"
        );
}

/*
 * Currently this doesn't check if the new target is valid, should it?
 * Maybe add a validation flag to args and make it optional?
 */
static void rename_symlink(const char *sym_path, const char *new_link) {
    const size_t len = strlen(sym_path);
    const char *temp_extension = ".tmp";
    char *temp_path = malloc(len + strlen(temp_extension) + 1);
    if (temp_path == NULL) {
        return;
    }

    memcpy(temp_path, sym_path, len);
    temp_path[len] = '\0';
    strcat(temp_path, temp_extension);

    if (symlink(new_link, temp_path) == -1) {
        perror("symlink");
        goto error;
    }

    if (rename(temp_path, sym_path) == -1) {
        perror("rename");
        goto error;
    }

error:
    free(temp_path);
    return;
}

static void handle_file_type(const char *pathname, const char *new_link) {
    struct stat res;
    if (lstat(pathname, &res) == -1) {
        perror("lstat");
        return;
    }

    //Stat doesn't distinguish between hard link and actual file
    if (S_ISLNK(res.st_mode)) {
        //Symlink
        rename_symlink(pathname, new_link);
    }
    //Are we even going to try and rename regular files and/or special files like block devices?
}

int main(int argc, char **argv) {
    if (argc == 1) {
        print_help();
        exit(EXIT_SUCCESS);
    }

    const char *target_symlink = NULL;
    const char *target_dest = NULL;

    int c;
    while (1) {
        int option_index = 0;

        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"src", required_argument, 0, 's'},
            {"dest", required_argument, 0, 'd'},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "hs:d:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'd':
                target_dest = optarg;
                break;
            case 's':
                target_symlink = optarg;
                break;
            case 'h':
            case '?':
                print_help();
                exit(EXIT_SUCCESS);
        }
    }

    if (target_dest == NULL && target_symlink == NULL && argc == 3) {
        target_symlink = argv[1];
        target_dest = argv[2];
    } else {
        //while inform people its not being used
        if (optind < argc) {
            printf("ignoring args: ");
            while (optind < argc) printf("%s ", argv[optind++]);
            printf("\n");
        }

        if (target_symlink == NULL || target_dest == NULL) {
            print_help();
            exit(EXIT_SUCCESS);
        }
    }

    handle_file_type(target_symlink, target_dest);

    return EXIT_SUCCESS;
}
