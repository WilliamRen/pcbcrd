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

const char* program_name;
int nofork_flag = 0; /* 1 == run in foreground */
char *host, *port;

void usage(void);
void run_daemon(void);
void sighandle(int signo);
void * type_barecode(void *arg);

int
main(int argc, char *argv[])
{
    int next_option;
    const char *short_options = "hfH:p:";
    static const struct option long_options[]=
    {
	{"help", no_argument, NULL, 'h'},
	{"host", required_argument, NULL, 'H'},
	{"port", required_argument, NULL, 'p'},
	{"foreground", no_argument, NULL, 'f'},
	{NULL, 0, NULL, 0} /* end of array */
    };

    program_name = argv[0];
    do
    {
	next_option = getopt_long(argc, argv, short_options, long_options, NULL);

	switch (next_option)
	{
	    case 'h':
		usage();
		break;
	    case 'H':
		host = strdup(optarg);
		break;
	    case 'p':
		port = strdup(optarg);
		break;
	    case 'f':
		nofork_flag++;
		break;
	    default:
		/* do nothing */
		break;
	}
    }
    while(next_option != -1);

    if (!nofork_flag) {
	if (fork())
	    exit(0);
	FILE *pidfile;
	pidfile = fopen("/tmp/pcbcrd.pid", "w+");
	fprintf(pidfile, "%i\n", (int)getpid());
	fclose(pidfile);
	printf("forked, pid: %i\n", (int)getpid());
    }
    /* ignore SIGPIPE and catch SIGTERM|INT */
    if (signal(SIGPIPE, sighandle) == SIG_ERR)
            fprintf(stderr, "can't catch SIGPIPE signal");
    if (signal(SIGTERM, sighandle) == SIG_ERR)
            fprintf(stderr, "can't catch SIGTERM signal");
    if (signal(SIGINT, sighandle) == SIG_ERR)
            fprintf(stderr, "can't catch SIGINT signal");
    if (signal(SIGALRM, sighandle) == SIG_ERR)
            fprintf(stderr, "can't catch SIGALRM signal");
    if (signal(SIGHUP, sighandle) == SIG_ERR)
            fprintf(stderr, "can't catch SIGHUP signal");


    (void)run_daemon();


    return EXIT_SUCCESS;
}

void
usage(void)
{
    printf("usage: %s [-hHpf]\n", program_name);
    printf("\t-h --help\t\tDisplay help\n");
    printf("\t-H --host [host]\tHost to listen to\n");
    printf("\t-p --port [port]\tPort to listen to\n");
    printf("\t-f --foreground\t\tRun in foreground\n");

    exit (-1);
}


void
sighandle(int signo)
{
        if ((signo == SIGTERM) || (signo == SIGINT)) {
                printf("Exiting.\n");
		unlink("/tmp/pcbcrd.pid");
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
    int yes = 1, err;
    pthread_t tid;
    struct  timeval sock_timeout = {5, 0};
    struct  linger lng = {1, 5};


    if (port == NULL)
	port = "13000";
    if (host == NULL)
	host = "localhost";
    printf("Listening on %s:%s\n", host, port);


    struct addrinfo hint, *res;
    bzero(&hint, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_family = PF_INET;

    hint.ai_flags |= AI_CANONNAME;

    int tmp;
    if (tmp = getaddrinfo(host, port, &hint, &res))
    {
	fprintf(stderr, "getaddrinfo : %s", gai_strerror(tmp));
    	exit(-1);
    }

    if ((srv_fd = socket(res->ai_family,
		    res->ai_socktype,
		    res->ai_protocol)) == -1)
    {
	fprintf(stderr, "socket error");
	exit(-1);
    }

    struct sockaddr_storage srv_addr;
    memcpy(&srv_addr, res->ai_addr, res->ai_addrlen);

    if (setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
	fprintf(stderr, "setsockopt failed");
	exit(-1);
    }

    setsockopt(srv_fd, SOL_SOCKET, SO_RCVTIMEO,
	    &sock_timeout, sizeof(struct timeval));
    setsockopt(srv_fd, SOL_SOCKET, SO_SNDTIMEO,
	    &sock_timeout, sizeof(struct timeval));
    setsockopt(srv_fd, SOL_SOCKET, SO_LINGER,
	    &lng, sizeof(struct linger));

    if (bind(srv_fd, (struct sockaddr *)&srv_addr, res->ai_addrlen) == -1)
    {
	fprintf(stderr, "bind failed");
	exit(-1);
    }

    if (listen(srv_fd, 5) < 0)
    {
	fprintf(stderr, "listen error");
	exit(-1);
    }

    while(1) {
	sin_size = sizeof(struct sockaddr_in);
	if ((clt_fd = accept(srv_fd,
			(struct sockaddr *)&clt_addr,
			&sin_size)) < 0) {
	    continue;
	}

	err = pthread_create(&tid, NULL, type_barecode, (void *)clt_fd);
	if (err != 0)
	    fprintf(stderr, "error creating thread, err: %d", err);
    }
	/* NEVER REACHED */
}

void *
type_barecode(void *arg)
{
    int fd = (int)arg;
    pthread_detach(pthread_self());

    char buffer[MAXLEN];
    int n = recv(fd, buffer, MAXLEN, 0);
    buffer[strlen(buffer) -1]='\0';
    if (n == -1)
	goto error0;
    if (n == 0)
	goto error0;

    fprintf(stderr, "received: %s", buffer);

    char *ack = "ack\n";
    int senderr = send(fd, ack, strlen(ack), 0);
    if (senderr < 0)
    {
	fprintf(stderr, "send() error");
	exit(-1);
    }

error0:
    close(fd);
    pthread_exit(NULL);
}


