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
 * @file mp_shm_fifo.c
 * @author Rafael Antoniello
 */

#include "mp_shm_fifo.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>

#include "mp_log.h"
#include "mp_check.h"
#include "mp_statcodes.h"

// **** Definitions ****


/// FIFO element context structure.
typedef struct shm_fifo_elem_ctx_s
{
    /// Element size in bytes.
    ssize_t size;

    /// Point to this memory pool of "fifo_elem_ctx_s::size" bytes
    /// (note that shm_fifo_elem_ctx_s::size <= pool maximum size)
    uint8_t shm_elem_pool[]; // flexible array member must be last
} shm_fifo_elem_ctx_t;

/// SHM-FIFO module context structure.
typedef struct mp_shm_fifo_ctx_s
{
    /// FIFO name
    char shm_fifo_name[NAME_MAX];

    /// Available flags:
    /// - SHM_FIFO_O_NONBLOCK
    volatile uint32_t flags;

    /// Exit flag: if set to non-zero value, FIFO module should finish/unblock transactions as fast as possible
    volatile int flag_exit;

    /// Memory pool used to enqueue FIFO elements
    size_t shm_pool_size;

    /// API mutex.
    pthread_mutex_t api_mutex;
    int flag_api_mutex_initialized;

    /// Signals each time a new chunk enters the FIFO buffer.
    pthread_cond_t buf_put_signal;
    int flag_buf_put_signal_initialized;

    /// Signals each time a new chunk is consumed from the FIFO buffer.
    pthread_cond_t buf_get_signal;
    int flag_buf_get_signal_initialized;

    /// Number of slots currently used.
    volatile ssize_t slots_used_cnt;

    /// Addition of all the size values of the elements currently enqueued in the FIFO pool. Namely, is the overall
    /// FIFO buffer level (in bytes).
    volatile ssize_t buf_level;

    /// This index is the buffer byte position corresponding to the next slot available for input.
    /// Each time an element is pushed into the FIFO, it is copied starting at this byte position; then the index is
    /// incremented to point to the next byte position available in the FIFO buffer.
    volatile int input_byte_idx;

    /// This index is the buffer byte position corresponding to the start of the next element available for output.
    /// Each time an element is pulled out of the FIFO, we increment this index to point to the first byte of the next
    /// element to be pulled.
    volatile int output_byte_idx;

    /// This is a circular buffer of elements/chunks of data.
    shm_fifo_elem_ctx_t buf[]; // flexible array member must be last
} mp_shm_fifo_ctx_t;


// Define only for debugging/testing purposes
//#define SHM_FIFO_DBG_PRINT_POOL

// **** Prototypes ****


// **** Implementations ****


