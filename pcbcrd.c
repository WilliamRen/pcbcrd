/* Copyright 2010 Marc Lagrange <rhaamo@gruik.at>

   This file is part of pcbcrd.

   pcbcrd is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   pcbcrd is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with pcbcrd.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * File: pcbcrd.c
 * Description: Main file of pcbcrd
 */

#include	<err.h>
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<getopt.h>
#include	<signal.h>

// Xdotool for keyboard emulation
#include	<xdo.h>

#include        <sys/socket.h>
#include        <netinet/in.h>
#include        <arpa/inet.h>
#include        <signal.h>
#include        <pthread.h>
#include        <netdb.h>

#include	<limits.h>
#define	MAXLEN	LINE_MAX

/* some useful define for default values */
#define	PCBRCD_DEFAULT_PIDFILE	"/tmp/pcbcrd.pid"
#define	PCBRCD_DEFAULT_PORT	"13000"
#define	PCBRCD_DEFAULT_HOST	"localhost"

int nofork_flag = 0; /* 1 == run in foreground */
int enter_flag = 0;  /* 1 == press enter after codebare typing */
char *host, *port;

void usage(void);
void run_daemon(void);
void sighandle(int signo);
void * get_barecode(void *arg);
void type_barecode(char *barecode);

int
main(int argc, char *argv[])
{
    int c;
    const char *short_options = "hfH:p:e";
    static const struct option long_options[]=
    {
	{"help", no_argument,       NULL, 'h'},
	{"host", required_argument, NULL, 'H'},
	{"port", required_argument, NULL, 'p'},
	{"foreground", no_argument, NULL, 'f'},
	{"enter", no_argument,	    NULL, 'e'},
	{NULL, 0, NULL, 0} /* end of array */
    };

	while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		switch ((char)c) {
		case 'H':
			host = strdup(optarg);
			break;
		case 'p':
			port = strdup(optarg);
			break;
		case 'f':
			nofork_flag++;
			break;
		case 'e':
			enter_flag++;
			break;
		default:  /* FALLTHROUGH */
		case '?': /* FALLTHROUGH */
		case 'h':
			usage();
			/* NOTREACHED */
		}
	}

    if (!nofork_flag) {
	if (fork())
	    exit(EXIT_SUCCESS);
	FILE *pidfile;
	pidfile = fopen(PCBRCD_DEFAULT_PIDFILE, "w");
	if (pidfile == NULL)
		err(EXIT_FAILURE, "couldn't open %s", PCBRCD_DEFAULT_PIDFILE);
	fprintf(pidfile, "%i\n", (int)getpid());
	fclose(pidfile);
	printf("forked, pid: %i\n", (int)getpid());
    }
    /* ignore SIGPIPE and catch SIGTERM|INT */
    if (signal(SIGPIPE, sighandle) == SIG_ERR)
            fprintf(stderr, "can't catch SIGPIPE signal\n");
    if (signal(SIGTERM, sighandle) == SIG_ERR)
            fprintf(stderr, "can't catch SIGTERM signal\n");
    if (signal(SIGINT, sighandle) == SIG_ERR)
            fprintf(stderr, "can't catch SIGINT signal\n");
    if (signal(SIGALRM, sighandle) == SIG_ERR)
            fprintf(stderr, "can't catch SIGALRM signal\n");
    if (signal(SIGHUP, sighandle) == SIG_ERR)
            fprintf(stderr, "can't catch SIGHUP signal\n");


    (void)run_daemon();


    return EXIT_SUCCESS;
}

void
usage(void)
{
    extern const char *__progname;
    printf("usage: %s [-hHpf]\n", __progname);
    printf("\t-h --help           Display help\n");
    printf("\t-H --host [host]    Host to listen to\n");
    printf("\t-p --port [port]    Port to listen to\n");
    printf("\t-f --foreground     Run in foreground\n");
    printf("\t-e --enter          Press enter after typing\n");

    exit(EXIT_FAILURE);
}


