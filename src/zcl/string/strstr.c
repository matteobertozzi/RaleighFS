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

#include <stddef.h>

#include <zcl/memmem.h>
#include <zcl/strlen.h>
#include <zcl/strstr.h>

char *z_strstr (const char *haystack, const char *needle) {
    return((char *)z_memmem(haystack, z_strlen(haystack),
                            needle, z_strlen(needle)));
}

char *z_strrstr (const char *haystack, const char *needle) {
    return((char *)z_memrmem(haystack, z_strlen(haystack),
                             needle, z_strlen(needle)));
}


ssize_t z_strpos (const char *haystack, const char *needle) {
    char *p;
    p = (char *)z_memmem(haystack, z_strlen(haystack),
                         needle, z_strlen(needle));
    return((p != NULL) ? (p - haystack) : -1);
}

ssize_t z_strrpos (const char *haystack, const char *needle) {
    char *p;
    p = (char *)z_memrmem(haystack, z_strlen(haystack),
                          needle, z_strlen(needle));
    return((p != NULL) ? (p - haystack) : -1);
}

