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
 * @file mp_shm_fifo.h
 * @author Rafael Antoniello
 */

#ifndef MPUTILS_MP_SHM_FIFO_H_
#define MPUTILS_MP_SHM_FIFO_H_

#include <sys/types.h>
#include <inttypes.h>

/* **** Definitions **** */

/* Forward declarations */
typedef struct mp_log_ctx_s mp_log_ctx_t;
typedef struct mp_shm_fifo_ctx_s mp_shm_fifo_ctx_t;

/** Flag indicating FIFO is non-bocking (FIFO is blocking by default) */
#define SHM_FIFO_O_NONBLOCK     (1 << 0)
#define SHM_FIFO_EXHAUST_CTRL   (1 << 1) //FIXME!!

/* **** Prototypes **** */

mp_shm_fifo_ctx_t* mp_shm_fifo_create(const char *fifo_file_name,
        const size_t shm_pool_size, uint32_t flags, mp_log_ctx_t *mp_log_ctx);

void mp_shm_fifo_release(mp_shm_fifo_ctx_t **ref_mp_shm_fifo_ctx,
        mp_log_ctx_t *mp_log_ctx);

mp_shm_fifo_ctx_t* mp_shm_fifo_open(const char *fifo_file_name,
        mp_log_ctx_t *mp_log_ctx);

void mp_shm_fifo_close(mp_shm_fifo_ctx_t **ref_mp_shm_fifo_ctx,
        mp_log_ctx_t *mp_log_ctx);

void mp_shm_fifo_set_blocking_mode(mp_shm_fifo_ctx_t *mp_shm_fifo_ctx,
        int do_block, mp_log_ctx_t *mp_log_ctx);

int mp_shm_fifo_push(mp_shm_fifo_ctx_t *mp_shm_fifo_ctx, const void *elem,
        size_t elem_size, mp_log_ctx_t *mp_log_ctx);

int mp_shm_fifo_pull(mp_shm_fifo_ctx_t *mp_shm_fifo_ctx, void **ref_elem,
        size_t *ref_elem_size, int64_t tout_usecs, mp_log_ctx_t *mp_log_ctx);

ssize_t mp_shm_fifo_get_buffer_level(mp_shm_fifo_ctx_t *mp_shm_fifo_ctx,
        mp_log_ctx_t *mp_log_ctx);

void mp_shm_fifo_empty(mp_shm_fifo_ctx_t *mp_shm_fifo_ctx,
        mp_log_ctx_t *mp_log_ctx);

#endif /* MPUTILS_MP_SHM_FIFO_H_ */
