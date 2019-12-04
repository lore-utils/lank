#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

static void print_help(void) {
    printf("lank - a symlink utility for all things symlink\n"
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
           "    Uses the PCRE2 regex engine for replacements.\n");
}

/*
 * Currently this doesn't check if the new target is valid, should it?
 * Maybe add a validation flag to args and make it optional?
 */
static void rename_symlink(const char *sym_path, const char *new_link) {
    const size_t len           = strlen(sym_path);
    const char *temp_extension = ".tmp";
    char *temp_path            = malloc(len + strlen(temp_extension) + 1);
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

/**
 *  Fills newlink with a null terminated string for renaming
 */
static void re_path(
    const char *sym_path, const char *match, const char *replacement, char **new_link) {
    struct stat res           = {0};
    char *temp_dest           = NULL;
    pcre2_code *match_pattern = NULL;
    size_t new_dest_len       = 0;
    int error_code            = 0;
    size_t error_offset       = 0;

    char err_msg[120];

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

    match_pattern
        = pcre2_compile((PCRE2_SPTR)match, strlen(match), 0, &error_code, &error_offset, NULL);
    if (match_pattern == NULL) {
        pcre2_get_error_message(error_code, (PCRE2_UCHAR *)err_msg, sizeof(err_msg));
        printf("%s\n", err_msg);
        goto error;
    }

    pcre2_substitute(match_pattern, (PCRE2_SPTR)temp_dest, strlen(temp_dest), 0,
        PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_EXTENDED | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH,
        NULL, NULL, (PCRE2_SPTR)replacement, strlen(replacement), NULL, &new_dest_len);

    *new_link = malloc(new_dest_len + 1);
    if (*new_link == NULL) {
        goto error;
    }

    if (pcre2_substitute(match_pattern, (PCRE2_SPTR)temp_dest, strlen(temp_dest), 0,
            PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_EXTENDED | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH,
            NULL, NULL, (PCRE2_SPTR)replacement, strlen(replacement), (PCRE2_UCHAR *)*new_link,
            &new_dest_len)
        < 0) {
        goto error;
    }

    (*new_link)[new_dest_len] = '\0';

error:
    if (temp_dest) {
        free(temp_dest);
    }
    if (match_pattern) {
        pcre2_code_free(match_pattern);
    }
    return;
}

static void single_rename(const char *pathname, const char *replacement) {
    struct stat res;
    int ret;
    if ((ret = lstat(pathname, &res)) == -1) {
        if (errno == ENOENT) {
            //path doesnt exist
            if (symlink(replacement, pathname) == -1) {
                perror("symlink");
            }
            //no need to replace it, it didnt exist in the first place
            return;
        } else {
            perror("lstat");
            return;
        }
    }

    if (S_ISLNK(res.st_mode)) {
        rename_symlink(pathname, replacement);
    } else {
        fprintf(stderr, "%s is not a symlink\n", pathname);
    }
}

static void batch_rename(
    const char *match, const char *replacement, const char **symlinks, const size_t n) {
    for (size_t i = 0; i < n; ++i) {
        char *path = NULL;
        re_path(symlinks[i], match, replacement, &path);
        if (path) {
            single_rename(symlinks[i], path);
            free(path);
        }
    }
}

int main(int argc, char **argv) {
    if (argc == 1) {
        print_help();
        exit(EXIT_SUCCESS);
    }

    const char *target_symlink = NULL;
    const char *target_dest    = NULL;
    const char *match          = NULL;
    const char *replacement    = NULL;

    int c;
    while (1) {
        int option_index = 0;

        static struct option long_options[]
            = {{"help", no_argument, 0, 'h'}, {"src", required_argument, 0, 's'},
                {"dest", required_argument, 0, 'd'}, {"match", required_argument, 0, 'm'},
                {"replacement", required_argument, 0, 'r'}, {0, 0, 0, 0}};

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

    if (argc == 3 && target_dest == NULL && target_symlink == NULL) {
        target_symlink = argv[1];
        target_dest    = argv[2];
        optind += 2;
    } else if (argc >= 4 && match == NULL && replacement == NULL) {
        match       = argv[1];
        replacement = argv[2];
        optind += 2;
    }

    if (target_symlink != NULL && target_dest != NULL && match == NULL && replacement == NULL) {
        //single mode
        if (optind < argc) {
            printf("ignoring args: ");
            while (optind < argc) {
                printf("%s ", argv[optind++]);
            }
            printf("\n");
        }
        single_rename(target_symlink, target_dest);
    } else if (match != NULL && replacement != NULL) {
        //batch mode
        batch_rename(match, replacement, (const char **)argv + optind, argc - optind);
    } else {
        printf("what %s %s %s %s\n", target_symlink, target_dest, match, replacement);
        //you messed up
        print_help();
        exit(EXIT_SUCCESS);
    }

    return EXIT_SUCCESS;
}
