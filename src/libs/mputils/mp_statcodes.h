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
 * @file mp_statcodes.h
 * @author Rafael Antoniello
 */

#ifndef MPUTILS_MP_STATCODES_H_
#define MPUTILS_MP_STATCODES_H_

/**
 * Generic status codes.
 */
typedef enum mp_statcodes_enum {
    MP_SUCCESS = 0, //< Generic success code
    MP_ERROR, //< Generic error code
    MP_NOTMODIFIED, //< Resource requested found but not modified
    MP_ENOTFOUND, //< Resource requested not found
    MP_EAGAIN, //< Resource temporarily unavailable (call again)
    MP_EOF, //< End of file
    MP_ENOMEM, //< Not enough space
    MP_EINVAL, //< Invalid argument
    MP_ECONFLICT, //< Conflict with the current state of the target resource
    MP_ETIMEDOUT, //< Operation timed out
    MP_EINTR, //< Operation interrupted

    MP_EBESFORMAT, //< Bad or not supported elementary stream format
    MP_EBMUXFORMAT, //< Bad or not supported multiplex format

    MP_STATCODES_MAX
} mp_statcodes_t;

#endif /* MPUTILS_MP_STATCODES_H_ */
