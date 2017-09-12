#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>

int main(int argc, char **argv) {

    int set = 0;
    struct option ops[] = {
        {"printf", required_argument, 0, 'p'},
        {"help", optional_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"other", no_argument, 0, 0},
        {"set", required_argument, &set, 12},
        {0, 0, 0, 0}
    };

    int c = 0;
    int opt_idx = 0;
    while ((c = getopt_long(argc, argv, "p:h:v", ops, &opt_idx) ) != -1) {
        switch (c) {
            case 0:
                printf("zero name %s\n", ops[opt_idx].name);
                printf("set %d\n", set);
                break;
            case 'p':
                printf("printf %s\n", optarg);
                break;
            case 'h':
                printf("help %s\n", optarg);
                break;
            case 'v':
                printf("version\n");
                break;
            case '?':
                printf("??????\n");
                break;
            defualt:
                printf("what fuck.");
                break;
        }

        printf("optind = %d\n", optind);
    }

    return 0;
}
