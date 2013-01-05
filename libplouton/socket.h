/**
 * Copyright 2011 by Tor E. Hagemann <hagemt@rpi.edu>
 * This file is part of Plouton.
 *
 * Plouton is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Plouton is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Plouton.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

int server_socket(const char * port);
int client_socket(const char * port);

/*
void destroy_socket(int);
int create_socket(const char *port, struct sockaddr_in *local_addr);
*/

#include <stdio.h>

inline void
__hexdump(FILE * fp, unsigned char * buffer, size_t len) {
	size_t i, j;
	unsigned int mark;

	/* While there's still buffer, handle lines in sixteen bytes per row */
	for (mark = 0x00, i = 0; i < len; mark += 0x10, i = j) {
		/* First, print index marker */
		fprintf(fp, "[%08X] ", mark);
		for (j = i; j < len && (j - i) < 16; ++j) {
			fprintf(fp, "%X%X ", (buffer[j] & 0xF0) >> 4, (buffer[j] & 0x0F));
		}

		/* Pad with spaces as necessary */
		for (j -= i; j < 16; ++j) {
			fprintf(fp, "   ");
		}

		/* Tack on the ASCII representation, dots for non-printables */
		putc(' ', fp);
		for (j = i; j < len && (j - i) < 16; ++j) {
			putc((buffer[j] < ' ' || buffer[j] > '~') ? '.' : buffer[j], fp);
		}
		putc('\n', fp);
	}
}

#endif /* SOCKET_UTILS_H */
