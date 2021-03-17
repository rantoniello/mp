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
 * @file mp_utils_tests_main.c
 * @author Rafael Antoniello
 */

#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "mp_log_tests.h"
#include "mp_time_tests.h"
#include "mp_shm_fifo_tests.h"

int main(void)
{
    int number_failed = 0, ret_code = EXIT_FAILURE;
    Suite *suites[] = {
            mp_log_suite(),
            mp_time_suite(),
            mp_shm_fifo_suite(),
            NULL
    };

    for (int i = 0; suites[i] != NULL; i++) {
        Suite *s = suites[i];
        SRunner *sr = srunner_create(s);
        srunner_run_all(sr, CK_NORMAL);
        number_failed += srunner_ntests_failed(sr);
        srunner_free(sr);
    }

    if (number_failed == 0) {
        printf("\nTESTS SUCCEED\n");
        ret_code = EXIT_SUCCESS;
    } else {
        printf("\nTEST FAILED (%d failures)\n", number_failed);
    }

    return ret_code;
}
