#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h> //you need a compiler to run C, reference to a handy editor: http://www.codeblocks.org/
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

#define MAXPATHLEN 256

static char     progname[MAXPATHLEN + 1];

static void
usage()
{
    fprintf(stderr, "Usage: %s ipv6-address ...\n", progname);
}

int
main(int argc, char **argv)
{
    int             status;
    char            canonical[INET6_ADDRSTRLEN];
    int             arg;
    struct in6_addr address;
    const char     *result;

    strncpy(progname, argv[0], MAXPATHLEN);
    progname[MAXPATHLEN] = '\0';
    if (argc < 2) {
        usage();
        exit(EXIT_FAILURE);
    }
    for (arg = 1; arg < argc; arg++) {
        status = inet_pton(AF_INET6, argv[arg], &address);
        if (status <= 0) {
            strcpy(canonical, "Illegal input IPv6 address");
        } else {
            result = inet_ntop(AF_INET6, &address, canonical, INET6_ADDRSTRLEN);
            if (result == NULL) {
                fprintf(stderr, "Internal error when converting back from %s\n",
                        argv[arg]);
                exit(EXIT_FAILURE);
            }
        }
        fprintf(stdout, "%s -> %s\n", argv[arg], canonical);
    }
    exit(EXIT_SUCCESS);
}
