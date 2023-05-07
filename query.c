/*
 * query.c
 * Handle a query
 * By J. Stuart McMurray
 * Created 20230507
 * Last Modified 20230507
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/nameser.h>
#include <netdb.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "query.h"

/* Sizes and offsets. */
#define BUFLEN  (1024) /* Size of message buffer. */
#define NAMEOFF (12)   /* Offset to name in first question. */
#define RECLEN  (16)   /* Length of our A resource record */

unsigned char arec[RECLEN];         /* A Record to return. */
unsigned char buf[BUFLEN];          /* Message buffer. */
unsigned char name[BUFLEN];         /* Queried name. */

/* Between evaluates to true if c is between low and high, inclusive. */
#define BETWEEN(low, high, c) ((low) <= (c) && (c) <= (high))

/* isldh returns nonzero if c isn't a letter, digit, or hyphen. */
static int
isldh(unsigned char c)
{
        return  BETWEEN('a', 'z', c) ||
                BETWEEN('A', 'Z', c) ||
                BETWEEN('0', '9', c) ||
                (c == '-');
}

/* makearec makes an A record with a name pointing at the first question and
 * the IP address in addr.  It terminates the program on error. */
void
makearec(const char *addr, const char *attl)
{
        struct addrinfo hints, *res;
        int ret;
        unsigned char *p;
        const char *errstr;
        uint32_t ttl;
        struct sockaddr_in *sa;

        /* Work out the TTL. */
        ttl = (uint16_t)strtonum(attl, 0, UINT32_MAX, &errstr);
        if (NULL != errstr)
                errx(5, "parsing TTL (%s): %s", attl, errstr);

        /* Work out the IP address. */
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        if (0 != (ret = getaddrinfo(addr, NULL, &hints, &res)))
                errx(3, "getaddrinfo(%s): %s", addr, gai_strerror(ret));
        if (NULL == res) /* Unpossible. */
                errx(6, "no addresses for %s", addr);
        if (sizeof(struct sockaddr_in) != res->ai_addrlen) /* Unpossible. */
                errx(7, "expected %lu bytes of address data, got %d",
                                sizeof(struct sockaddr_in), res->ai_addrlen);
        sa = (struct sockaddr_in*)res->ai_addr;

        /* Roll our record. */
        p = arec;
        PUTSHORT((INDIR_MASK<<8 | NAMEOFF), p); /* Name in first question. */
        PUTSHORT(T_A, p);                  /* Type */
        PUTSHORT(C_IN, p);                 /* Class */
        PUTLONG (ttl, p);                  /* TTL */
        PUTSHORT(4, p);                    /* Length */
        PUTLONG (htonl(sa->sin_addr.s_addr), p); /* IP Address. */

        freeaddrinfo(res);
}

/* handle_query reads a query from s and sends back a reply. */
void
handle_query(int s)
{
        ssize_t nr, ns, reslen;
        struct sockaddr_in sa;
        socklen_t alen;
        HEADER *hdr;
        unsigned char *namep, *bufp, *end;
        int i;

        /* Get a query. */
        memset(&sa, 0, sizeof(sa));
        alen = sizeof(sa);
        if (-1 == (nr = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&sa,
                                        &alen)))
                err(16, "recvfrom");
        hdr = (HEADER *)buf;
        bufp = buf + sizeof(HEADER);
        end = buf + nr;

        /* Make sure it's a query. */
        if (0 != hdr->qr) {
                lprintf(&sa, "Sent a response");
                return;
        } else if (QUERY != hdr->opcode) {
                lprintf(&sa, "Invalid opcode %u", ntohs(hdr->opcode));
                return;
        } else if (hdr->tc) {
                lprintf(&sa, "Truncated");
                return;
        } else if (0 == hdr->qdcount) {
                lprintf(&sa, "No questions");
        }

        /* Work out where the end of the first question is and copy its
         * qname. */
        memset(name, 0, sizeof(name));
        namep = name;
        while (bufp < end) { /* Get to end of name. */
                if (0 != (INDIR_MASK & *bufp)) {
                        /* Compression shouldn't happen in the first name. */
                        lprintf(&sa, "First qname compressed");
                        return;
                } else if  (0 == *bufp) {
                        /* 0-length label means we're done. */
                        *namep++ = '.';
                        *namep = '\0';
                        bufp++;
                        break;
                }
                /* We've got a label size. */
                if ((*bufp + 2) >= ((BUFLEN-1) - (namep - name))) {
                        lprintf(&sa,"Not enough namebuffer room for "
                                        "a %d-byte label after %s", *bufp,
                                        name);
                        return;
                }
                /* Save this label. */
                if (namep != name)
                        *namep++ = '.';
                memcpy(namep, bufp+1, *bufp);
                for (i = 0; i < *bufp; ++i)
                        if (!isldh(namep[i]))
                                namep[i] = '?';
                namep += *bufp;
                bufp += *bufp + 1;
        }
        /* Leave the class and type intact as well. */
        bufp += 2 * sizeof(uint16_t);

        /* We could, in theory, be sent past the end of the message if
         * someone's pulling a sneaky. */
        if (bufp > end) {
                lprintf(&sa, "Corrupt query: last label of question too long");
                return;
        }

        /* Make sure we have enough space for a response and stick in our 
         * record. */
        if (RECLEN > (BUFLEN - (bufp - buf))) {
                lprintf(&sa, "Query for %s too long (%u) for reply", name,
                                bufp - buf);
                return;
        }
        memcpy(bufp, arec, sizeof(arec));
        reslen = (bufp - buf) + sizeof(arec);

        /* Responsify the header. */
        hdr->qr = 1;
        hdr->aa = 1;
        hdr->ra = 1;
        hdr->qdcount = htons(1);
        hdr->ancount = htons(1);
        hdr->nscount = 0;
        hdr->arcount = 0;

        /* Log it and send it back. */
        if (-1 == (ns = sendto(s, buf, reslen, 0, (const struct sockaddr *)&sa,
                                        sizeof(sa)))) {
                lprintf(&sa, "Sending reply: %s", strerror(errno));
                return;
        } else if (reslen != ns) {
                lprintf(&sa, "Sent %d/%d response bytes", ns, reslen);
                return;
        }
        lprintf(&sa, "%u %s", ntohs(hdr->id), name);
}
