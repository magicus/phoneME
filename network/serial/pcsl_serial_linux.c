/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <pcsl_network_serial.h>

/**
 * See pcsl_network_serial.h for definition
 */
int pcsl_serial_open(int port_id) {
	int fd;
    char port_name[20];
	struct termios termio;
    char* PORT_NAME = "/dev/ttyS";

    sprintf(port_name, "%s%d", PORT_NAME, port_id);

	//fd = open("/dev/ttyS0", O_RDWR | O_NONBLOCK | O_NOCTTY);
	fd = open(port_name, O_RDWR | O_NONBLOCK | O_NOCTTY);

	if (fd == -1) {
		if (errno == EACCES) {
			printf("No permission. Switch to root user and run again.\n");
		} else {
			printf("Unknown error occured while open\n");
		}
		return -1;
	}

	if (tcgetattr(fd, &termio) != 0) {	
		printf("Can't tcgetattr\n");
		return -1;
	}

	termio.c_iflag = termio.c_oflag = 0;
	termio.c_cflag &= ~(CSIZE | CSTOPB | PARENB);
	termio.c_cflag |= (CS8);
	termio.c_lflag &= ~(ICANON | ISIG | ECHO); 
	termio.c_cc[VMIN] = 1;
	termio.c_cc[VTIME] = 0;

	cfsetispeed(&termio, B9600);
	cfsetospeed(&termio, B9600);

	if (tcsetattr(fd, TCSANOW, &termio) < 0) {
		printf("Can't set serial port par\n");
		return -1;
	}

	return fd;
}

/**
 * See pcsl_network_serial.h for definition
 */
int pcsl_serial_readchar(int port, char* buffer, int size) {
    int result;
    result = read(port, buffer, size);
    return result;
}

/**
 * See pcsl_network_serial.h for definition
 */
int pcsl_serial_writechar(int port, char* buffer, int size) {
    int result;
	result = write(port, buffer, size);
	return result;
}

/**
 * See pcsl_network_serial.h for definition
 */
int pcsl_serial_close(int port) {
    int result;
    result = close(port);
    return result;
}

/**
 * See pcsl_network_serial.h for definition
 */
void pcsl_serial_flush(int port) {
    tcflush(port, TCIOFLUSH);
}



