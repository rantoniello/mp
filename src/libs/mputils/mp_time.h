/*
 * Copyright (c) 2017, 2018, 2019, 2020, 2021 Rafael Antoniello
 *
 * This file is part of MediaProcessors.
 *
 * MediaProcessors is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MediaProcessors is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with MediaProcessors. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file mp_time.h
 * @author Rafael Antoniello
 */

#ifndef MPUTILS_MP_TIME_H_
#define MPUTILS_MP_TIME_H_

#include <sys/types.h>
#include <inttypes.h>

/* **** Definitions **** */

/* Forward declarations */
typedef struct mp_log_ctx_s mp_log_ctx_t;

typedef int(*mp_clock_gettime_fxn)(clockid_t clockid, struct timespec *tp);

/* **** Prototypes *****/

/**
 * This function internally calls 'clock_gettime' with clock-id
 * CLOCK_MONOTONIC_COARSE but returning a 64-bit unsigned integer representing
 * the time in milliseconds.
 * @param mp_log_ctx Pointer to the log module context structure.
 * @return A 64-bit unsigned integer representing the time in milliseconds.
 */
uint64_t mp_gettime_monotcoarse_msecs(mp_log_ctx_t *mp_log_ctx);

/**
 * This function internally calls 'clock_gettime' with clock-id
 * CLOCK_MONOTONIC but returning a 64-bit unsigned integer representing
 * the time in milliseconds.
 * @param mp_log_ctx Pointer to the log module context structure.
 * @return A 64-bit unsigned integer representing the time in milliseconds.
 */
uint64_t mp_gettime_monot_msecs(mp_log_ctx_t *mp_log_ctx);

extern mp_clock_gettime_fxn mp_clock_gettime;

#endif /* MPUTILS_MP_TIME_H_ */
