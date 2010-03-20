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

// Xdotool for keyboard emulation
#include	<xdo.h>

const char* program_name;
int nofork_flag = 0; /* 1 == run in foreground */

void usage(void);

int
main(int argc, char *argv[])
{
    int next_option;
    const char *short_options = "hH:p:f";
    static const struct option long_options[]=
    {
	{"help", optional_argument, NULL, 'h'},
	{"host", optional_argument, NULL, 'H'},
	{"port", optional_argument, NULL, 'p'},
	{"foreground", optional_argument, NULL, 'f'},
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
		printf("%s\n", optarg);
		break;
	    default:
		/* do nothing */
		break;
	}
    }
    while(next_option != -1);


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



