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

#ifndef _Z_BTREE_H_
#define _Z_BTREE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/bytes.h>

Z_TYPEDEF_STRUCT(z_btree)

struct z_btree {
  void *root;
  unsigned int nnodes;
  unsigned int height;
};

int z_btree_open (z_btree_t *btree);
void z_btree_close (z_btree_t *btree);
int z_btree_insert (z_btree_t *btree, z_bytes_t *key, z_bytes_t *value);
z_bytes_t *z_btree_remove (z_btree_t *self, z_bytes_t *key);
z_bytes_t *z_btree_lookup (const z_btree_t *self, const z_bytes_t *key);
void z_btree_debug (const z_btree_t *btree);

__Z_END_DECLS__

#endif /* !_Z_BTREE_H_ */