mp_shm_fifo_ctx_t* mp_shm_fifo_create(const char *fifo_file_name, const size_t shm_pool_size, uint32_t flags,
        mp_log_ctx_t *mp_log_ctx)
{
    pthread_mutexattr_t mutexattr;
    pthread_condattr_t condattr1, condattr2;
    size_t mp_shm_fifo_ctx_size;
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx = NULL;
    int fd = -1, end_code = MP_ERROR;
    MP_LOG_INIT(mp_log_ctx);

    // Check arguments
    MP_CHECK(fifo_file_name != NULL && shm_pool_size > 0, return NULL);
    if(strlen(fifo_file_name) > NAME_MAX)
    {
        MP_LOGE("Maximum FIFO file-name length exceeded; name has to be maximum %d characters\n", NAME_MAX);
        return NULL;
    }
    // Parameter 'flags' may take any value
    // Parameter 'mp_log_ctx' is allowed to be NULL

    // Create the shared memory segment
    // Note on 'O_CREAT|O_EXCL' usage: if both flags specified when opening a shared memory object with the given
    // name that already exists, return error 'EEXIST'. The check for the existence of the object, and its creation if
    // it does not exist, are performed **atomically**.
    errno = 0;
    fd = shm_open(fifo_file_name, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
    if(fd < 0 && errno == EEXIST)
    {
        MP_LOGE("Trying to create an SHM-FIFO that already exists (errno: %d)\n", errno);
        goto end;
    }
    MP_CHECK(fd >= 0, MP_LOGE("Could not open SHM-FIFO (errno: %d)\n", errno); goto end);

    // Compute the size of the FIFO context structure allocation
    // Note that 'sizeof(mp_shm_fifo_ctx_t)' is intended to reserve the FIFO's context space aligned at the beginning of
    // the shared-memory segment
    mp_shm_fifo_ctx_size = sizeof(mp_shm_fifo_ctx_t) + shm_pool_size;

    // Configure size of the shared memory segment
    // Note: we use the 'ftruncate()' on a brand new FIFO, thus, the new FIFO memory space reads as null bytes ('\0').
    // Also note that the file offset is not changed (points to the beginning of the shared-memory segment).
    MP_CHECK(ftruncate(fd, mp_shm_fifo_ctx_size) == 0, goto end);

    // Map the shared memory segment in the address space of the calling process
    if((mp_shm_fifo_ctx = (mp_shm_fifo_ctx_t*)mmap(NULL, mp_shm_fifo_ctx_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
        mp_shm_fifo_ctx = NULL;
    MP_CHECK(mp_shm_fifo_ctx != NULL, goto end);

    // **** Initialize rest of FIFO context structure members for the first time ****

    //memset(mp_shm_fifo_ctx, 0, mp_shm_fifo_ctx_size);

    snprintf(mp_shm_fifo_ctx->shm_fifo_name, NAME_MAX - 1, "%s", fifo_file_name);

    mp_shm_fifo_ctx->flags = flags;
    MP_LOGD("Exhaustive circular buffer checking mode %s on fifo '%s'\n", flags & SHM_FIFO_EXHAUST_CTRL ? "on!" : "off",
            fifo_file_name);

    mp_shm_fifo_ctx->flag_exit = 0; // Already performed at memset() above

    mp_shm_fifo_ctx->shm_pool_size = shm_pool_size;

    pthread_mutexattr_init(&mutexattr);
    MP_CHECK(pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED) == 0, goto end);
    MP_CHECK(pthread_mutex_init(&mp_shm_fifo_ctx->api_mutex, &mutexattr) == 0, goto end);
    mp_shm_fifo_ctx->flag_api_mutex_initialized = 1;

    pthread_condattr_init(&condattr1);
    MP_CHECK(pthread_condattr_setclock(&condattr1, CLOCK_MONOTONIC) == 0, goto end);
    MP_CHECK(pthread_condattr_setpshared(&condattr1, PTHREAD_PROCESS_SHARED) == 0, goto end);
    MP_CHECK(pthread_cond_init(&mp_shm_fifo_ctx->buf_put_signal, &condattr1) == 0, goto end);
    mp_shm_fifo_ctx->flag_buf_put_signal_initialized = 1;

    pthread_condattr_init(&condattr2);
    MP_CHECK(pthread_condattr_setclock(&condattr2, CLOCK_MONOTONIC) == 0, goto end);
    MP_CHECK(pthread_condattr_setpshared(&condattr2, PTHREAD_PROCESS_SHARED) == 0, goto end);
    MP_CHECK(pthread_cond_init(&mp_shm_fifo_ctx->buf_get_signal, &condattr2) == 0, goto end);
    mp_shm_fifo_ctx->flag_buf_get_signal_initialized = 1;

    mp_shm_fifo_ctx->slots_used_cnt = 0; // Already performed at memset() above

    mp_shm_fifo_ctx->buf_level = 0; // Already performed at memset() above

    mp_shm_fifo_ctx->input_byte_idx = 0; // Already performed at memset() above

    mp_shm_fifo_ctx->output_byte_idx = 0; // Already performed at memset() above

    MP_LOGD("FIFO successfully created with pool size of %u bytes.\n", shm_pool_size);
    end_code = MP_SUCCESS;
end:
    // File descriptor is not necessary to be kept
    if(fd >= 0)
        MP_ASSERT(close(fd) == 0);

    // Release FIFO in case error occurred
    if(end_code != MP_SUCCESS)
        mp_shm_fifo_release(&mp_shm_fifo_ctx, MP_LOG_GET());

    return mp_shm_fifo_ctx;
}


void mp_shm_fifo_release(mp_shm_fifo_ctx_t **ref_mp_shm_fifo_ctx, mp_log_ctx_t *mp_log_ctx)
{
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx;
    size_t mp_shm_fifo_ctx_size;
    register int flag_api_mutex_initialized, flag_buf_put_signal_initialized, flag_buf_get_signal_initialized;
    char shm_fifo_name[NAME_MAX] = {0};
    MP_LOG_INIT(mp_log_ctx);

    // Check arguments
    if(ref_mp_shm_fifo_ctx == NULL || (mp_shm_fifo_ctx = *ref_mp_shm_fifo_ctx) == NULL)
        return;
    // Parameter 'mp_log_ctx' is allowed to be NULL

    // **** De-initialize FIFO context structure members ****

    flag_api_mutex_initialized = mp_shm_fifo_ctx->flag_api_mutex_initialized;
    flag_buf_put_signal_initialized = mp_shm_fifo_ctx->flag_buf_put_signal_initialized;
    flag_buf_get_signal_initialized = mp_shm_fifo_ctx->flag_buf_get_signal_initialized;
    strcpy(shm_fifo_name, mp_shm_fifo_ctx->shm_fifo_name);

    // Set exit flag and send signals to eventually unlock the API MUTEX
    mp_shm_fifo_ctx->flag_exit= 1;
    if(flag_api_mutex_initialized != 0)
    {
        pthread_mutex_lock(&mp_shm_fifo_ctx->api_mutex);
        if(flag_buf_put_signal_initialized != 0)
            pthread_cond_broadcast(&mp_shm_fifo_ctx->buf_put_signal);
        if(flag_buf_get_signal_initialized != 0)
            pthread_cond_broadcast(&mp_shm_fifo_ctx->buf_get_signal);
        pthread_mutex_unlock(&mp_shm_fifo_ctx->api_mutex);
    }

    // Release API MUTEX
    if(flag_api_mutex_initialized != 0)
    {
        MP_ASSERT(pthread_mutex_destroy(&mp_shm_fifo_ctx->api_mutex) == 0);
        mp_shm_fifo_ctx->flag_api_mutex_initialized = 0;
    }

    // Release conditionals
    if(flag_buf_put_signal_initialized != 0)
    {
        MP_ASSERT(pthread_cond_destroy(&mp_shm_fifo_ctx->buf_put_signal) == 0);
        mp_shm_fifo_ctx->flag_buf_put_signal_initialized = 0;
    }
    if(flag_buf_get_signal_initialized!= 0)
    {
        MP_ASSERT(pthread_cond_destroy(&mp_shm_fifo_ctx->buf_get_signal) == 0);
        mp_shm_fifo_ctx->flag_buf_get_signal_initialized = 0;
    }

    // Remove the mapped shared memory segment from the address space
    mp_shm_fifo_ctx_size = sizeof(mp_shm_fifo_ctx_t) + mp_shm_fifo_ctx->shm_pool_size;
    MP_ASSERT(munmap(mp_shm_fifo_ctx, mp_shm_fifo_ctx_size)== 0);

    // Unlink FIFO
    MP_ASSERT(shm_unlink(shm_fifo_name) == 0);

    MP_LOGD("shm_fifo_release() '%s' completed OK!\n", shm_fifo_name);

    *ref_mp_shm_fifo_ctx = NULL;
}


mp_shm_fifo_ctx_t* mp_shm_fifo_open(const char *fifo_file_name, mp_log_ctx_t *mp_log_ctx)
{
    size_t mp_shm_fifo_ctx_size;
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx = NULL;
    int fd = -1, end_code = MP_ERROR;
    MP_LOG_INIT(mp_log_ctx);

    // Check arguments
    MP_CHECK(fifo_file_name != NULL, return NULL);
    // Parameter 'mp_log_ctx' is allowed to be NULL

    // Open already existent shared memory segment
    fd = shm_open(fifo_file_name, O_RDWR, S_IRUSR | S_IWUSR);
    MP_CHECK(fd >= 0, MP_LOGE("Could not open SHM-FIFO '%s' (errno: %d)\n", fifo_file_name, errno); goto end);

    // Partially map the shared memory segment in the address space of the calling process
    if((mp_shm_fifo_ctx = (mp_shm_fifo_ctx_t*)mmap(NULL, sizeof(mp_shm_fifo_ctx_t), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
        mp_shm_fifo_ctx = NULL;
    MP_CHECK(mp_shm_fifo_ctx != NULL, goto end);

    // Re-map shared segment with actual size
    mp_shm_fifo_ctx_size = sizeof(mp_shm_fifo_ctx_t) + mp_shm_fifo_ctx->shm_pool_size;
    MP_CHECK(munmap(mp_shm_fifo_ctx, sizeof(mp_shm_fifo_ctx_t)) == 0, goto end);
    if((mp_shm_fifo_ctx = (mp_shm_fifo_ctx_t*)mmap(NULL, mp_shm_fifo_ctx_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
        mp_shm_fifo_ctx = NULL;
    MP_CHECK(mp_shm_fifo_ctx != NULL, goto end);

    end_code = MP_SUCCESS;
end:
    // File descriptor is not necessary to be kept
    if(fd >= 0)
        MP_ASSERT(close(fd) == 0);

    // Release FIFO in case error occurred
    if(end_code != MP_SUCCESS)
        mp_shm_fifo_close(&mp_shm_fifo_ctx, MP_LOG_GET());

    return mp_shm_fifo_ctx;
}


void mp_shm_fifo_close(mp_shm_fifo_ctx_t **ref_mp_shm_fifo_ctx, mp_log_ctx_t *mp_log_ctx)
{
    mp_shm_fifo_ctx_t *mp_shm_fifo_ctx;
    size_t mp_shm_fifo_ctx_size;
    MP_LOG_INIT(mp_log_ctx);

    // Check arguments
    if(ref_mp_shm_fifo_ctx == NULL || (mp_shm_fifo_ctx = *ref_mp_shm_fifo_ctx) == NULL)
        return;
    // Parameter 'mp_log_ctx' is allowed to be NULL

    // Remove the mapped shared memory segment from the address space
    mp_shm_fifo_ctx_size = sizeof(mp_shm_fifo_ctx_t) + mp_shm_fifo_ctx->shm_pool_size;
    MP_ASSERT(munmap(mp_shm_fifo_ctx, mp_shm_fifo_ctx_size) == 0);

    *ref_mp_shm_fifo_ctx = NULL;
}


void mp_shm_fifo_set_blocking_mode(mp_shm_fifo_ctx_t *mp_shm_fifo_ctx, int do_block, mp_log_ctx_t *mp_log_ctx)
{
    MP_LOG_INIT(mp_log_ctx);

    MP_CHECK(mp_shm_fifo_ctx != NULL, return);
    // Parameter 'mp_log_ctx' is allowed to be NULL

    pthread_mutex_lock(&mp_shm_fifo_ctx->api_mutex);

    // Set the 'non-blocking' bit-flag
    if(do_block != 0) {
        mp_shm_fifo_ctx->flags &= ~((uint32_t)SHM_FIFO_O_NONBLOCK);
    } else {
        mp_shm_fifo_ctx->flags |= (uint32_t)SHM_FIFO_O_NONBLOCK;
    }

    // Announce to unblock conditional waits
    pthread_cond_broadcast(&mp_shm_fifo_ctx->buf_put_signal);
    pthread_cond_broadcast(&mp_shm_fifo_ctx->buf_get_signal);

    pthread_mutex_unlock(&mp_shm_fifo_ctx->api_mutex);
    return;
}


int mp_shm_fifo_push(mp_shm_fifo_ctx_t *mp_shm_fifo_ctx, const void *elem, size_t elem_size, mp_log_ctx_t *mp_log_ctx)
{
    size_t shm_pool_size, shm_elem_size = sizeof(shm_fifo_elem_ctx_t) + elem_size;
    shm_fifo_elem_ctx_t *shm_fifo_elem_ctx_aux = NULL;
    int input_byte_idx, end_code = MP_ERROR;
    MP_LOG_INIT(mp_log_ctx);

    // Check arguments
    MP_CHECK(mp_shm_fifo_ctx != NULL && elem != NULL && elem_size > 0, return MP_ERROR);
    if(shm_elem_size > (shm_pool_size = mp_shm_fifo_ctx->shm_pool_size))
    {
        MP_LOGE("Input element size can not exceed FIFO overall pool size (%d bytes)\n", shm_pool_size);
        return MP_ERROR;
    }
    // Parameter 'mp_log_ctx' is allowed to be NULL

    MP_LOGD("fifo_push; new element context size: %zu (raw-context) + %zu (input data) = %zu (total size); '%s'\n",
            sizeof(shm_fifo_elem_ctx_t), elem_size, shm_elem_size, elem); // comment-me
    MP_LOGD("FIFO buffer level previous to pushing new data: %zu\n", mp_shm_fifo_ctx->buf_level); // comment-me

    // Lock API MUTEX
    pthread_mutex_lock(&mp_shm_fifo_ctx->api_mutex);

    // In the case of blocking FIFO, if buffer is full we block until an element is consumed and a new free slot is
    // available.
    // In the case of a non-blocking FIFO, if buffer is full we exit returning 'ENOMEM' status.
    while((mp_shm_fifo_ctx->buf_level + shm_elem_size > shm_pool_size) && !(mp_shm_fifo_ctx->flags & SHM_FIFO_O_NONBLOCK) &&
            (mp_shm_fifo_ctx->flag_exit == 0))
    {
        MP_LOGW("SHM-FIFO buffer '%s' overflow!\n", mp_shm_fifo_ctx->shm_fifo_name);
        pthread_cond_broadcast(&mp_shm_fifo_ctx->buf_put_signal);
        pthread_cond_wait(&mp_shm_fifo_ctx->buf_get_signal, &mp_shm_fifo_ctx->api_mutex);
    }
    if((mp_shm_fifo_ctx->buf_level + shm_elem_size > shm_pool_size) && (mp_shm_fifo_ctx->flags & SHM_FIFO_O_NONBLOCK))
    {
        MP_LOGW("SHM-FIFO buffer '%s' overflow!\n", mp_shm_fifo_ctx->shm_fifo_name);
        end_code = MP_ENOMEM;
        goto end;
    }

    // Copy the new element
    if((input_byte_idx = mp_shm_fifo_ctx->input_byte_idx) + shm_elem_size > shm_pool_size)
    {
        // Treat fragmentation at the end of the memory pool
        int fragm0_size = shm_pool_size - input_byte_idx;
        int fragm1_size = shm_elem_size - fragm0_size;
        MP_CHECK(fragm0_size > 0 && fragm1_size > 0, goto end); // should never fail

        // To simplify copy operations (and avoid byte-by-byte coping) we compose a temporal element
        shm_fifo_elem_ctx_aux = (shm_fifo_elem_ctx_t*)malloc(shm_elem_size);
        MP_CHECK(shm_fifo_elem_ctx_aux != NULL, goto end);
        shm_fifo_elem_ctx_aux->size = elem_size;
        memcpy(&shm_fifo_elem_ctx_aux->shm_elem_pool[0], elem, elem_size);

        // Finally, copy the two fragments to the shared-memory segment
        memcpy((uint8_t*)mp_shm_fifo_ctx->buf + input_byte_idx, shm_fifo_elem_ctx_aux, fragm0_size);
        memcpy(mp_shm_fifo_ctx->buf, (uint8_t*)shm_fifo_elem_ctx_aux + fragm0_size, fragm1_size);
    }
    else // input_byte_idx + shm_elem_size <= shm_pool_size
    {
        // Get FIFO slot where to put new element
        shm_fifo_elem_ctx_t *shm_fifo_elem_ctx = (shm_fifo_elem_ctx_t*)(((uint8_t*)mp_shm_fifo_ctx->buf) +
                input_byte_idx);
        if(mp_shm_fifo_ctx->flags & SHM_FIFO_EXHAUST_CTRL)
        {
            MP_CHECK(shm_fifo_elem_ctx->size == 0, goto end);
        }
        memcpy(&shm_fifo_elem_ctx->shm_elem_pool[0], elem, elem_size);
        shm_fifo_elem_ctx->size = elem_size;
    }

    // Update circular buffer management variables
    mp_shm_fifo_ctx->slots_used_cnt += 1;
    mp_shm_fifo_ctx->buf_level += shm_elem_size;
    mp_shm_fifo_ctx->input_byte_idx = (input_byte_idx + shm_elem_size) % shm_pool_size;
    MP_LOGD("Pushed FIFO; buffer level: %zu\n", mp_shm_fifo_ctx->buf_level);

    // Signal we have a new element in the FIFO
    pthread_cond_broadcast(&mp_shm_fifo_ctx->buf_put_signal);

    end_code = MP_SUCCESS;
end:
    pthread_mutex_unlock(&mp_shm_fifo_ctx->api_mutex);
    if(shm_fifo_elem_ctx_aux != NULL)
        free(shm_fifo_elem_ctx_aux);
    return end_code;
}


int mp_shm_fifo_pull(mp_shm_fifo_ctx_t *mp_shm_fifo_ctx, void **ref_elem, size_t *ref_elem_size, int64_t tout_usecs,
        mp_log_ctx_t *mp_log_ctx)
{
    size_t shm_pool_size, elem_size;
    int output_byte_idx, ret_code, flag_is_fragmented, end_code = MP_ERROR;
    void *elem_cpy = NULL;
    struct timespec ts_tout= {0};
    MP_LOG_INIT(mp_log_ctx);

    // Check arguments
    MP_CHECK(mp_shm_fifo_ctx != NULL && ref_elem != NULL && ref_elem_size != NULL, return MP_ERROR);
    // Parameter 'tout_usecs' may take any value
    // Parameter 'mp_log_ctx' is allowed to be NULL

    // Reset arguments to be returned by value
    *ref_elem = NULL;
    *ref_elem_size = 0;

    // Lock API MUTEX
    pthread_mutex_lock(&mp_shm_fifo_ctx->api_mutex);

    // Get current time and compute time-out if applicable. Note that a negative time-out mens 'wait indefinitely'.
    if(tout_usecs >= 0)
    {
        struct timespec ts_curr;
        register int64_t curr_nsec;

        // Get current time
        MP_CHECK(clock_gettime(CLOCK_MONOTONIC, &ts_curr) == 0, goto end);
        curr_nsec = (int64_t)ts_curr.tv_sec * 1000000000 + (int64_t)ts_curr.tv_nsec;
        //MP_LOGD("curr_nsec: %"PRId64"\n", curr_nsec); //comment-me
        //MP_LOGD("secs: %"PRId64"\n", (int64_t)ts_curr.tv_sec); //comment-me
        //MP_LOGD("nsecs: %"PRId64"\n", (int64_t)ts_curr.tv_nsec); //comment-me

        // Compute time-out
        curr_nsec += (tout_usecs * 1000);
        ts_tout.tv_sec = curr_nsec / 1000000000;
        ts_tout.tv_nsec = curr_nsec % 1000000000;
        //MP_LOGD("tout_nsec: %"PRId64"\n", (int64_t)ts_tout.tv_sec*1000000000 + (int64_t)ts_tout.tv_nsec); //comment-me
        //MP_LOGD("secs: %"PRId64"\n", (int64_t)ts_tout.tv_sec); //comment-me
        //MP_LOGD("nsecs: %"PRId64"\n", (int64_t)ts_tout.tv_nsec); //comment-me
    }

    // In the case of blocking FIFO, if buffer is empty we block until a new element is inserted, or if it is the
    // case, until time-out occur.
    // In the case of a non-blocking FIFO, if buffer is empty we exit returning 'EAGAIN' status.
    while(mp_shm_fifo_ctx->buf_level <= 0 && !(mp_shm_fifo_ctx->flags & SHM_FIFO_O_NONBLOCK) && mp_shm_fifo_ctx->flag_exit == 0)
    {
        MP_LOGD("SHM-FIFO buffer '%s' underrun\n", mp_shm_fifo_ctx->shm_fifo_name);
        pthread_cond_broadcast(&mp_shm_fifo_ctx->buf_get_signal);

        // Manage pulling time-out
        if(tout_usecs >= 0)
        {
            ret_code = pthread_cond_timedwait(&mp_shm_fifo_ctx->buf_put_signal, &mp_shm_fifo_ctx->api_mutex, &ts_tout);
            if(ret_code == ETIMEDOUT)
            {
                MP_LOGW("FIFO pulling timed-out on empty buffer!\n");
                end_code = MP_ETIMEDOUT;
                goto end;
            }
        }
        else
        {
            MP_CHECK(pthread_cond_wait(&mp_shm_fifo_ctx->buf_put_signal, &mp_shm_fifo_ctx->api_mutex) == 0, goto end);
        }
    }
    if(mp_shm_fifo_ctx->buf_level <= 0 && (mp_shm_fifo_ctx->flags & SHM_FIFO_O_NONBLOCK))
    {
        MP_LOGD("SHM-FIFO buffer '%s' underrun\n", mp_shm_fifo_ctx->shm_fifo_name);
        end_code = MP_EAGAIN;
        goto end;
    }

    shm_pool_size = mp_shm_fifo_ctx->shm_pool_size;
    output_byte_idx = mp_shm_fifo_ctx->output_byte_idx;

    // Check if element context structure "header" itself is fragmented
    flag_is_fragmented = 0; // "false"
    if(output_byte_idx + sizeof(shm_fifo_elem_ctx_t) > shm_pool_size)
        flag_is_fragmented = 1;
    // Check if full element context structure is fragmented in any byte
    if(!flag_is_fragmented)
    {
        MP_CHECK((elem_size = ((shm_fifo_elem_ctx_t*)((uint8_t*)mp_shm_fifo_ctx->buf + output_byte_idx))->size) > 0,
                goto end);
        if(output_byte_idx + sizeof(shm_fifo_elem_ctx_t) + elem_size > shm_pool_size)
            flag_is_fragmented = 1;
    }

    if(flag_is_fragmented)
    {
        // Treat fragmentation at the end of the memory pool
        int buf_idx = output_byte_idx;
        uint8_t elem_ctx_hdr[sizeof(shm_fifo_elem_ctx_t)];
        shm_fifo_elem_ctx_t *shm_fifo_elem_ctx = (shm_fifo_elem_ctx_t*)elem_ctx_hdr;

        // Copy byte-by-byte until we got the element context "header"; get element size
        for(size_t i = 0; i < sizeof(elem_ctx_hdr); i++)
        {
            elem_ctx_hdr[i] = *((uint8_t*)mp_shm_fifo_ctx->buf + buf_idx);
            buf_idx = (buf_idx + 1) % shm_pool_size;
        }
        MP_CHECK((elem_size = shm_fifo_elem_ctx->size) > 0, goto end);

        // Allocate and copy the element
        elem_cpy = malloc(elem_size);
        MP_CHECK(elem_cpy != NULL, goto end);
        for(size_t i = 0; i < elem_size; i++)
        {
            ((uint8_t*)elem_cpy)[i] = *((uint8_t*)mp_shm_fifo_ctx->buf + buf_idx);
            buf_idx = (buf_idx + 1) % shm_pool_size;
        }

#ifdef SHM_FIFO_DBG_PRINT_POOL
        printf("PREV_FRAG_ERASING##idx=%d# ", output_byte_idx);
        for(int i = 0; i < shm_pool_size; i++)
            printf("%s%.*s%s ", output_byte_idx == i ? "{" : "'", 1, &((uint8_t*)mp_shm_fifo_ctx->buf)[i],
                    output_byte_idx == i ? "}" : "'");
        printf("\n"); fflush(stdout);
#endif

        if(mp_shm_fifo_ctx->flags & SHM_FIFO_EXHAUST_CTRL)
        {
            // Flush element from FIFO
            buf_idx = output_byte_idx; // rewind to the start of element context
            for(size_t i = 0; i < sizeof(shm_fifo_elem_ctx_t) + elem_size; i++)
            {
                ((char*)mp_shm_fifo_ctx->buf)[buf_idx] = '\0';
                buf_idx = (buf_idx + 1) % shm_pool_size;
            }
        }

#ifdef SHM_FIFO_DBG_PRINT_POOL
        printf("POST_FRAG_ERASING##idx=%d# ", buf_idx);
        for(int i= 0; i< shm_pool_size; i++)
            printf("%s%.*s%s ", buf_idx == i ? "{" : "'", 1, &((uint8_t*)mp_shm_fifo_ctx->buf)[i],
                    buf_idx == i ? "}" : "'");
        printf("\n"); fflush(stdout);
#endif
    }
    else // output_byte_idx + sizeof(fifo_elem_ctx_t) + elem_size <= shm_pool_size
    {
        shm_fifo_elem_ctx_t *shm_fifo_elem_ctx;

        // Get FIFO slot from where to read the element
        shm_fifo_elem_ctx = (shm_fifo_elem_ctx_t*)(((uint8_t*)mp_shm_fifo_ctx->buf) + output_byte_idx);
        MP_CHECK((elem_size = shm_fifo_elem_ctx->size) > 0, goto end);

        // Allocate and copy the element
        elem_cpy = malloc(elem_size);
        MP_CHECK(elem_cpy != NULL, goto end);
        memcpy(elem_cpy, &shm_fifo_elem_ctx->shm_elem_pool[0], elem_size);

#ifdef SHM_FIFO_DBG_PRINT_POOL
        printf("PREV_ERASING##idx=%d# ", output_byte_idx);
        for(int i= 0; i< shm_pool_size; i++)
            printf("%s%.*s%s ", output_byte_idx == i ? "{" : "'", 1, &((uint8_t*)mp_shm_fifo_ctx->buf)[i],
                    output_byte_idx == i ? "}" : "'");
        printf("\n"); fflush(stdout);
#endif

        if(mp_shm_fifo_ctx->flags & SHM_FIFO_EXHAUST_CTRL)
        {
            // Flush element from FIFO
            memset(shm_fifo_elem_ctx, 0 , sizeof(shm_fifo_elem_ctx_t) + elem_size);
        }

#ifdef SHM_FIFO_DBG_PRINT_POOL
        printf("POST_ERASING##idx=%d# ", output_byte_idx + (int)elem_size + (int)sizeof(shm_fifo_elem_ctx_t));
        for(int i= 0; i< shm_pool_size; i++)
            printf("%s%.*s%s ", output_byte_idx + elem_size + sizeof(shm_fifo_elem_ctx_t) == i ? "{" : "'",
                    1, &((uint8_t*)mp_shm_fifo_ctx->buf)[i],
                    output_byte_idx + elem_size + sizeof(shm_fifo_elem_ctx_t) == i ? "}" : "'");
        printf("\n"); fflush(stdout);
#endif
    }

    // Set the element references to return.
    *ref_elem = elem_cpy;
    elem_cpy = NULL; // Avoid double referencing
    *ref_elem_size = (size_t)elem_size;

    // Update circular buffer management variables.
    MP_LOGD("fifo_pull; pulled element context size: %zu (raw-context) + %zu (input data) = %zu (total size); '%s'\n",
            sizeof(shm_fifo_elem_ctx_t), elem_size, sizeof(shm_fifo_elem_ctx_t) + elem_size, *ref_elem); // comment-me
    mp_shm_fifo_ctx->slots_used_cnt -= 1;
    mp_shm_fifo_ctx->buf_level -= sizeof(shm_fifo_elem_ctx_t) + elem_size;
    mp_shm_fifo_ctx->output_byte_idx = (output_byte_idx + sizeof(shm_fifo_elem_ctx_t) + elem_size) % shm_pool_size;

    // Signal we have a new free slot in the FIFO
    pthread_cond_broadcast(&mp_shm_fifo_ctx->buf_get_signal);
    MP_LOGD("Pulled FIFO; new buffer level: %zu\n", mp_shm_fifo_ctx->buf_level);

    end_code = MP_SUCCESS;
end:
    pthread_mutex_unlock(&mp_shm_fifo_ctx->api_mutex);
    if(elem_cpy != NULL)
        free(elem_cpy);
    return end_code;
}


ssize_t mp_shm_fifo_get_buffer_level(mp_shm_fifo_ctx_t *mp_shm_fifo_ctx, mp_log_ctx_t *mp_log_ctx)
{
    ssize_t buf_level = -1; // invalid value to indicate error
    MP_LOG_INIT(mp_log_ctx);

    // Check arguments
    MP_CHECK(mp_shm_fifo_ctx != NULL, return -1);
    // Parameter 'mp_log_ctx' is allowed to be NULL

    pthread_mutex_lock(&mp_shm_fifo_ctx->api_mutex);
    buf_level = mp_shm_fifo_ctx->buf_level;
    pthread_mutex_unlock(&mp_shm_fifo_ctx->api_mutex);

    return buf_level;
}


void mp_shm_fifo_empty(mp_shm_fifo_ctx_t *mp_shm_fifo_ctx, mp_log_ctx_t *mp_log_ctx)
{
    MP_LOG_INIT(mp_log_ctx);

    // Check arguments
    MP_CHECK(mp_shm_fifo_ctx != NULL, return);
    // Parameter 'mp_log_ctx' is allowed to be NULL

    // Lock API mutex
    pthread_mutex_lock(&mp_shm_fifo_ctx->api_mutex);

    // Release all the elements available in FIFO buffer
    memset(&mp_shm_fifo_ctx->buf[0], 0, mp_shm_fifo_ctx->shm_pool_size);

    // Reset FIFO level and indexes
    mp_shm_fifo_ctx->slots_used_cnt= 0;
    mp_shm_fifo_ctx->buf_level= 0;
    mp_shm_fifo_ctx->input_byte_idx= 0;
    mp_shm_fifo_ctx->output_byte_idx= 0;

    // Signal we have a new free slot in the FIFO
    pthread_cond_broadcast(&mp_shm_fifo_ctx->buf_get_signal);
    MP_LOGD("FIFO emptied!\n");

    pthread_mutex_unlock(&mp_shm_fifo_ctx->api_mutex);
}

