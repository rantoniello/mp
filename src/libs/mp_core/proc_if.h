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
 * @file proc_if.h
 * @author Rafael Antoniello
 */

#ifndef MP_CORE_PROC_IF_H_
#define MP_CORE_PROC_IF_H_

#include <sys/types.h>
#include <inttypes.h>
#include <stdarg.h>

typedef struct proc_ctx_s proc_ctx_t;
typedef struct proc_if_s proc_if_t;
typedef struct log_ctx_s log_ctx_t;
typedef struct fifo_ctx_s fifo_ctx_t;
typedef void *(enqueue_fxn)(void *opaque, void *dest, const void *src,
        size_t size, log_ctx_t *log_ctx);
typedef int (dequeue_fxn)(void *opaque, void **ref_elem,
        size_t *ref_elem_size, const void *src, size_t size,
        log_ctx_t *log_ctx);

/**
 * Processor interface: Each processor implementation must instantiate a
 * static unambiguous interface of this type.
 */
typedef struct proc_if_s {
	/**
	 * Unambiguous processor identifier name
	 */
	char *proc_name;
	/**
	 * Processor type
	 */
	char *proc_type;
	/**
	 * Instantiate specific processor.
	 * This callback is mandatory (cannot be NULL).
	 * @param proc_if Pointer to the processor interface.
	 * @param settings Pointer to a JSON formatted character string containing
	 * initial settings for the processor.
	 * @param log_ctx Pointer to an externally instantiated logger.
	 * @param arg Variable list of parameters defined by user.
	 * @return Pointer to the new processor context structure instance on
	 * success, NULL if fails.
	 */
	proc_ctx_t *(*open)(const proc_if_t *proc_if, const char *settings,
	        log_ctx_t *const log_ctx, va_list arg);
	/**
	 * Release processor instance.
	 * This callback is mandatory (cannot be NULL).
	 * @param ref_proc_ctx Reference to the pointer to the processor instance
	 * to release.
	 */
	void (*close)(proc_ctx_t **ref_proc_ctx);
	/**
	 * Put new processor settings.
	 * This callback is optional (can be set to NULL).
	 * @param proc_ctx Pointer to the processor instance.
	 * @param settings Pointer to a JSON formatted character string containing
	 * new settings for the processor.
	 * @return MP_SUCCESS code in case of success.
	 */
	int (*put)(const proc_ctx_t *proc_ctx, const char *settings);
	/**
	 * Get current processor settings and status.
	 * This method is asynchronous and thread safe.
	 * This callback is optional (can be set to NULL).
	 * @param proc_ctx Pointer to the processor instance.
	 * @param ref_status Reference to the pointer to a data structure
	 * returning the processor's representational state (including current
	 * settings) as a JSON formatted character string.
	 * @return MP_SUCCESS code in case of success.
	 */
	int (*get)(const proc_ctx_t *proc_ctx, char **ref_status);
	/**
	 * Process one frame of data. The frame is read from the input FIFO buffer
	 * and is completely processed. If an output frame is produced, is written
	 * to the output FIFO buffer.
	 * This callback is mandatory (cannot be NULL).
	 * @param proc_ctx Pointer to the processor instance.
	 * @param fifo_ctx_i Pointer to the input FIFO buffer.
	 * @param fifo_ctx_o Pointer to the output FIFO buffer.
	 * @return MP_SUCCESS code in case of success.
	 */
	int (*process_frame)(const proc_ctx_t *proc_ctx, fifo_ctx_t *fifo_ctx_i,
			fifo_ctx_t *fifo_ctx_o);
	/**
	 * Request for specific processor options.
	 * This callback is optional (can be set to NULL).
	 * @param proc_ctx Pointer to the processor instance.
	 * @param tag Processor option tag identifier string. Refer to the
	 * specific implementation of this function to see the available options.
	 * arg Variable list of parameters according to selected option. Refer to
	 * the specific implementation of this function to see the parameters
	 * corresponding to each available option.
	 * @return MP_SUCCESS code in case of success.
	 */
	int (*opt)(const proc_ctx_t *proc_ctx, const char *tag, va_list arg);
    /**
     * This callback is used by the processor to dequeue data frames to be
     * processed from the input FIFO.
     * This callback is optional, and can be set to NULL. In that case, the
     * data is managed as a raw byte buffer (referenced by a pointer to
     * uint8_t) and will be copied using the default function.
     */
    dequeue_fxn *ififo_fxn;
	/**
	 * This callback is used by the processor to enqueue processed data frames
	 * to the output FIFO.
	 * This callback is optional, and can be set to NULL. In that case, the
	 * data is managed as a raw byte buffer (referenced by a pointer to
	 * uint8_t) and will be copied using the default function.
	 */
	enqueue_fxn *ofifo_fxn;
} proc_if_t;

/**
 * Allocate an uninitialized processor interface context structure.
 * @return Pointer to interface instance; NULL if fails.
 */
proc_if_t* proc_if_allocate();

/**
 * Duplicate a processor interface context structure.
 * @param proc_if_arg Pointer to the processor interface context structure to
 * be duplicated.
 * @return Pointer to a new replica of the given structure; NULL if fails.
 */
proc_if_t* proc_if_dup(const proc_if_t *proc_if_arg);

/**
 * Compares if given processor interfaces are the equal.
 * @return Code MP_SUCCESS if given interfaces are equal.
 */
int proc_if_cmp(const proc_if_t* proc_if1, const proc_if_t* proc_if2);

/**
 * Release a processor interface context structure.
 * @param Reference to the pointer to the processor interface context structure
 * to be released. Pointer is set to NULL on return.
 */
void proc_if_release(proc_if_t **ref_proc_if);

/**
 * //TODO
 */
void* proc_if_fifo_elem_ctx_memcpy_default(void *opaque, void *dest,
        const void *src, size_t size, log_ctx_t *log_ctx);

/**
 * //TODO
 */
int proc_if_fifo_elem_ctx_dequeue_default(void *opaque, void **ref_elem,
        size_t *ref_elem_size, const void *src, size_t size,
        log_ctx_t *log_ctx);

#endif /* MP_CORE_PROC_IF_H_ */
