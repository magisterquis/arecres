/*
 * log.h
 * Nicer logging
 * By J. Stuart McMurray
 * Created 20230507
 * Last Modified 20230507
 */

#ifndef HAVE_LOG_H
#define HAVE_LOG_H

/* Easy address printing. */
#define ADDRFMT "%s:%hu"
#define ADDRSTR(sa) (inet_ntoa((sa)->sin_addr)), (ntohs((sa)->sin_port))

void lprintf(const struct sockaddr_in *, const char *, ...);

#endif /* #ifndef HAVE_LOG_H */
