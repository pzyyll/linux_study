#include <cstdio>
#include <cstdlib>
#include <unistd.h>

int main(int argc, char **argv) {

    int opt;

    while ((opt = getopt(argc, argv, "abc:e:") ) != -1) {
        switch (opt) {
        case 'a':
            printf("opt a.\n");
            break;
        case 'b':
            printf("opt b.\n");
            break;
        case 'c':
            printf("opt c. arg=%s\n", optarg);
            break;
        case 'e':
            printf("opt e. arg=%s\n", optarg);
            break;
        case '?':
            printf("????????");
            break;
        default:
            printf("what fuck?\n");
            break;
        }
    }

    printf("%d\n", optind);

    return 0;
}
