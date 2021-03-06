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

#ifndef _Z_SNPRINTF_H_
#define _Z_SNPRINTF_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <stdarg.h>

int     z_snprintf      (char *buf,
                         unsigned int bufsize,
                         const char *frmt, ...);
int     z_vsvnprintf    (char *buf,
                         unsigned int bufsize,
                         const char *frmt,
                         va_list ap);

__Z_END_DECLS__

#endif /* !_Z_SNPRINTF_H_ */