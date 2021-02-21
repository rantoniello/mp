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
 * @file mp_log.c
 * @author Rafael Antoniello
 */

#include "mp_log.h"

#include <stdlib.h>
#include <stdio.h>

#include "mp_check.h"
#include "mp_mem.h"

/** Color codes for terminal depending on logging level */
static const char *mp_log_color[MP_LOG_ENUM_MAX] = {
    "\x1B[0m", /*Normal: MP_LOG_DBG*/
    "\x1B[33m", /*Yellow: MP_LOG_WAR*/
    "\033[1;31m" /*Bold red: MP_LOG_ERR*/
};

typedef struct mp_log_ctx_s {
    void *opaque_logger_ctx;
    mp_log_ext_trace_fxn_t mp_log_ext_trace_fxn;
} mp_log_ctx_t;

mp_log_ctx_t* mp_log_open(void *const opaque_logger_ctx,
        mp_log_ext_trace_fxn_t mp_log_ext_trace_fxn)
{
    mp_log_ctx_t *mp_log_ctx;

    /* Check arguments
     * Parameter 'opaque_logger_ctx_ptr' is allowed to be NULL (use default)
     * Parameter 'mp_log_ext_trace_fxn' is allowed to be NULL
     */

    mp_log_ctx = (mp_log_ctx_t*)mp_calloc(1, sizeof(mp_log_ctx_t));
    if (mp_log_ctx == NULL)
        return NULL;
    mp_log_ctx->opaque_logger_ctx = opaque_logger_ctx;
    mp_log_ctx->mp_log_ext_trace_fxn = mp_log_ext_trace_fxn;

    return mp_log_ctx;
}

void mp_log_close(mp_log_ctx_t **ref_mp_log_ctx)
{
    mp_log_ctx_t *mp_log_ctx;

    if (ref_mp_log_ctx == NULL || (mp_log_ctx = *ref_mp_log_ctx) == NULL)
        return;

    mp_free(mp_log_ctx);
    *ref_mp_log_ctx = NULL;
}

void mp_log_trace(mp_log_ctx_t *mp_log_ctx, mp_log_level_t mp_log_level,
        const char *file_name, int line, const char *func_name,
        const char *format, ...)
{
    mp_log_ext_trace_fxn_t mp_log_ext_trace_fxn;
    va_list args;

    if (mp_log_ctx == NULL || mp_log_level >= MP_LOG_ENUM_MAX ||
            file_name == NULL || func_name == NULL || format == NULL)
        return;

    va_start(args, format);

    mp_log_ext_trace_fxn = mp_log_ctx->mp_log_ext_trace_fxn;

    if (mp_log_ext_trace_fxn == NULL) {
        printf("%s%s-%d: ", mp_log_color[mp_log_level], file_name, line);
        vprintf(format, args);
        printf("%s", mp_log_color[MP_LOG_DBG]); // back to "normal"
        fflush(stdout);
    } else {
        mp_log_ext_trace_fxn(mp_log_ctx->opaque_logger_ctx, mp_log_level,
                file_name, line, func_name, format, args);
    }

    va_end(args);
    return;
}
