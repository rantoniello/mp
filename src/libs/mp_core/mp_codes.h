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
 * @file mp_codes.h
 * @author Rafael Antoniello
 */

#ifndef MP_CORE_MP_CODES_H_
#define MP_CORE_MP_CODES_H_

/**
 * Generic status codes.
 */
typedef enum mp_codes {
    /**
     * Generic success code
     */
    MP_SUCCESS = 0,
    /**
     * Generic error code
     */
    MP_ERROR,
    /**
     * Resource requested found but not modified
     */
    MP_NOTMODIFIED,
    /**
     * Resource requested not found
     */
    MP_ENOTFOUND,
    /**
     * Resource temporarily unavailable (call again)
     */
    MP_EAGAIN,
    /**
     * End of file
     */
    MP_EOF,
    /**
     * Not enough space
     */
    MP_ENOMEM,
    /**
     * Invalid argument
     */
    MP_EINVAL,
    /**
     * Out of memory
     */
    MP_ENOMEM,
    /**
     * Conflict with the current state of the target resource
     */
    MP_ECONFLICT,
    /**
     * Operation timed out
     */
    MP_ETIMEDOUT,
    /**
     * Operation interrupted
     */
    MP_EINTR,
    /**
     * Bad or not supported media format
     */
    MP_EBAVFORMAT,
    /**
     * Bad or not supported multiplex format
     */
    MP_EBMUXFORMAT,
    /**
     * Enumeration maximum value
     */
    MP_CODES_MAX
} mp_codes_enum_t;

#endif /* MP_CORE_MP_CODES_H_ */
