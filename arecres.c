/*
 * arecres.c
 * Respond to all DNS queries with a A record.
 * By J. Stuart McMurray
 * Created 20230505
 * Last Modified 20230507
 */

#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arecres.h"
#include "log.h"
#include "query.h"

void listen_address(struct sockaddr_in *, const char *, const char *);

void
usage(void)
{
        fprintf(stderr, "Usage: %s [-h] [-l laddr] [-p lport] [-t ttl] addr\n"
"\n"
"Responds with a single A record for addr (which may be a hostname) to all \n"
"DNS queries.\n"
"\n"
"Options:\n"
"  -l laddr - Listen IPv4 address (default: %s)\n"
"  -p lport - Listen port (default: %s)\n"
"  -t ttl   - Response TTL (default: %s)\n",
                        getprogname(), DEFADDR, DEFPORT, DEFTTL);
        exit(1);
}

int
main(int argc, char **argv)
{
        int ch, s;
        char *laddr, *lport, *ttl;
        struct sockaddr_in sa;
        socklen_t namelen;

        if (-1 == pledge("dns inet stdio", ""))
                err(17, "pledge (init)");

        /* Flag-parsing */
        laddr = DEFADDR;
        lport = DEFPORT;
        ttl   = DEFTTL;
        while ((ch = getopt(argc, argv, "l:p:t:h")) != -1) {
                switch (ch) {
                        case 'l':
                                laddr = optarg;
                                break;
                        case 'p':
                                lport = optarg;
                                break;
                        case 't':
                                ttl = optarg;
                                break;
                        case 'h':
                        default:
                                usage();
                }
        }
        argc -= optind;
        argv += optind;

        /* Work out the A record to serve. */
        if (1 != argc)
                errx(2, "need an IP address for replies");
        makearec(argv[0], ttl);

        /* Work out the listen address. */
        listen_address(&sa, laddr, lport);

        /* Open a socket to listen for queries. */
        if (-1 == (s = (socket(AF_INET, SOCK_DGRAM, 0))))
                err(11, "socket");
        if (-1 == bind(s, (struct sockaddr *)&sa, sizeof(sa)))
                err(12, "bind");
        memset(&sa, 0, sizeof(sa));
        namelen = sizeof(sa);
        if (-1 == getsockname(s, (struct sockaddr *)&sa, &namelen))
                err(15, "getsockname");
        lprintf(NULL, "Listening on " ADDRFMT, ADDRSTR(&sa));

        if (-1 == pledge("inet stdio", ""))
                err(18, "pledge");

        /* Handle queries. */
        for (;;)
                handle_query(s);

        /* Unpossible. */
        return -1;
}

/* listen_address populates sa from laddr and lport.  It terminates the
 * program on error. */
void
listen_address(struct sockaddr_in *sa, const char *laddr, const char *lport)
{
        in_port_t pnum;
        const char *errstr;
        int ret;

        memset(sa, 0, sizeof(*sa));
        sa->sin_len = sizeof(sa);
        sa->sin_family = AF_INET;
        pnum = strtonum(lport, 0, 0xFFFF, &errstr);
        if (NULL != errstr)
                err(13, "parsing listen port (%s): %s", lport, errstr);
        sa->sin_port = htons(pnum);
        switch (ret = inet_pton(AF_INET, laddr, &sa->sin_addr.s_addr)) {
                case 1: /* Worky */
                        break;
                case 0: /* Unparseable */
                        errx(8, "unparseable listen address %s", laddr);
                case -1: /* Error */
                        err(9, "inet_pton(%s)", laddr);
                default: /* Unpossible */
                        errx(10, "inet_pton(%s): unknown error", laddr);

        }
}
