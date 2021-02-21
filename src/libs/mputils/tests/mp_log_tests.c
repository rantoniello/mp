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
 * @file mp_log_tests.c
 * @author Rafael Antoniello
 */

#include "mp_log_tests.h"

#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <mputils/mp_check.h>
#include <mputils/mp_log.h>
#include <mputils/mp_mem.h>

static void *fake_calloc_failure(size_t count, size_t eltsize)
{
    return NULL;
}

static void (log_ext_trace_fxn_1)(void *opaque_logger_ctx,
        mp_log_level_t mp_log_level, const char *file_name, int line,
        const char *func_name, const char *format, va_list args)
{
    printf("log-level = %d; File %s:Function %s:line %d: ", mp_log_level,
            file_name, func_name, line);
    vprintf(format, args);
    fflush(stdout);
}

START_TEST(test_mp_log_open)
{
    mp_log_ctx_t *mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    mp_log_close(&mp_log_ctx);
    ck_assert(mp_log_ctx == NULL);

    mp_calloc = fake_calloc_failure; // Mock calloc
    mp_log_ctx = mp_log_open(NULL, NULL);
    mp_calloc = calloc;
    ck_assert(mp_log_ctx == NULL);
    mp_log_close(&mp_log_ctx);
    ck_assert(mp_log_ctx == NULL);
}
END_TEST

START_TEST(test_mp_log_close)
{
    mp_log_close(NULL);

    mp_log_ctx_t *mp_log_ctx = NULL;
    mp_log_close(&mp_log_ctx);
    ck_assert(mp_log_ctx == NULL);

    mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    mp_log_close(&mp_log_ctx);
    ck_assert(mp_log_ctx == NULL);
}
END_TEST

START_TEST(test_mp_log_trace)
{
    mp_log_ctx_t *mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);

    mp_log_trace(NULL, MP_LOG_DBG, "myfile.c", 22, "myfxn", "%s\n",
                "this is a test traceline");
    mp_log_trace(mp_log_ctx, MP_LOG_ENUM_MAX, "myfile.c", 22, "myfxn", "%s\n",
                "this is a test traceline");
    mp_log_trace(mp_log_ctx, MP_LOG_DBG, NULL, 22, "myfxn", "%s\n",
                "this is a test traceline");
    mp_log_trace(mp_log_ctx, MP_LOG_DBG, "myfile.c", -1, NULL, "%s\n",
                "this is a test traceline");
    mp_log_trace(mp_log_ctx, MP_LOG_DBG, "myfile.c", 22, "myfxn", NULL);

    /* Simple successful case */
    mp_log_trace(mp_log_ctx, MP_LOG_DBG, "myfile.c", 22, "myfxn", "%s\n",
            "this is a test trace line");
    mp_log_close(&mp_log_ctx);
    ck_assert(mp_log_ctx == NULL);

    mp_log_ctx = mp_log_open(NULL, log_ext_trace_fxn_1);
    ck_assert(mp_log_ctx != NULL);
    mp_log_trace(mp_log_ctx, MP_LOG_DBG, "myfile.c", 22, "myfxn", "%s\n",
            "this is another test trace line");
    mp_log_close(&mp_log_ctx);
    ck_assert(mp_log_ctx == NULL);
}
END_TEST

Suite *mp_log_suite(void)
{
    Suite *s;
    TCase *tc_prototypes;

    s = suite_create("mp_log");

    /* Header file public prototypes test case */
    tc_prototypes = tcase_create("mp_log_prototypes");

    tcase_add_test(tc_prototypes, test_mp_log_open);
    tcase_add_test(tc_prototypes, test_mp_log_close);
    tcase_add_test(tc_prototypes, test_mp_log_trace);
    suite_add_tcase(s, tc_prototypes);

    return s;
}
