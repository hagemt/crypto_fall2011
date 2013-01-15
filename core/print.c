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

#include "libplouton.h"

#include <stdio.h>
#include <stdlib.h>

static inline void
__printx(FILE *fp, unsigned char c)
{
	fprintf(fp, "%X%X ", (c & 0xF0) >> 4, (c & 0x0F));
}

#define BANKING_STEP 0x10
#define BANKING_CHAR(x) ((x < ' ' || x > '~') ? '.' : x)

void
hexdump(FILE *fp, unsigned char *data, size_t len) {
	size_t i, pos;
	unsigned int mark;

	/* While there's still data, handle sixteen-byte chunks */
	for (mark = 0x00, pos = 0; pos < len; mark += BANKING_STEP, pos = i) {
		/* First, print row index marker, width eight */
		fprintf(fp, "[%08X] ", mark);

		/* Then, (up to) sixteen hex pairs (bytes) per line */
		for (i = pos; i < pos + BANKING_STEP; ++i) {
			if (i < len) {
				__printx(fp, data[i]);
			} else {
				fprintf(fp, "   ");
			}
		}
		putc(' ', fp);

		/* Tack on the ASCII representation, dots for non-printables */
		for (i = pos; i < pos + BANKING_STEP; ++i) {
			putc((i < len) ? BANKING_CHAR(data[i]) : ' ', fp);
		}
		putc('\n', fp);
	}
}

/*! \brief Print a labeled hex-formated representation of a string
 *
 *  Format: [label]: XX XX XX XX XX XX XX XX ([len * 8] bits)\n
 */
void
fprintx(FILE *fp, const char *label, unsigned char *data, size_t len)
{
	size_t i;
	fprintf(fp, "%s: ", label);
	for (i = 0; i < len; ++i) {
		/* TODO account for longer lines more neatly: */
		if (i && (i * 2) % BANKING_MAX_COMMAND_LENGTH == 0) {
			fprintf(fp, "\n     ");
		}
		__printx(fp, data[i]);
	}
	fprintf(fp, "(%u bits)\n", (unsigned) (len * 8));
}

