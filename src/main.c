#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define PCRE2_CODE_UNIT_WIDTH 8
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
static void rename_symlink(const char *sym_path, const char *match, const char *replacement) {
    const size_t len = strlen(sym_path);
    const char *temp_extension = ".tmp";
    struct stat res = {0};
    char *temp_path = malloc(len + strlen(temp_extension) + 1);
    char *temp_dest = NULL;
    char *new_link = NULL;
    pcre2_code *match_pattern = NULL;
    size_t new_dest_len = 0;
    int error_code = 0;
    size_t error_offset = 0;

    char err_msg[120];

    if (temp_path == NULL) {
        goto error;
    }

    if (lstat(sym_path, &res) == -1) {
        perror("lstat");
        goto error;
    }

    temp_dest = malloc(res.st_size + 1);
    if (temp_dest == NULL) {
        goto error;
    }

    if (readlink(sym_path, temp_dest, res.st_size + 1) == -1) {
        perror("readlink");
        goto error;
    }
    temp_dest[res.st_size] = '\0';

    match_pattern = pcre2_compile((const unsigned char *) match, strlen(match),
            0, &error_code, &error_offset, NULL);
    if (match_pattern == NULL) {
        pcre2_get_error_message(error_code, (unsigned char *) err_msg, sizeof(err_msg));
        printf("%s\n", err_msg);
        goto error;
    }

    pcre2_substitute(match_pattern, (const unsigned char *) temp_dest, strlen(temp_dest), 0,
            PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_EXTENDED | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH,
            NULL, NULL, (const unsigned char *) replacement, strlen(replacement), NULL, &new_dest_len);

    new_link = malloc(new_dest_len + 1);
    if (new_link == NULL) {
        goto error;
    }

    if (pcre2_substitute(match_pattern, (const unsigned char *) temp_dest, strlen(temp_dest), 0,
                PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_EXTENDED | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH,
                NULL, NULL, (const unsigned char *) replacement, strlen(replacement), (unsigned char *) new_link, &new_dest_len) < 0) {
        goto error;
    }

    new_link[new_dest_len] = '\0';

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
    if (temp_path) {
        free(temp_path);
    }
    if (temp_dest) {
        free(temp_dest);
    }
    if (match_pattern) {
        pcre2_code_free(match_pattern);
    }
    if (new_link) {
        free(new_link);
    }
    return;
}

static void handle_file_type(const char *pathname, const char *match, const char *replacement) {
    struct stat res;
    if (lstat(pathname, &res) == -1) {
        perror("lstat");
        return;
    }

    //Stat doesn't distinguish between hard link and actual file
    if (S_ISLNK(res.st_mode)) {
        //Symlink
        rename_symlink(pathname, match, replacement);
    }
    //Are we even going to try and rename regular files and/or special files like block devices?
}

static void batch_rename(const char* match, const char* replacement, const char **symlinks, const size_t n) {
    printf("match %s replacement %s\n", match, replacement);
    for (size_t i = 0; i < n; ++i) {
        printf("%s\n", symlinks[i]);
    }

    for (size_t i = 0; i < n; ++i) {
        handle_file_type(symlinks[i], match, replacement);
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
        match = argv[1];
        replacement = argv[2];
        optind += 2;
    }


#if 0
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
#else
    if (match != NULL && replacement != NULL) {
        //batch mode
        batch_rename(match, replacement, (const char **)argv+optind, argc-optind);
    } else {
        //you messed up
        print_help();
        exit(EXIT_SUCCESS);
    }
#endif

    return EXIT_SUCCESS;
}
