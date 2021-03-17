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
 * @file mp_time.c
 * @author Rafael Antoniello
 */

#include "mp_time.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <inttypes.h>

#include "mp_log.h"
#include "mp_check.h"

/* **** Definitions **** */

#define GETTIME(CLOCKID, TS) \
    MP_CHECK(mp_clock_gettime(CLOCKID, &TS) == 0, \
            TS.tv_sec = 0; TS.tv_nsec = 0);

#define TIMESPEC2MSEC(RET_MSEC, TS) \
    RET_MSEC  = (uint64_t)TS.tv_sec * 1000;\
    RET_MSEC += (uint64_t)TS.tv_nsec / 1000000;

#define MP_GETTIME_GENERIC(CLOCKID, TRANSFORM_MACRO, LOGCTX) \
    MP_LOG_INIT(LOGCTX);\
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 0};\
    uint64_t retval;\
    GETTIME(CLOCKID, ts)\
    TRANSFORM_MACRO(retval, ts)\
    return retval;

/* **** Implementations **** */

mp_clock_gettime_fxn mp_clock_gettime = clock_gettime;

uint64_t mp_gettime_monotcoarse_msecs(mp_log_ctx_t *mp_log_ctx)
{
    MP_GETTIME_GENERIC(CLOCK_MONOTONIC_COARSE, TIMESPEC2MSEC, mp_log_ctx)
}

uint64_t mp_gettime_monot_msecs(mp_log_ctx_t *mp_log_ctx)
{
    MP_GETTIME_GENERIC(CLOCK_MONOTONIC, TIMESPEC2MSEC, mp_log_ctx)
}
