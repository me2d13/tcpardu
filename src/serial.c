/*
 * serial.c
 *
 *  Created on: Mar 20, 2016
 *      Author: petr
 */

#include <dirent.h>
#include <stdio.h>

void listSerialDevices(char *deviceRoot, char *deviceFilter) {
	DIR *d;
	struct dirent *dir;
	d = opendir(deviceRoot);
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			printf("%s\n", dir->d_name);
		}
		closedir(d);
	}
}
