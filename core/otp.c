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

#ifndef NDEBUG
#include <stdio.h>
#endif /* DEBUG */

#include <sys/ipc.h>
#include <sys/shm.h>

#include "libplouton.h"

inline int *
new_shmid(int *shmid) {
	size_t len;
	char *shmem;

#ifndef NDEBUG
	fprintf(stderr, "INFO: using shared memory key: 0x%X\n",
			BANKING_AUTH_SHMKEY);
#endif

	len = sizeof(BANKING_MSG_AUTH_CHECK);
	*shmid = shmget(BANKING_AUTH_SHMKEY, len, IPC_CREAT | 0666);
	/* Try to fill the shared memory segment with OTP */
	if (*shmid != -1 && (shmem = shmat(*shmid, NULL, 0))) {
		snprintf(shmem, len, BANKING_MSG_AUTH_CHECK);
#ifndef NDEBUG
		fprintf(stderr, "INFO: shared memory contents: '%s'\n", shmem);
	} else {
		fprintf(stderr, "INFO: shared memory segment not found!\n");
#endif
	}

#ifndef NDEBUG
	fprintf(stderr, "INFO: new shared memory id: %i\n", *shmid);
#endif

	return shmid;
}

inline int *
old_shmid(int *shmid) {
	size_t len = sizeof(BANKING_MSG_AUTH_CHECK);
	*shmid = shmget(BANKING_AUTH_SHMKEY, len, IPC_EXCL | 0666);
#ifndef NDEBUG
	fprintf(stderr, "INFO: old shared memory id: %i\n", *shmid);
#endif
	return shmid;
}
