/* Program ------------------------------------------------------------------
 * Description - Poll /proc/meminfo periodically for data.
 * License     - See COPYING file
 *----------------------------------------------------------------------------*/
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define MAXLEN 256
#define PACKAGE "mmw"
#define VERSION "2.0"
#define MEMINFO "/proc/meminfo"

void version(void);
void usage(void);
void print_head(unsigned long int fws, char *units, unsigned int dsize);
void read_meminfo(unsigned int polls, unsigned int interval,
		  unsigned int sf, unsigned int hflag, unsigned int dsize);

/* simple reusable version print */
void version()
{
	printf("%s %s\n", PACKAGE, VERSION);
}

/* reusable usage print */
void usage()
{
  printf(PACKAGE " [options][arguments]\n"
         PACKAGE " [-h|--human][-i|--interval][-p|--polls][-s][-u][-v]\n"
                 "Options\n"
                 "  -h|--human             Human readable format\n"
                 "  -i|--interval SECONDS  Seconds between polls\n"
                 "  -p|--polls    NPOLLS   Times to poll\n"
                 "  -s|--swap              Poll swap information as well\n"
                 "  -u|--usage             Print usage message\n"
                 "  -v|--version           Print version and exit\n");
}

/* simple header */
void print_head(unsigned long int fws, char *units, unsigned int dsize)
{
	int count;

	static char *header[] = {
		"total",
		"free",
		"shared",
		"buffer",
		"cached",
		"swap",
		"sfree",
	};

	printf("Memory Usage in: %s\n", units);

	/* determine the field width for the header */
	if (fws <= 100000) { /* MB */
		for (count = 0; count <= dsize; count++)
			printf("%-8s", header[count]);
	} else if (fws <= 10000000) { /* 1/10 GB */
		for (count = 0; count <= dsize; count++)
			printf("%-9s", header[count]);
	} else if (fws <= 100000000) { /* GB */
		for (count = 0; count <= dsize; count++)
			printf("%-11s", header[count]);
	} else if (fws > 100000000) { /* TB */
		for (count = 0; count <= dsize; count++)
			printf("%-14s", header[count]);
	} else { /* parallel universe */
		for (count = 0; count <= dsize; count++)
			printf("%-15s", header[count]);
	}

	printf("\n");
}

/* recursively prints out mem information */
void read_meminfo(unsigned int polls, unsigned int interval,
		  unsigned int sf, unsigned int hflag, unsigned int dsize)
{
	FILE *fp;
	static char ch[MAXLEN];
	unsigned long int mem_array[7];
	unsigned int count;
	long int hdiv;
	char *units;
	short int i;

	units = "kB";
	hdiv = 1;

	/* initialize the array */
	for (i = 0; i <= (dsize + 1); i++)
		mem_array[i] = 0;

	if (polls != 0) {
		if ((fp = fopen(MEMINFO, "r")) == NULL) {
			fprintf(stderr, "could not open file %s\n", MEMINFO);
			exit(EXIT_FAILURE);
		}

		/* Populate the mem_array data structure */
		while (fgets(ch, MAXLEN, fp) != NULL) {
			if (!strncmp(ch, "MemTotal:", 9)) {
				sscanf(ch + 10, "%lu", &mem_array[0]);
			} else if (!strncmp(ch, "MemFree:", 8)) {
				sscanf(ch + 10, "%lu", &mem_array[1]);
			} else if (!strncmp(ch, "MemShared:", 10)) {
				sscanf(ch + 10, "%lu", &mem_array[2]);
			} else if (!strncmp(ch, "Buffers:", 8)) {
				sscanf(ch + 10, "%lu", &mem_array[3]);
			} else if (!strncmp(ch, "Cached:", 7)) {
				sscanf(ch + 10, "%lu", &mem_array[4]);
			} else if ((dsize > 4)
				   && (!strncmp(ch, "SwapTotal", 9))) {
				sscanf(ch + 10, "%lu", &mem_array[5]);
			} else if ((dsize > 4) && (!strncmp(ch, "SwapFree", 8))) {
				sscanf(ch + 10, "%lu", &mem_array[6]);
			}
		}

		fclose(fp);	/* all done - close /proc/meminfo */

		/* If human flag is set change the human-readable fraction number */
		if (hflag) {
			if (mem_array[0] <= 999) {
				hdiv = 1;
			} else if (mem_array[0] <= 999999) {
				hdiv = 1000;
				units = "MB";
			} else if (mem_array[0] <= 99999999) {
				hdiv = 1000;
				units = "MB";
			} else if (mem_array[0] <= 999999999) {
				hdiv = (1000000);
				units = "GB";
			} else {
				hdiv = (1000000000);
				units = "TB";
			}
		}

		/* If this is the first line print the header */
		if (sf == 1)
			print_head(mem_array[0] / hdiv, units, dsize);

		/* determine the field width for each printout */
		if (mem_array[0] / hdiv <= 100000) {
			for (count = 0; count <= dsize; count++)
				printf("%-8li", mem_array[count] / hdiv);
		} else if (mem_array[0] / hdiv <= 100000000) {
			for (count = 0; count <= dsize; count++)
				printf("%-11li", mem_array[count] / hdiv);
		} else if (mem_array[0] / hdiv > 100000000) {
			for (count = 0; count <= dsize; count++)
				printf("%-14li", mem_array[count] / hdiv);
		} else {
			for (count = 0; count <= dsize; count++)
				printf("%-15li", mem_array[count] / hdiv);
		}

		printf("\n");

		if (--polls != 0)
			sleep(interval);

		read_meminfo(polls, interval, 0, hflag, dsize);

	}

}

/* main */
int main(int argc, char *argv[])
{
	int c, hflag, interval, poll, dsize;

	/* Defaults */
	interval = 5;
	poll = 5;
	hflag = 0;
	dsize = 4;

	/* default operation catch */
	if (argc == 1) {
		read_meminfo(poll, interval, 1, hflag, dsize);
		return EXIT_SUCCESS;
	}

	while (1) {
		static struct option long_options[] = {
			{"human", no_argument, 0, 'h'},
			{"interval", required_argument, 0, 'i'},
			{"poll", required_argument, 0, 'p'},
			{"swap", no_argument, 0, 's'},
			{"version", no_argument, 0, 'v'},
			{"usage", no_argument, 0, 'u'},
			{0, 0, 0, 0}	/* This is a filler for -1 */
		};

		int option_index = 0;

		c = getopt_long(argc, argv, "hi:p:svu", long_options,
				&option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'h':
			hflag = 1;
			break;
		case 'i':
			if (isalpha(*optarg)) {
				fprintf(stderr,
					"Error: interval must be a number\n");
				usage();
				return EXIT_FAILURE;
			}
			interval = atoi(optarg);
			break;
		case 'p':
			if (isalpha(*optarg)) {
				fprintf(stderr,
					"Error: poll must be a number\n");
				usage();
				exit(EXIT_FAILURE);
			}
			poll = atoi(optarg);
			break;
		case 's':
			dsize = (dsize + 2);
			break;
		case 'v':
			version();
			return EXIT_SUCCESS;
			break;
		case 'u':
			usage();
			return EXIT_SUCCESS;
			break;
		default:
			usage();
			return EXIT_FAILURE;
			break;
		}
	}

	read_meminfo(poll, interval, 1, hflag, dsize);
	return EXIT_SUCCESS;
}
