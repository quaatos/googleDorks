#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

/* http://www.over-yonder.net/~fullermd/projects/libcidr/ */
#include <libcidr.h>

#define MAXPROGLENGTH 1024
#define MAXPREFIXLENGTH 256

char            progname[MAXPROGLENGTH];
char            filename[MAXPROGLENGTH];

void
usage()
{
    fprintf(stderr, "Usage: %s [-g] [-s] filename prefix\n", progname);
}

int
main(int argc, char **argv)
{
    extern char    *optarg;
    extern int      optind;
    char            prefix[MAXPREFIXLENGTH];
    CIDR           *prefix_binary, *pfx_binary, *left, *right;
    char            ch;
    FILE           *f;
    int             fresult;
    int             line;
    char           *pfx;
    unsigned int    specific = 1, verbose = 0;

    strcpy(progname, argv[0]);
    while ((ch = getopt(argc, argv, "hgvs")) != EOF) {
        switch (ch) {
        case 'h':
            usage();
            return (EXIT_SUCCESS);
        case 'v':
            verbose = 1;
            break;
        case 's':
            specific = 1;
            break;
        case 'g':
            specific = 0;
            break;
        default:
            usage();
            return (EXIT_FAILURE);
        }
    }
    argc -= optind;
    argv += optind;
    if (argc != 2) {
        usage();
        abort();
    }

    strcpy(filename, argv[0]);
    strcpy(prefix, argv[1]);
    prefix_binary = cidr_from_str(prefix);
    if (prefix_binary == NULL) {
        fprintf(stderr, "Invalid prefix %s\n", prefix);
        abort();
    }
    f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "File %s cannot be opened\n", filename);
        abort();
    }
    fresult = 1;
    line = 0;
    pfx = malloc(MAXPREFIXLENGTH);
    while (fresult != EOF) {
        fresult = fscanf(f, "PREFIX: %s\n", pfx);
        line++;
        if (fresult == EOF) {
            break;
        }
        if (verbose) {
            if (line % 1000 == 0) {
                fprintf(stdout, "%i lines processed... (at %s)\n", line, pfx);
            }
        }
        if (fresult < 1) {
            continue;
        }
        pfx_binary = cidr_from_str(pfx);
        if (pfx_binary == NULL) {
            fprintf(stderr, "Invalid prefix %s at line %i of file %s\n", pfx, line,
                    filename);
            abort();
        }
        if (specific == 1) {
            left = prefix_binary;
            right = pfx_binary;
        } else {
            right = prefix_binary;
            left = pfx_binary;
        }
        if (cidr_contains(left, right) == 0) {
            fprintf(stdout, "%s (line %i)\n", cidr_to_str(pfx_binary, 0), line);
        }
    }
    fclose(f);
    return (0);
}
