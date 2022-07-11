#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

#define MAXHOSTNAMELEN 256
#define MAXPORTLEN 6
#define MAXPATHLEN 256
#define DEFAULT_PORT 80
#define INPUTBUFFERLEN 256
#define OUTPUTBUFFERLEN 1024

static void
usage(const char *progname)
{
    fprintf(stderr, "Usage: %s hostname [path]\n", progname);
}

static char    *
text_of(const struct sockaddr *address)
{
    char           *text = malloc(INET6_ADDRSTRLEN);
    const char     *result;
    struct sockaddr_in6 *address_v6;
    struct sockaddr_in *address_v4;
    if (address->sa_family == AF_INET6) {
        address_v6 = (struct sockaddr_in6 *) address;
        result = inet_ntop(AF_INET6, &address_v6->sin6_addr, text, INET6_ADDRSTRLEN);
    } else if (address->sa_family == AF_INET) {
        address_v4 = (struct sockaddr_in *) address;
        result = inet_ntop(AF_INET, &address_v4->sin_addr, text, INET_ADDRSTRLEN);
    } else {
        return ("[Unknown family address]");
    }
    if (result == NULL) {
        return ("[Internal error in address formating]");
    }
    return text;
}

int
main(int argc, char **argv)
{
    static int      verbose = 0;
    static int      fast_open = 0;
    static struct option long_options[] = {
        {"verbose", no_argument, &verbose, 1},
        {"tcp_fast_open", no_argument, &fast_open, 1},
        {"port", required_argument, NULL, 0},
        {0, 0, 0, 0}
    };
    char            hostname[MAXHOSTNAMELEN + 1];
    struct addrinfo hints_numeric, hints;
    struct addrinfo *addrs, *hostref;
    int             status;
    int             c;
    int             port = DEFAULT_PORT;
    char           *path = malloc(MAXPATHLEN);
    char           *port_text = malloc(MAXPORTLEN);
    char           *readbuf = malloc(INPUTBUFFERLEN);
    int             sd;
    ssize_t         n;
    bool            success = false;
    FILE           *wsock;
    char           *data = malloc(OUTPUTBUFFERLEN);
    unsigned int    data_len;

    while (true) {
        /* getopt_long stores the option index here.  */
        int             option_index = 0;

        c = getopt_long(argc, argv, "vfp:", long_options, &option_index);

        /* Detect the end of the options.  */
        if (c == -1)
            break;

        switch (c) {
        case 0:
            /* If this option set a flag, do nothing else now.  */
            if (long_options[option_index].flag != 0)
                break;
            if (strcmp(long_options[option_index].name, "port") == 0) {
                port = strtol(optarg, NULL, 10);        /* Assume the port number is 
                                                         * in base 10 */
                if (port == 0) {
                    fprintf(stderr, "Invalid port number %s\n", optarg);
                    abort();
                }
            } else {
                fprintf(stderr, "Internal error, option %s not known\n",
                        long_options[option_index].name);
            }
            break;
        case 'v':
            verbose = 1;
            break;
        case 'f':
            fast_open = 1;
            break;
        case 'p':
            port = strtol(optarg, NULL, 10);    /* Assume the port number is in base 
                                                 * 10 */
            if (port == 0) {
                fprintf(stderr, "Invalid port number %s\n", optarg);
                abort();
            }
            break;
        case '?':
        case 'h':
            usage(argv[0]);
            exit(EXIT_SUCCESS);
        default:
            abort();
        }
    }

    /* Print any remaining command line arguments (not options).  */
    if (argc - optind != 1 && argc - optind != 2) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    strncpy(hostname, argv[optind], MAXHOSTNAMELEN);
    hostname[MAXHOSTNAMELEN] = '\0';
    sprintf(port_text, "%i", port);
    if ((argc - optind) == 2) {
        strncpy(path, argv[optind + 1], MAXPATHLEN);
    } else {
        strncpy(path, "/", 1);
    }
    /* RFC 1123 says we must try IP addresses first */
    memset(&hints_numeric, 0, sizeof(hints_numeric));
    hints_numeric.ai_flags = AI_NUMERICHOST;
    hints_numeric.ai_socktype = SOCK_STREAM;
    addrs = malloc(sizeof(struct addrinfo));
    memset(&addrs, 0, sizeof(addrs));
    status = getaddrinfo(hostname, port_text, &hints_numeric, &addrs);
    if (!status) {
        if (verbose) {
            fprintf(stdout, "%s is an IP address\n", hostname);
        }
    } else {
        if (status == EAI_NONAME) {
            /* Not an IP address */
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            addrs = malloc(sizeof(struct addrinfo));
            status = getaddrinfo(hostname, port_text, &hints, &addrs);
            if (status) {
                fprintf(stderr, "Nothing found about host name \"%s\"\n", hostname);
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Internal error, cannot resolve \"%s\" (error %i)\n",
                    hostname, status);
            exit(EXIT_FAILURE);
        }
        if (verbose) {
            fprintf(stdout, "Address(es) of %s is(are):", hostname);
            fprintf(stdout, " %s ", text_of(addrs->ai_addr));
            for (hostref = addrs->ai_next; hostref != NULL;
                 hostref = hostref->ai_next) {
                fprintf(stdout, "%s ", text_of(hostref->ai_addr));
            }
            fprintf(stdout, "\n");
        }
    }
    hostref = addrs;
    while (!success && hostref != NULL) {
        status =
            socket(hostref->ai_family, hostref->ai_socktype, hostref->ai_protocol);
        if (status < 0) {
            fprintf(stderr, "socket: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        sd = status;
        if (hostref->ai_family != AF_INET && hostref->ai_family != AF_INET6) {
            fprintf(stderr, "Unsupported family %i\n", hostref->ai_family);
            exit(EXIT_FAILURE);
        }

        if (!fast_open) {
            status =
                connect(sd, (const struct sockaddr *) hostref->ai_addr,
                        hostref->ai_addrlen);
            if (status != 0) {
                fprintf(stderr, "connect: %s\n", strerror(errno));
                hostref = hostref->ai_next;
            } else {
                if (verbose) {
                    fprintf(stdout, "Connection successful to %s\n",
                            text_of(hostref->ai_addr));
                }
                success = true;
                wsock = fdopen(sd, "a");
                if (wsock == NULL) {
                    fprintf(stderr, "fdopen: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                fprintf(wsock, "GET %s HTTP/1.1\r\n", path);
                fprintf(wsock,
                        "Host: %s\r\nUser-Agent: not your business\r\nConnection: close\r\n\r\n",
                        hostname);
                fflush(wsock);
            }
        } else {                /* TCP Fast Open */
            sprintf(data,
                    "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: not your business\r\nConnection: close\r\n\r\n",
                    path, hostname);
            data_len = strlen(data);
            /* Do not forget to be sure that sysctl's
             * net.ipv4.tcp_fastopen = 1.  Same thing for IPv6. The
             * value of this sysctl variable is a bitmap. 1 = client,
             * 2 = server, 3 = both. */
            status = sendto(sd, (const void *) data, data_len, MSG_FASTOPEN,
                            (const struct sockaddr *) hostref->ai_addr,
                            hostref->ai_addrlen);
            if (status < 0) {
                fprintf(stderr, "sendto: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (verbose) {
                /* sendto succeeeds even if TFO is not supported on the other side
                 * (it falls back to the 3WHS). Apparently no way to know if it was
                 * really with TFO
                 * http://stackoverflow.com/questions/26244536/how-to-know-if-sendto-with-tcp-fast-open-actually-used-fast-open 
                 */
                fprintf(stdout, "Connection successful (TFO requested) to %s\n",
                        text_of(hostref->ai_addr));
            }
            success = true;
        }
    }
    if (!success) {
        fprintf(stderr, "No IP address succeeded\n");
        exit(EXIT_FAILURE);
    }
    n = read(sd, readbuf, INPUTBUFFERLEN);
    if (verbose) {
        fprintf(stdout, "%li bytes read\n", (long) n);
    }
    exit(EXIT_SUCCESS);
}
