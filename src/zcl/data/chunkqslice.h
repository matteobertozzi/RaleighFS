/*
 *   Copyright 2012 Matteo Bertozzi
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef _Z_CHUNKQ_SLICE_H_
#define _Z_CHUNKQ_SLICE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/types.h>

#include <zcl/chunkq.h>
#include <zcl/slice.h>

typedef struct {
    z_slice_t __base_type__;
    const z_chunkq_extent_t *extent;
} z_chunkq_slice_t;

void    z_chunkq_slice       (z_chunkq_slice_t *slice,
                              const z_chunkq_extent_t *chunkq_ext);

__Z_END_DECLS__

#endif /* _Z_CHUNKQ_SLICE_H_ */
