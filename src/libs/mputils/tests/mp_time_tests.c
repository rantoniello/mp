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
 * @file mp_time_tests.c
 * @author Rafael Antoniello
 */

#include "mp_time_tests.h"

#include <stdio.h>
#include <unistd.h>
#include <check.h>
#include <time.h>

#include <mputils/mp_check.h>
#include <mputils/mp_log.h>
#include <mputils/mp_time.h>

static int fake_clock_gettime_failure(clockid_t clockid, struct timespec *tp)
{
    return 1;
}

START_TEST(test_mp_gettime_monotcoarse_msecs)
{
    uint64_t t0 = 0, tcurr = 0, T0_msecs = 200;

    /* Open logger (just log to stdout) */
    mp_log_ctx_t *mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_INIT(mp_log_ctx);

    /* Get initial time */
    t0 = mp_gettime_monotcoarse_msecs(MP_LOG_GET());
    ck_assert(t0 > 0);

    tcurr = mp_gettime_monotcoarse_msecs(MP_LOG_GET());
    ck_assert(tcurr >= t0);

    usleep(1000 * T0_msecs);

    tcurr = mp_gettime_monotcoarse_msecs(MP_LOG_GET());
    ck_assert((tcurr >= t0) && (tcurr - t0 >= T0_msecs));

    // Force 'mp_clock_gettime' to fail
    mp_clock_gettime = fake_clock_gettime_failure; // Mock clock_gettime
    tcurr = mp_gettime_monotcoarse_msecs(MP_LOG_GET());
    mp_clock_gettime = clock_gettime; // restore
    ck_assert(tcurr == 0);

    mp_log_close(&mp_log_ctx);
    ck_assert(mp_log_ctx == NULL);
}
END_TEST

START_TEST(test_mp_gettime_monot_msecs)
{
    uint64_t t0 = 0, tcurr = 0, T0_msecs = 200;

    /* Open logger (just log to stdout) */
    mp_log_ctx_t *mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_INIT(mp_log_ctx);

    /* Get initial time */
    t0 = mp_gettime_monot_msecs(MP_LOG_GET());
    ck_assert(t0 > 0);

    tcurr = mp_gettime_monot_msecs(MP_LOG_GET());
    ck_assert(tcurr >= t0);

    usleep(1000 * T0_msecs);

    tcurr = mp_gettime_monot_msecs(MP_LOG_GET());
    ck_assert((tcurr >= t0) && (tcurr - t0 >= T0_msecs));

    // Force 'mp_clock_gettime' to fail
    mp_clock_gettime = fake_clock_gettime_failure; // Mock clock_gettime
    tcurr = mp_gettime_monot_msecs(MP_LOG_GET());
    mp_clock_gettime = clock_gettime; // restore
    ck_assert(tcurr == 0);

    mp_log_close(&mp_log_ctx);
    ck_assert(mp_log_ctx == NULL);
}
END_TEST

Suite *mp_time_suite()
{
    Suite *s;
    TCase *tc;

    s = suite_create("mp_time");
    tc = tcase_create("mp_time_testcase");

    tcase_add_test(tc, test_mp_gettime_monotcoarse_msecs);
    tcase_add_test(tc, test_mp_gettime_monot_msecs);
    suite_add_tcase(s, tc);

    return s;
}
