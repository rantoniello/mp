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
 * @file mp_shm_fifo_tests.c
 * @author Rafael Antoniello
 */

#include "mp_shm_fifo_tests.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <check.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <mputils/mp_check.h>
#include <mputils/mp_log.h>
#include <mputils/mp_mem.h>
#include <mputils/mp_statcodes.h>
#include <mputils/mp_time.h>
#include <mputils/mp_shm_fifo.h>

#define SHM_FIFO_MESSAGE_MAX_LEN 17
#define SHM_FIFO_NAME "/fifo_shm_utest"

START_TEST(test_mp_shm_fifo_create)
{
    /* Open logger (just log to stdout) */
    mp_log_ctx_t *mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_INIT(mp_log_ctx);

    /* Pass 'null' FIFO name */
    ck_assert(mp_shm_fifo_create(NULL, 12, 0, MP_LOG_GET()) == NULL);

    /* Try setting a zero sized FIFO */
    ck_assert(mp_shm_fifo_create("/anyname", 0, 0, MP_LOG_GET()) == NULL);

    /* Pass excessively long FIFO name */
    char bad_length_name[1024];
    memset(bad_length_name, 'F', sizeof(bad_length_name) - 1);
    bad_length_name[sizeof(bad_length_name) - 1] = '\0';
    ck_assert(mp_shm_fifo_create(bad_length_name, 1, 0, MP_LOG_GET()) == NULL);

    /* Succeed creating the FIFO */
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx = mp_shm_fifo_create(SHM_FIFO_NAME, 16,
            0, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx != NULL);
    mp_shm_fifo_release(&mp_shm_fifo_ctx, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx == NULL);

    mp_log_close(&mp_log_ctx);
}
END_TEST

START_TEST(test_mp_shm_fifo_release)
{
    /* Open logger (just log to stdout) */
    mp_log_ctx_t *mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_INIT(mp_log_ctx);

    mp_shm_fifo_release(NULL, NULL);

    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx = NULL;
    mp_shm_fifo_release(&mp_shm_fifo_ctx, NULL);
    mp_shm_fifo_release(&mp_shm_fifo_ctx, mp_log_ctx);

    mp_shm_fifo_ctx = mp_shm_fifo_create(SHM_FIFO_NAME, 1, 0, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx != NULL);
    mp_shm_fifo_release(&mp_shm_fifo_ctx, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx == NULL);

    mp_log_close(&mp_log_ctx);
}
END_TEST

START_TEST(test_mp_shm_fifo_get_buffer_level)
{
    mp_log_ctx_t *mp_log_ctx;
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx;
    const size_t max_data_size = SHM_FIFO_MESSAGE_MAX_LEN,
            shm_pool_size = sizeof(ssize_t) + max_data_size;
    MP_LOG_INIT(NULL);

    /* Open logger (just log to stdout) */
    mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_SET(mp_log_ctx);

    /* Create SHM-FIFO */
    mp_shm_fifo_ctx = mp_shm_fifo_create(SHM_FIFO_NAME, shm_pool_size,
            SHM_FIFO_EXHAUST_CTRL, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx != NULL);

    mp_shm_fifo_set_blocking_mode(mp_shm_fifo_ctx, 1, MP_LOG_GET());
    mp_shm_fifo_empty(mp_shm_fifo_ctx, MP_LOG_GET());
    ck_assert(mp_shm_fifo_get_buffer_level(mp_shm_fifo_ctx, MP_LOG_GET())
            == 0);

    ck_assert(mp_shm_fifo_push(mp_shm_fifo_ctx, "abcdefghijklmno\0", 16,
            MP_LOG_GET()) == MP_SUCCESS);
    ck_assert(mp_shm_fifo_get_buffer_level(mp_shm_fifo_ctx, MP_LOG_GET()) ==
            (16 + sizeof(ssize_t)));

    /* Test bad arguments */
    ck_assert(mp_shm_fifo_get_buffer_level(NULL, MP_LOG_GET()) == -1);

    /* Unblock FIFO and release */
    mp_shm_fifo_set_blocking_mode(mp_shm_fifo_ctx, 0, MP_LOG_GET());
    mp_shm_fifo_release(&mp_shm_fifo_ctx, MP_LOG_GET());
    mp_log_close(&mp_log_ctx);
}
END_TEST

