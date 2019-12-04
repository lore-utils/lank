#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static void print_help(void) {
    puts(
        "lank - a symlink utility for all things symlink\n"
        "Usage: lank LINK_NAME TARGET\n"
        "-[-h]elp - this message");
}

int main(int argc, char **argv) {
    if (argc == 1) {
        print_help();
        exit(EXIT_SUCCESS);
    }

    int c;
    while (1) {
        int option_index = 0;

        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "h", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
            case '?':
                print_help();
                exit(EXIT_SUCCESS);
        }
    }

    //while inform people its not being used
    if (optind < argc) {
        printf("ignoring args: ");
        while (optind < argc) printf("%s ", argv[optind++]);
        printf("\n");
    }

    return EXIT_SUCCESS;
}
