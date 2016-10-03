#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>

#include "tcpardu.h"
#include "tools.h"
#include "serial.h"
#include "tcp.h"
#include "mqtt.h"

/********** defaults **********/

#define VERSION         "0.10"
#define EXE_NAME        "tcpardu"

/********** end defaults **********/

int gTcpPort = 0;
int gIsShutdown = FALSE;

/* main */
int main(int argc, char *argv[]) {
	signal(SIGINT, handle_ctrl_c);
	signal(SIGTERM, handle_kill);
	setDevicePathDefaults();
	int result = processCommandLineArguments(argc, argv);
	if (result == RETURN_ERROR) {
		return 1;
	} else if (result == RETURN_STOP) {
		return 0;
	}
	result = validateConfiguration();
	if (result == RETURN_OK) {
		if (gTcpPort > 0) { 
			startTcpServer(gTcpPort);
		}
		if (getMqttBrokerHost() != NULL) {
			connectMqtt();
		}
		executionLoop();
		if (getMqttBrokerHost() != NULL) {
			disconnectMqtt();
		}
		return 0;
	}
	return 1;
}

void executionLoop() {
	int serialDetectTrigerCountdown = 0;
	while (!gIsShutdown) {
		// detect serial devices
		if (serialDetectTrigerCountdown == 0) {
			detectSerialDevices();
			serialDetectTrigerCountdown = WAIT_TEN_SEC_BEFORE_REDETECTING_SERIAL_DEVICES;
		} else {
			serialDetectTrigerCountdown--;
		}
		// prepare select
		int maxfd = 0;
		int selectResult;
		fd_set readfds;
		fd_set writefds;
		struct timeval tv;

		prepareSelectSets(&readfds, &writefds, &maxfd);
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		selectResult = select(maxfd, &readfds, &writefds, NULL, &tv);
		if (selectResult < 0) {
			if (errno != EINTR)
				log(TL_ERROR, "Main: Select error %d.", selectResult);
			break;
		} else if (selectResult > 0) {
			if (isServerStarted()) {
				handleTcpRead(&readfds);
			}
			handleSerialRead(&readfds);
		}
	}
	cleanupSerial();
}

void prepareSelectSets(fd_set *pRS, fd_set *pWS, int *pMaxFD) {
	FD_ZERO(pRS);
	FD_ZERO(pWS);
	int lMaxFD;
	if (isServerStarted()) {
		prepareTcpSelectSets(pRS, &lMaxFD);
	}
	prepareSerialSelectSets(pRS, pWS, &lMaxFD);
	*pMaxFD = lMaxFD + 1;
}

void handle_ctrl_c(int pVal) {
	log(TL_INFO, "Signal: Ctrl+c detected, quit");
	shutDown();
}

void handle_kill(int pVal) {
	log(TL_INFO, "Signal: SIGTERM detected, quit");
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
					"  -?,  --help             Prints this help information\n"
					"  -e,  --verbose          Debug output\n"
					"  -v,  --version          Print version information\n\n"
					"  -t,  --list             List detected serial devices\n\n"
					"  -d,  --devices path     Sets serial devices path (default \"/dev/serial/by-id\")\n"
					"  -f,  --filter name      Device name filter (default \"arduino\")\n"
					"  -h,  --http             Http mode (default is telnet mode)\n"
					"Mandatory configuration:\n"
					"  -p,  --port number    Runs TCP server at port\n"
					"    OR\n"
					"  -m,  --mqtt number    Connect to mqtt broker at standard port 1883\n",
			exec);
}

void shutDown() {
	printf("Shutting down\n");
	gIsShutdown = TRUE;
}

int processCommandLineArguments(int argc, char *argv[]) {
	/* getopt variables */
	int ch, option_index;
	static struct option long_options[] = {
			{ "port", 1, 0, 'p' },
			{ "mqtt", 1, 0, 'm' },
			{ "help", 0, 0, '?' },
			{ "version", 0, 0, 'v' },
			{ "verbose", 0, 0, 'e' },
			{ "list", 0, 0, 't' },
			{ "devices", 1, 0, 'd' },
			{ "filter", 1, 0, 'f' },
			{ "http", 0, 0, 'h' },
			{ 0, 0, 0, 0 }
	};

	/* process command line arguments */
	while (1) {
		ch = getopt_long(argc, argv, "p:m:?vetd:f:h", long_options, &option_index);
		if (ch == -1)
			break;
		switch (ch) {
		case '?':
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
			log(TL_DEBUG, "Main: Setting tcp port to %d", gTcpPort);
			break;
			/* verbose */
		case 'e':
			setTraceLevel(TL_DEBUG);
			log(TL_DEBUG, "Main: Trace level set to DEBUG");
			break;
			/* list serials */
		case 't':
			listSerialDevices();
			return RETURN_STOP;
			break;
			/* device path */
		case 'd':
			setDevicePath(optarg);
			log(TL_DEBUG, "Main: Setting device path to '%s'", optarg);
			break;
			/* device filter */
		case 'f':
			setDeviceFilter(optarg);
			log(TL_DEBUG, "Main: Setting device filter to '%s'", optarg);
			break;
			/* http mode */
		case 'h':
			setHttpMode();
			log(TL_INFO, "Main: Using http mode");
			break;
			/* mqtt */
		case 'm':
			setMqttBrokerHost(optarg);
			log(TL_DEBUG, "Main: Setting mqtt broker to '%s'", optarg);
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
	if (gTcpPort <= 0 && getMqttBrokerHost() == NULL) {
		log(TL_ERROR, "Tcp port must be set to correct number or mqtt host must be set.\n\n");
		print_help(stdout, EXE_NAME);
		return RETURN_ERROR;
	}
	return RETURN_OK;
}
