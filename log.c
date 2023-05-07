/*
 * log.c
 * Nicer logging
 * By J. Stuart McMurray
 * Created 20230507
 * Last Modified 20230507
 */

#include <arpa/inet.h>
#include <netinet/in.h>

#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "log.h"

/* Sizes and offsets. */
#define TIMELEN (20)   /* Size of timestamp buffer. */

char timestamp[TIMELEN+1]; /* Timestamp buffer. */

/* lprintf logs a message to stdout with a timestamp.  If sa is not NULL, it
 * will be logged before the message. */
void
lprintf(const struct sockaddr_in *sa, const char *format, ...)
{
        time_t clock;
        va_list ap;

        /* Print a timestamp. */
        clock = time(NULL);
        if (0 == strftime(timestamp, TIMELEN, "%F %T", localtime(&clock)))
                err(14, "strftime");
        printf("%s ", timestamp);
        /* If we have an address, log that as well. */
        if (NULL != sa)
                printf(ADDRFMT " ", ADDRSTR(sa));
        /* Print the message. */
        va_start(ap, format);
        vprintf(format, ap);
        va_end(ap);
        printf("\n");
}