START_TEST(test_mp_shm_fifo_pull_bad_args)
{
    mp_log_ctx_t *mp_log_ctx;
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx;
    const size_t max_data_size = SHM_FIFO_MESSAGE_MAX_LEN,
            shm_pool_size = sizeof(ssize_t) + max_data_size;
    MP_LOG_INIT(NULL);
    uint8_t *elem = NULL;
    size_t elem_size = 0;

    /* Open logger (just log to stdout) */
    mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_SET(mp_log_ctx);

    /* Create SHM-FIFO */
    mp_shm_fifo_ctx = mp_shm_fifo_create(SHM_FIFO_NAME, shm_pool_size,
            SHM_FIFO_EXHAUST_CTRL, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx != NULL);

    /* Note that 'read_buf' is not freed as calls to 'shm_fifo_pull' always
     * fail
     */
    ck_assert(mp_shm_fifo_pull(NULL, (void**)&elem, &elem_size, -1,
            MP_LOG_GET()) == MP_ERROR);

    ck_assert(mp_shm_fifo_pull(mp_shm_fifo_ctx, (void**)NULL, &elem_size, -1,
            MP_LOG_GET()) == MP_ERROR);

    ck_assert(mp_shm_fifo_pull(mp_shm_fifo_ctx, (void**)&elem, NULL, -1,
            MP_LOG_GET()) == MP_ERROR);

    /* Unblock FIFO and release */
    mp_shm_fifo_set_blocking_mode(mp_shm_fifo_ctx, 0, MP_LOG_GET());
    mp_shm_fifo_release(&mp_shm_fifo_ctx, MP_LOG_GET());
    mp_log_close(&mp_log_ctx);
}
END_TEST


START_TEST(test_mp_shm_fifo_empty)
{
    mp_log_ctx_t *mp_log_ctx;
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx;
    const size_t max_data_size = SHM_FIFO_MESSAGE_MAX_LEN,
            shm_pool_size = sizeof(ssize_t) + max_data_size;
    MP_LOG_INIT(NULL);
    uint8_t *elem = NULL;
    size_t elem_size = 0;

    /* Open logger (just log to stdout) */
    mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_SET(mp_log_ctx);

    /* Create SHM-FIFO */
    mp_shm_fifo_ctx = mp_shm_fifo_create(SHM_FIFO_NAME, shm_pool_size,
            SHM_FIFO_EXHAUST_CTRL, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx != NULL);

    /* Push, empty and try pulling a message (should time-out) */

    ck_assert(mp_shm_fifo_push(mp_shm_fifo_ctx, "abcdefghijklmno\0", 16,
            MP_LOG_GET()) == MP_SUCCESS);

    mp_shm_fifo_empty(mp_shm_fifo_ctx, MP_LOG_GET());

    ck_assert(mp_shm_fifo_pull(mp_shm_fifo_ctx, (void**)&elem, &elem_size,
            1*1000 /*1 millisecond*/, MP_LOG_GET()) == MP_ETIMEDOUT &&
            elem == NULL);

    /* Test bad arguments */
    mp_shm_fifo_empty(NULL, MP_LOG_GET());

    /* Unblock FIFO and release */
    mp_shm_fifo_set_blocking_mode(mp_shm_fifo_ctx, 0, MP_LOG_GET());
    mp_shm_fifo_release(&mp_shm_fifo_ctx, MP_LOG_GET());
    mp_log_close(&mp_log_ctx);
}
END_TEST

START_TEST(test_mp_shm_fifo_push_bad_args)
{
    mp_log_ctx_t *mp_log_ctx;
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx;
    const size_t max_data_size = SHM_FIFO_MESSAGE_MAX_LEN,
            shm_pool_size = sizeof(ssize_t) + max_data_size;
    MP_LOG_INIT(NULL);

    /* Open logger (just log to stdout) */
    mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_SET(mp_log_ctx);

    /* Create SHM-FIFO */
    mp_shm_fifo_ctx = mp_shm_fifo_create(SHM_FIFO_NAME, shm_pool_size,
            SHM_FIFO_EXHAUST_CTRL, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx != NULL);

    ck_assert(mp_shm_fifo_push(NULL, "abcdefghijklmno\0", 16, MP_LOG_GET())
            == MP_ERROR);

    ck_assert(mp_shm_fifo_push(mp_shm_fifo_ctx, NULL, 16, MP_LOG_GET())
            == MP_ERROR);

    ck_assert(mp_shm_fifo_push(mp_shm_fifo_ctx, "abcdefghijklmno\0", 0,
            MP_LOG_GET()) == MP_ERROR);

    /* Unblock FIFO and release */
    mp_shm_fifo_set_blocking_mode(mp_shm_fifo_ctx, 0, MP_LOG_GET());
    mp_shm_fifo_release(&mp_shm_fifo_ctx, MP_LOG_GET());
    mp_log_close(&mp_log_ctx);
}
END_TEST

