/*
 * arecres.h
 * Respond to all DNS queries with a A record.
 * By J. Stuart McMurray
 * Created 20230505
 * Last Modified 20230507
 */

#ifndef HAVE_ARECRES_H
#define HAVE_ARECRES_H

/* Default flag values. */
#define DEFADDR "0.0.0.0" /* Listen address. */
#define DEFPORT "53"      /* Listen port. */
#define DEFTTL  "300"     /* Response TTL. */


/* For Linux. */
#ifndef __OpenBSD__
int pledge(const char *, const char *) {return 0;}
#endif /* #ifndef __OpenBSD__ */

#endif /* #ifndef HAVE_ARECRES_H */
