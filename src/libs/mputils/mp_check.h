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
 * @file mp_check.h
 * @author Rafael Antoniello
 */

#ifndef MPUTILS_MP_CHECK_H_
#define MPUTILS_MP_CHECK_H_

#include "mp_log.h"

/**
 * Simple ASSERT implementation: does not exit the program but just outputs
 * an error trace.
 */
#define MP_ASSERT(COND) \
    if(!(COND))\
        MP_LOGE("Assertion failed.\n");

/**
 * Generic trace for tracking check-points failures.
 */
#define MP_CHECK(COND, ACTION) \
    if(!(COND)) {\
        MP_LOGE("Check point failed.\n");\
        ACTION;\
    }

#endif /* MPUTILS_MP_CHECK_H_ */