START_TEST(test_mp_shm_fifo_other_bad_args)
{
    mp_log_ctx_t *mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_INIT(mp_log_ctx);

    /* Test bad arguments at 'fifo_set_blocking_mode()' */
    mp_shm_fifo_set_blocking_mode(NULL, 0, MP_LOG_GET());

    /* Test bad arguments at 'shm_fifo_open()' */
    ck_assert(mp_shm_fifo_open(NULL, MP_LOG_GET()) == NULL);

    /* Test bad arguments at 'shm_fifo_close()' */
    mp_shm_fifo_close(NULL, MP_LOG_GET());
    mp_shm_fifo_ctx_t *fifo_ctx_bad = NULL;
    mp_shm_fifo_close(&fifo_ctx_bad, MP_LOG_GET());

    mp_log_close(&mp_log_ctx);
}
END_TEST

static const char *messages_list_1[]=
{
        "Hello, world!.\0",
        "How are you?.\0",
        "abcdefghijklmno\0",
        "123456789\0",
        "__ABCD__1234_\0",
        "_            _\0",
        "_/)=:;.\"·#{+]\0",
        "{\"key\":\"val\"}\0",
        "Goodbye.\0",
        "_/)=:;.\"·#{+]\0",
        "{\"key\":\"varte\0",
        "_/)=:;.\\0",
        "{\"key\":####al\"}\0",
        " \0",
        "\0",
        "     \0",
        "",
        "################\0", // Test max length (SHM_FIFO_MESSAGE_MAX_LEN)
        "_            _\0",
        "_/)=:;.\"·#{+]\0",
        "{\"key\":\"val\"}\0",
        "Goodbye.\0",
        "_/)=:;.\"·#{+]\0",
        "{\"key\":\"varte\0",
        "_/)=:;.\\0",
        "{\"key\":####al\"}\0",
        " \0",
        "\0",
        "     \0",
        "Goodbye.\0",
        NULL
};

static const char *messages_list_2[]=
{
        "IIIIIIIIIIIIIIIII\0", // Exceed maximum lenght -fail to push-
        NULL
};

static void happypath_consumer_task(mp_log_ctx_t *const mp_log_ctx)
{
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx;
    uint8_t *elem = NULL;
    size_t elem_size = 0;
    int message_cnt = 0;
    MP_LOG_INIT(mp_log_ctx);

    mp_shm_fifo_ctx = mp_shm_fifo_open(SHM_FIFO_NAME, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx != NULL);

    while (messages_list_1[message_cnt] != NULL) {
        int ret_code;

        if (elem != NULL) {
            free(elem);
            elem = NULL;
        }

        ret_code = mp_shm_fifo_pull(mp_shm_fifo_ctx, (void**)&elem, &elem_size,
                -1, MP_LOG_GET());
        if (ret_code == MP_EAGAIN) {
            MP_LOGD("FIFO unlocked, exiting consumer task\n");
            break;
        } else if (ret_code == MP_ETIMEDOUT) {
            MP_LOGD("FIFO timed-out, exiting consumer task\n");
            break;
        }
        ck_assert(ret_code == MP_SUCCESS);
        ck_assert(elem != NULL);

        MP_LOGD("Consumer get %zu chars from FIFO: '%s'\n", elem_size, elem);
        ck_assert(!strcmp((const char*)elem, messages_list_1[message_cnt++]));
    }

    MP_LOGD("Exiting consumer task\n");
    if (elem != NULL)
        free(elem);
    mp_shm_fifo_close(&mp_shm_fifo_ctx, MP_LOG_GET());
}

