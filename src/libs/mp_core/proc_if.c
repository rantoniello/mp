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

#include "proc_if.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "mp_codes.h"
#include "mp_calias.h"
#include <libmediaprocsutils/log.h>
#include <libmediaprocsutils/check_utils.h>
#include <libmediaprocsutils/mem_utils.h>

proc_if_t* proc_if_allocate()
{
    return (proc_if_t*)mp_calloc(1, sizeof(proc_if_t));
}

proc_if_t *proc_if_dup(const proc_if_t *proc_if_arg)
{
    proc_if_t *proc_if;

    if (proc_if_arg == NULL)
        return NULL;

    proc_if = proc_if_allocate();
    if (proc_if == NULL)
        return NULL;

    if (proc_if_arg->proc_name != NULL)
        proc_if->proc_name = mp_strdup(proc_if_arg->proc_name);

    if (proc_if_arg->proc_type != NULL)
        proc_if->proc_type = mp_strdup(proc_if_arg->proc_type);

    proc_if->open = proc_if_arg->open;
    proc_if->close = proc_if_arg->close;
    proc_if->put = proc_if_arg->put;
    proc_if->get = proc_if_arg->get;
    proc_if->process_frame = proc_if_arg->process_frame;
    proc_if->opt = proc_if_arg->opt;
    proc_if->ififo_fxn = proc_if_arg->ififo_fxn;
    proc_if->ofifo_fxn = proc_if_arg->ofifo_fxn;

    return proc_if;
}

int proc_if_cmp(const proc_if_t *proc_if1, const proc_if_t *proc_if2)
{
    int ret_val = MP_ERROR; // means "not equal"

    if (proc_if1 == NULL || proc_if2 == NULL)
        return MP_EINVAL;

    if (mp_strcmp(proc_if1->proc_name, proc_if2->proc_name) == 0 &&
            mp_strcmp(proc_if1->proc_type, proc_if2->proc_type) == 0 &&
            proc_if1->open == proc_if2->open &&
            proc_if1->close == proc_if2->close &&
            proc_if1->put == proc_if2->put &&
            proc_if1->get == proc_if2->get &&
            proc_if1->process_frame == proc_if2->process_frame &&
            proc_if1->opt == proc_if2->opt &&
            proc_if1->ififo_fxn == proc_if2->ififo_fxn &&
            proc_if1->ofifo_fxn == proc_if2->ofifo_fxn)
        ret_val = MP_SUCCESS;

    return ret_val;
}

void proc_if_release(proc_if_t **ref_proc_if)
{
    proc_if_t *proc_if;

    if (ref_proc_if == NULL || (proc_if = *ref_proc_if) == NULL)
        return;

    if (proc_if->proc_name != NULL)
        mp_free(proc_if->proc_name);

    if (proc_if->proc_type != NULL)
        mp_free(proc_if->proc_type);

    mp_free(proc_if);
    *ref_proc_if = NULL;
}

void* proc_if_fifo_elem_ctx_memcpy_default(void *opaque, void *dest,
        const void *src, size_t size, log_ctx_t *log_ctx)
{
    proc_frame_ctx_t *proc_frame_ctx = NULL;
    LOG_CTX_INIT(log_ctx);

    /* Note:
     * default src -> proc_frame_ctx_t*
     * default dst -> void* @ memory pool
     * default size -> sizeof(void*)
     */

    /* Duplicate input frame */
    proc_frame_ctx = proc_frame_ctx_dup((const proc_frame_ctx_t*)src);
    CHECK_DO(proc_frame_ctx != NULL , return NULL);

    /* Copy the pointer value (thus pass pointer address) */
    //memcpy(dest, &proc_frame_ctx, sizeof(void*));
    *(proc_frame_ctx_t**)dest = proc_frame_ctx;
LOGW("#### proc_if_fifo_elem_ctx_memcpy_default proc_frame_ctx-push = %p wxh= %d x %d\n", (*(proc_frame_ctx_t**)dest),
        (*(proc_frame_ctx_t**)dest)->width[0], (*(proc_frame_ctx_t**)dest)->height[0]); //FIXME!!
    return dest;
}

int proc_if_fifo_elem_ctx_dequeue_default(void *opaque, void **ref_elem,
        size_t *ref_elem_size, const void *src, size_t size,
        log_ctx_t *log_ctx)
{
    LOG_CTX_INIT(log_ctx); /* Argument 'log_ctx' is allowed to be NULL */

    /* Note:
     * default src -> proc_frame_ctx_t* element dequeued from FIFO)
     * default size -> sizeof(void*)
     */

    /* Check arguments */
    CHECK_DO(ref_elem != NULL && ref_elem_size != NULL && src != NULL &&
            size == sizeof (proc_frame_ctx_t**), return STAT_ERROR);
LOGW("#### proc_if_fifo_elem_ctx_dequeue_default proc_frame_ctx dequeue = %p; size = %zu hxw = %d x %d\n", *ref_elem, *ref_elem_size,
        (*(proc_frame_ctx_t**)src)->height[0], (*(proc_frame_ctx_t**)src)->width[0]); fflush(stdout); //FIXME!!
    /* Just pass the pointer stored in FIFO (copy pointer value) */
    *ref_elem = *(proc_frame_ctx_t**)src; //FIXME!!
    //memcpy(ref_elem, src, size);
    *ref_elem_size = size;
LOGW("#### proc_if_fifo_elem_ctx_dequeue_default proc_frame_ctx= %p; size = %zu\n", *ref_elem, *ref_elem_size); //FIXME!!
    return STAT_SUCCESS;
}
