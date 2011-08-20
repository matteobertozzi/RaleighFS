/*
 *   Copyright 2011 Matteo Bertozzi
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

#ifndef _Z_MEMCMP_H_
#define _Z_MEMCMP_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

int     z_memcmp      (const void *m1, const void *m2, unsigned int n);
int     z_memcmp8     (const void *m1, const void *m2, unsigned int n);
int     z_memcmp16    (const void *m1, const void *m2, unsigned int n);
int     z_memcmp32    (const void *m1, const void *m2, unsigned int n);
int     z_memcmp64    (const void *m1, const void *m2, unsigned int n);

__Z_END_DECLS__

#endif /* !_Z_MEMCMP_H */