START_TEST(test_mp_shm_fifo_happypath)
{
    int status;
    mp_log_ctx_t *mp_log_ctx;
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx;
    const size_t max_data_size = SHM_FIFO_MESSAGE_MAX_LEN,
            shm_pool_size = sizeof(ssize_t) + max_data_size;
    pid_t child_pid;
    MP_LOG_INIT(NULL);

    /* Open logger (just log to stdout) */
    mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_SET(mp_log_ctx);

    /* Make sure FIFO name does not already exist (delete if applicable) */
    if (access("/dev/shm" SHM_FIFO_NAME, F_OK) == 0)
        ck_assert(shm_unlink(SHM_FIFO_NAME) == 0);

    /* Create SHM-FIFO */
    mp_shm_fifo_ctx = mp_shm_fifo_create(SHM_FIFO_NAME, shm_pool_size,
            SHM_FIFO_EXHAUST_CTRL, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx != NULL);

    /* Fork off the parent process */
    ck_assert((child_pid = fork()) >= 0);
    if (child_pid == 0) {
        /* Child process code ... */
        happypath_consumer_task(MP_LOG_GET());
        return;
    }

    /* Parent process code ... */

    /* Push some messages.
     * Note that pool size is 'shm_pool_size' and messages should fit!
     */
    for (int i = 0; messages_list_1[i] != NULL; i++)
        ck_assert(mp_shm_fifo_push(mp_shm_fifo_ctx, messages_list_1[i],
                strlen(messages_list_1[i]) + 1, MP_LOG_GET()) == MP_SUCCESS);

    /* Wait for consumer process to terminate */
    MP_LOGD("Waiting for consumer process to terminate...\n");
    ck_assert(waitpid(child_pid, &status, WUNTRACED) != -1);
    ck_assert(status == 0);
    MP_LOGD("Consumer process exited (status = %d)\n", WEXITSTATUS(status));

    /* Unblock FIFO and release */
    mp_shm_fifo_set_blocking_mode(mp_shm_fifo_ctx, 0, MP_LOG_GET());
    mp_shm_fifo_release(&mp_shm_fifo_ctx, MP_LOG_GET());
    mp_log_close(&mp_log_ctx);
}
END_TEST

START_TEST(test_mp_shm_fifo_push_invalid_message)
{
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx;
    const size_t max_data_size = SHM_FIFO_MESSAGE_MAX_LEN,
            shm_pool_size = sizeof(ssize_t) + max_data_size;

    /* Open logger (just log to stdout) */
    mp_log_ctx_t *mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_INIT(mp_log_ctx);

    /* Create SHM-FIFO */
    mp_shm_fifo_ctx = mp_shm_fifo_create(SHM_FIFO_NAME, shm_pool_size, 0,
            MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx != NULL);

    MP_LOGD("Push some **invalid** messages (should fail)\n");

    for (int i = 0; messages_list_2[i] != NULL; i++)
        ck_assert(mp_shm_fifo_push(mp_shm_fifo_ctx, messages_list_2[i],
                strlen(messages_list_2[i]) + 1, MP_LOG_GET()) == MP_ERROR);

    ck_assert(mp_shm_fifo_push(mp_shm_fifo_ctx, messages_list_2[0], 0,
            MP_LOG_GET()) == MP_ERROR);

    mp_shm_fifo_release(&mp_shm_fifo_ctx, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx == NULL);

    mp_log_close(&mp_log_ctx);
}
END_TEST

START_TEST(test_mp_shm_fifo_push_overflow)
{
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx;
    const size_t max_data_size = SHM_FIFO_MESSAGE_MAX_LEN,
            shm_pool_size = sizeof(ssize_t) + max_data_size;

    /* Open logger (just log to stdout) */
    mp_log_ctx_t *mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_INIT(mp_log_ctx);

    /* Create SHM-FIFO */
    mp_shm_fifo_ctx = mp_shm_fifo_create(SHM_FIFO_NAME, shm_pool_size, 0,
            MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx != NULL);

    /* Should unlock to return from function in overflow */
    mp_shm_fifo_set_blocking_mode(mp_shm_fifo_ctx, 0, MP_LOG_GET());

    MP_LOGD("Push some messages to force overflow\n");

    ck_assert(mp_shm_fifo_push(mp_shm_fifo_ctx, "abcdefghijklmno\0", 16,
            MP_LOG_GET()) == MP_SUCCESS);
    ck_assert(mp_shm_fifo_push(mp_shm_fifo_ctx, "fail\0", 5, MP_LOG_GET())
            == MP_ENOMEM);

    mp_shm_fifo_release(&mp_shm_fifo_ctx, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx == NULL);

    mp_log_close(&mp_log_ctx);
}
END_TEST

START_TEST(test_mp_shm_fifo_pull_underrun)
{
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx;
    const size_t max_data_size = SHM_FIFO_MESSAGE_MAX_LEN,
            shm_pool_size = sizeof(ssize_t) + max_data_size;

    /* Open logger (just log to stdout) */
    mp_log_ctx_t *mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_INIT(mp_log_ctx);

    /* Create SHM-FIFO */
    mp_shm_fifo_ctx = mp_shm_fifo_create(SHM_FIFO_NAME, shm_pool_size, 0,
            MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx != NULL);

    /* Should unlock to return from function in overflow */
    mp_shm_fifo_set_blocking_mode(mp_shm_fifo_ctx, 0, MP_LOG_GET());

    MP_LOGD("Pull empty FIFO to force underrun\n");

    mp_shm_fifo_empty(mp_shm_fifo_ctx, MP_LOG_GET());

    uint8_t *elem = NULL;
    size_t elem_size = 0;
    ck_assert(mp_shm_fifo_pull(mp_shm_fifo_ctx, (void**)&elem, &elem_size, -1,
            MP_LOG_GET()) == MP_EAGAIN);
    if (elem != NULL)
        free(elem);

    mp_shm_fifo_release(&mp_shm_fifo_ctx, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx == NULL);

    mp_log_close(&mp_log_ctx);
}
END_TEST

