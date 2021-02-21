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
 * @file mp_log.h
 * @author Rafael Antoniello
 */

#ifndef MPUTILS_MP_LOG_H_
#define MPUTILS_MP_LOG_H_

#include <stdarg.h>

/* **** Definitions **** */

/* Forward declarations */
typedef struct mp_log_ctx_s mp_log_ctx_t;

/** Logging levels enumeration type */
typedef enum mp_log_level_enum {
    MP_LOG_DBG = 0,
    MP_LOG_WAR,
    MP_LOG_ERR,
    MP_LOG_ENUM_MAX
} mp_log_level_t;

/** This MACRO defines the source code filename without path */
#define __FILENAME__ strrchr("/" __FILE__, '/') + 1

/**
 * This is an internal MACRO, used by the logger MACROS LOG[D/W/E...] for
 * tracing.
 */
#define MP_LOG(LEVEL, FORMAT, ...) \
        mp_log_trace(__mp_log_ctx, LEVEL, __FILENAME__, __LINE__, \
                __FUNCTION__, FORMAT, ##__VA_ARGS__)

/**
 * This MACRO may be called in any function before using the LOG[D/W/E...]
 * MACROs. It just declare and initializes a local pointer to a given logger
 * context structure.
 */
#define MP_LOG_INIT(MP_LOG_CTX) mp_log_ctx_t *__mp_log_ctx = MP_LOG_CTX

/**
 * This MACRO sets (but not declare) the value of local pointer __mp_log_ctx.
 */
#define MP_LOG_SET(MP_LOG_CTX) __mp_log_ctx = MP_LOG_CTX

/**
 * This MACRO fetches the local pointer __mp_log_ctx.
 */
#define MP_LOG_GET() __mp_log_ctx

/**
 * Logger debug MACRO
 */
#define MP_LOGD(FORMAT, ...) MP_LOG(MP_LOG_DBG, FORMAT, ##__VA_ARGS__)

/**
 * Logger warning MACRO
 */
#define MP_LOGW(FORMAT, ...) MP_LOG(MP_LOG_WAR, FORMAT, ##__VA_ARGS__)

/**
 * Logger error MACRO
 */
#define MP_LOGE(FORMAT, ...) MP_LOG(MP_LOG_ERR, FORMAT, ##__VA_ARGS__)

/** Private (externally defined) logging callback prototype */
typedef void (*mp_log_ext_trace_fxn_t)(void *opaque_logger_ctx,
        mp_log_level_t mp_log_level, const char *file_name,
        int line, const char *func_name, const char *format, va_list args);

/* **** Prototypes **** */

/**
 * Creates an instance of this module.
 * @param opaque_logger_ctx This opaque pointer will be passed to the callback
 * function mp_log_ext_trace_fxn.
 * @param mp_log_ext_trace_fxn Private logging function passed as a
 * callback.
 * @return Instance handler.
 */
mp_log_ctx_t* mp_log_open(void *const opaque_logger_ctx,
        mp_log_ext_trace_fxn_t mp_log_ext_trace_fxn);

/**
 * Release an instance of this module, obtained by a previous call to
 * mp_log_open().
 * @param ref_mp_log_ctx Reference to the pointer to the module instance
 * handler.
 */
void mp_log_close(mp_log_ctx_t **ref_mp_log_ctx);

/**
 * Logger's tracing function.
 * @param mp_log_ctx Module instance handler.
 * @param mp_log_level Logging level
 * @param file_name Caller filename
 * @param line Caller code line
 * @param func_name Caller function name
 * @param format Controls the output as in C printf
 * @param ... Variable list of arguments
 */
void mp_log_trace(mp_log_ctx_t *mp_log_ctx, mp_log_level_t mp_log_level,
        const char *file_name, int line, const char *func_name,
        const char *format, ...);

#endif /* MPUTILS_MP_LOG_H_ */
