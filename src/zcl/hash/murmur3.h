/*
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

#ifndef _Z_MURMUR3_H_
#define _Z_MURMUR3_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/debug.h>

uint32_t z_hash32_murmur3  (uint32_t seed, const void * key, size_t len);
uint64_t z_hash64_murmur3  (uint64_t seed, const void * key, size_t len);
void     z_hash128_murmur3 (const void * key, const size_t len, uint64_t digest[2]);

__Z_END_DECLS__

#endif /* _Z_MURMUR3_H_ */
