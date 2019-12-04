#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define PCRE2_CODE_UNIT_WIDTH 32
#include <pcre2.h>

static void print_help(void) {
    printf(
        "lank - a symlink utility for all things symlink\n"
        "Usage:\n"
        "    lank SRC DEST\n"
        "    lank MATCH REPLACEMENT FILES...\n"
        "    lank <OPTIONS>\n"
        "Options:\n"
        "    -[-s]rc - the src symlink file to modify\n"
        "    -[-d]est - the new destination for the input symlink\n"
        "\n"
        "    -[-m]atch - the regex pattern to apply to the target of the symlinks\n"
        "    -[-r]eplacement - the replacement pattern for the match\n"
        "\n"
        "    -[-h]elp - this message\n"
        "Notes:\n"
        "    Uses the PCRE2 regex engine for replacements.\n"
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

static void batch_rename(const char* match, const char* replacement, const char **symlinks, const size_t n) {
    printf("match %s replacement %s\n", match, replacement);
    for (size_t i = 0; i < n; ++i) {
        printf("%s\n", symlinks[i]);
    }
}

int main(int argc, char **argv) {
    if (argc == 1) {
        print_help();
        exit(EXIT_SUCCESS);
    }

    const char *target_symlink = NULL;
    const char *target_dest = NULL;
    const char *match = NULL;
    const char *replacement= NULL;

    int c;
    while (1) {
        int option_index = 0;

        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"src", required_argument, 0, 's'},
            {"dest", required_argument, 0, 'd'},
            {"match", required_argument, 0, 'm'},
            {"replacement", required_argument, 0, 'r'},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "hs:d:m:r:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'd':
                target_dest = optarg;
                break;
            case 's':
                target_symlink = optarg;
                break;
            case 'm':
                match = optarg;
                break;
            case 'r':
                replacement = optarg;
                break;
            case 'h':
            case '?':
                print_help();
                exit(EXIT_SUCCESS);
        }
    }

    if (argc == 3 && target_dest == NULL && target_symlink) {
        target_symlink = argv[1];
        target_dest = argv[2];
    } else if (argc >= 4 && match == NULL && replacement == NULL) {
        match = argv[0];
        replacement = argv[1];
        optind += 2;
    }


    if (match == NULL && replacement == NULL && target_symlink != NULL && target_dest != NULL) {
        //single mode
        if (optind < argc) {
            printf("ignoring args: ");
            while (optind < argc) {
                printf("%s ", argv[optind++]);
            }
            printf("\n");
        }
        handle_file_type(target_symlink, target_dest);
    } else if (match != NULL && replacement != NULL) {
        //batch mode
        batch_rename(match, replacement, (const char **)argv+optind, argc-optind);
    } else {
        //you messed up
        print_help();
        exit(EXIT_SUCCESS);
    }

    return EXIT_SUCCESS;
}
