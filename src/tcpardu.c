#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "tcpardu.h"
#include "tools.h"
#include "serial.h"

/********** defaults **********/

#define VERSION         "0.10"
#define EXE_NAME        "tcpardu"

/********** end defaults **********/

int gTcpPort = 0;
char *deviceMask = "/dev/ttyACM";

/* main */
int main(int argc, char *argv[]) {
	signal(SIGINT, handle_ctrl_c);
	signal(SIGTERM, handle_kill);
	int result = processCommandLineArguments(argc, argv);
	if (result == RETURN_ERROR) {
		return 1;
	} else if (result == RETURN_STOP) {
		return 0;
	}
	result = validateConfiguration();
	if (result == RETURN_OK) {
		printf("Hello\n");
		return 0;
	}
	return 1;
}

void handle_ctrl_c(int pVal) {
	debugLog(TL_INFO, "Signal: Ctrl+c detected, quit");
	shutDown();
}

void handle_kill(int pVal) {
	debugLog(TL_INFO, "Signal: SIGTERM detected, quit");
	shutDown();
}

void print_version(FILE *stream) {
	fprintf(stream, "%s version %s\n", EXE_NAME, VERSION);
}

void print_help(FILE *stream, char *exec) {
	print_version(stream);
	fprintf(stream,
			"\nUsage: %s [OPTIONS]\n\n"
					"Informative output:\n"
					"  -h,  --help             Print help information\n"
					"  -e,  --verbose          Debug output\n"
					"  -v,  --version          Print version information\n\n"
					"  -t,  --list             List detected serial devices\n\n"
					"Mandatory configuration:\n"
					"  -p,  --port number    Runs TCP server at port\n",
			exec);
}

void shutDown() {
	printf("Shutting down\n");
}

int processCommandLineArguments(int argc, char *argv[]) {
	/* getopt variables */
	int ch, option_index;
	static struct option long_options[] = {
			{ "port", 1, 0, 'p' },
			{ "help", 0, 0, 'h' },
			{ "version", 0, 0, 'v' },
			{ "verbose", 0, 0, 'e' },
			{ "list", 0, 0, 't' },
			{ 0, 0, 0, 0 }
	};

	/* process command line arguments */
	while (1) {
		ch = getopt_long(argc, argv, "p:hvet", long_options, &option_index);
		if (ch == -1)
			break;
		switch (ch) {
		case 'h':
			print_help(stdout, argv[0]);
			return RETURN_STOP;
			break;
			/* version */
		case 'v':
			print_version(stdout);
			return RETURN_STOP;
			break;
			/* tcp port */
		case 'p':
			gTcpPort = atoi(optarg);
			debugLog(TL_DEBUG, "Main: Setting tcp port to %d", gTcpPort);
			break;
			/* verbose */
		case 'e':
			setTraceLevel(TL_DEBUG);
			debugLog(TL_DEBUG, "Main: Trace level set to DEBUG");
			break;
			/* list serials */
		case 't':
			listSerialDevices(deviceMask);
			break;
			/* default */
		default:
			print_help(stderr, argv[0]);
			return RETURN_STOP;
			break;
		}
	}
	return RETURN_OK;
}

int validateConfiguration() {
	if (gTcpPort <= 0) {
		debugLog(TL_ERROR, "Tcp port must be set to correct number.\n\n");
		print_help(stdout, EXE_NAME);
		return RETURN_ERROR;
	}
	return RETURN_OK;
}