void
sighandle(int signo)
{
        if ((signo == SIGTERM) || (signo == SIGINT)) {
                printf("Exiting.\n");
		unlink(PCBRCD_DEFAULT_PIDFILE);
                exit(EXIT_SUCCESS);
        }

        if (signo == SIGHUP) {
        }

        if (signo == SIGALRM) {
        }
        /* TODO: SIGUSR2 and SIGUSR2 */
}

void
run_daemon(void)
{
    int clt_fd, srv_fd;
    struct sockaddr_in clt_addr;
    socklen_t sin_size;
    int yes = 1, eno;
    pthread_t tid;
    struct  timeval sock_timeout = {5, 0};
    struct  linger lng = {1, 5};


    if (port == NULL)
	port = PCBRCD_DEFAULT_PORT;
    if (host == NULL)
	host = PCBRCD_DEFAULT_HOST;
    printf("Listening on %s:%s\n", host, port);


    struct addrinfo hint, *res;
    bzero(&hint, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_family = PF_INET;

    hint.ai_flags |= AI_CANONNAME;

	int i = getaddrinfo(host, port, &hint, &res);
	if (i != 0)
		errx(EXIT_FAILURE, "getaddrinfo : %s", gai_strerror(i));

	srv_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (srv_fd == -1)
		err(EXIT_FAILURE, "socket");

    struct sockaddr_storage srv_addr;
    memcpy(&srv_addr, res->ai_addr, res->ai_addrlen);

	if (setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		err(EXIT_FAILURE, "setsockopt");

	/* XXX: ignore return values? */
    setsockopt(srv_fd, SOL_SOCKET, SO_RCVTIMEO,
	    &sock_timeout, sizeof(struct timeval));
    setsockopt(srv_fd, SOL_SOCKET, SO_SNDTIMEO,
	    &sock_timeout, sizeof(struct timeval));
    setsockopt(srv_fd, SOL_SOCKET, SO_LINGER,
	    &lng, sizeof(struct linger));

	if (bind(srv_fd, (struct sockaddr *)&srv_addr, res->ai_addrlen) == -1)
		err(EXIT_FAILURE, "bind");

	if (listen(srv_fd, 5) == -1)
		err(EXIT_FAILURE, "listen");

    while(1) {
	sin_size = sizeof(struct sockaddr_in);
	if ((clt_fd = accept(srv_fd,
			(struct sockaddr *)&clt_addr,
			&sin_size)) < 0) {
	    continue;
	}

	eno = pthread_create(&tid, NULL, get_barecode, (void *)clt_fd);
	if (eno != 0)
	    fprintf(stderr, "error creating thread: %d\n", eno);
    }
	/* NOTREACHED */
}

void *
get_barecode(void *arg)
{
    int fd = (int)arg;
    pthread_detach(pthread_self());

    char buffer[MAXLEN];
    int n = recv(fd, buffer, MAXLEN, 0);
    buffer[strlen(buffer) -1] = '\0';
    if (n == -1 || n == 0)
	goto error0;

    if (nofork_flag)
    	fprintf(stderr, "received: %s\n", buffer);
    (void)type_barecode(buffer);

    char *ack = "ack\n";
	if (send(fd, ack, strlen(ack), 0) == -1) {
		/* FIXME: should be syslog'd or ignored if we forked
		 	(!nofork_flag) */
		err(EXIT_FAILURE, "send");
	}

error0:
    close(fd);
    pthread_exit(NULL);
}

void
type_barecode(char *barecode)
{
	Window window = 0;
	useconds_t delay = 12000; /* 12ms between keystrokes default */
	xdo_t *xdo;

    	/* XXX: check DISPLAY? */
	if ((xdo = xdo_new(getenv("DISPLAY"))) == NULL)
		errx(EXIT_FAILURE, "Failed creating new xdo instance");

	if (xdo_type(xdo, window, barecode, delay) != 0) {
		fprintf(stderr, "xdo_type reported an error\n");
		/* XXX: return here? */
	}

	if (enter_flag) {
		if (xdo_keysequence(xdo, window, "Return") != 0)
			fprintf(stderr, "xdo_keysequence reported an error\n");
	}
	xdo_free(xdo);
}