START_TEST(test_mp_shm_fifo_pull_timeout)
{
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx;
    const size_t max_data_size = SHM_FIFO_MESSAGE_MAX_LEN,
            shm_pool_size = sizeof(ssize_t) + max_data_size;
    uint8_t *elem = NULL;
    size_t elem_size = 0;
    uint64_t t0 = 0, tcurr = 0;

    /* Open logger (just log to stdout) */
    mp_log_ctx_t *mp_log_ctx = mp_log_open(NULL, NULL);
    ck_assert(mp_log_ctx != NULL);
    MP_LOG_INIT(mp_log_ctx);

    /* Get initial time */
    t0 = mp_gettime_monotcoarse_msecs(MP_LOG_GET());
    ck_assert(t0 > 0);

    /* Create SHM-FIFO */
    mp_shm_fifo_ctx = mp_shm_fifo_create(SHM_FIFO_NAME, shm_pool_size, 0,
            MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx != NULL);

    mp_shm_fifo_set_blocking_mode(mp_shm_fifo_ctx, SHM_FIFO_O_NONBLOCK,
            MP_LOG_GET());
    mp_shm_fifo_empty(mp_shm_fifo_ctx, MP_LOG_GET());

    /* Push and pull a message; processing time should be ~ 0 */
    ck_assert(mp_shm_fifo_push(mp_shm_fifo_ctx, "abcdefghijklmno\0", 16,
            MP_LOG_GET()) == MP_SUCCESS);
    ck_assert(mp_shm_fifo_pull(mp_shm_fifo_ctx, (void**)&elem, &elem_size,
            1000*1000 /*1 second*/, MP_LOG_GET()) == MP_SUCCESS &&
            elem != NULL && elem_size == 16);
    tcurr = mp_gettime_monotcoarse_msecs(MP_LOG_GET());
    ck_assert(tcurr >= t0);
    free(elem);
    elem = NULL;
    elem_size = 0;

    /* Pull on empty FIFO; processing time should be ~ time-out */
    ck_assert(mp_shm_fifo_pull(mp_shm_fifo_ctx, (void**)&elem, &elem_size,
            1000*1000 /*1 second*/, MP_LOG_GET()) == MP_ETIMEDOUT /*&&
            elem == NULL*/);
    tcurr = mp_gettime_monotcoarse_msecs(MP_LOG_GET());
    ck_assert((tcurr >= t0) && (tcurr - t0 >= 1000));

    mp_shm_fifo_release(&mp_shm_fifo_ctx, MP_LOG_GET());
    ck_assert(mp_shm_fifo_ctx == NULL);

    mp_log_close(&mp_log_ctx);
}
END_TEST

Suite *mp_shm_fifo_suite()
{
    Suite *s;
    TCase *tc;

    s = suite_create("mp_shm_fifo");

    /* Header file public prototypes test case */
    tc = tcase_create("mp_shm_fifo_testcase");

    tcase_add_test(tc, test_mp_shm_fifo_create);
    tcase_add_test(tc, test_mp_shm_fifo_release);
    tcase_add_test(tc, test_mp_shm_fifo_get_buffer_level);
    tcase_add_test(tc, test_mp_shm_fifo_empty);
    tcase_add_test(tc, test_mp_shm_fifo_pull_bad_args);
    tcase_add_test(tc, test_mp_shm_fifo_push_bad_args);
    tcase_add_test(tc, test_mp_shm_fifo_other_bad_args);
    tcase_add_test(tc, test_mp_shm_fifo_happypath);
    tcase_add_test(tc, test_mp_shm_fifo_push_invalid_message);
    tcase_add_test(tc, test_mp_shm_fifo_push_overflow);
    tcase_add_test(tc, test_mp_shm_fifo_pull_underrun);
    tcase_add_test(tc, test_mp_shm_fifo_pull_timeout);
    suite_add_tcase(s, tc);

    return s;
}
