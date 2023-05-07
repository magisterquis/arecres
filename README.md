A Record Responder
==================
Minimal DNS server which responds to all queries with an A record.

Handy for catching exfil over DNS (e.g. from
[portscan2dns](https://github.com/magisterquis/portscan2dns)) without needing a
more complicated server.

For legal use only.

Features
--------
- IPv4-only
- Fast
- Smallish memory footprint
- Serves up a single A record to all queries

Quickstart
----------
```sh
git clone git@github.com:magisterquis/arecres.git
cd arecres
make
./arecres -h
./arecres $(curl -s ipv4.icanhazip.com)
```
On some systems (e.g. Linux), it may be necessary to use bmake instead of make.

Usage
-----
```
Usage: arecres [-h] [-l laddr] [-p lport] [-t ttl] addr

Responds with a single A record for addr (which may be a hostname) to all
DNS queries.

Options:
  -l laddr - Listen IPv4 address (default: 0.0.0.0)
  -p lport - Listen port (default: 53)
  -t ttl   - Response TTL (default: 300)
```
