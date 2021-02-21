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
 * @file mp_mem.h
 * @author Rafael Antoniello
 */

#include <stdlib.h>

#ifndef MPUTILS_MP_MEM_H_
#define MPUTILS_MP_MEM_H_

#define mp_malloc malloc

#define mp_free free

typedef void*(*mp_calloc_fxn)(size_t count, size_t eltsize);
extern mp_calloc_fxn mp_calloc;

#endif /* MPUTILS_MP_MEM_H_ */
