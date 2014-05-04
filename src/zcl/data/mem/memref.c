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

#include <zcl/memref.h>
#include <zcl/debug.h>

void z_memref_reset (z_memref_t *self) {
  z_memslice_clear(&(self->slice));
  self->vtable = NULL;
  self->object = NULL;
}

void z_memref_set_data (z_memref_t *self,
                           const void *data,
                           unsigned int size,
                           const z_vtable_refs_t *vtable,
                           void *object)
{
  if (Z_UNLIKELY(self == NULL)) return;

  Z_ASSERT((object == NULL && vtable == NULL) || (object != NULL && vtable != NULL),
           "vtable must be specified if the object is NOT NULL");
  z_memslice_set(&(self->slice), data, size);
  self->vtable = vtable;
  self->object = object;
}

void z_memref_set (z_memref_t *self,
                      const z_memslice_t *slice,
                      const z_vtable_refs_t *vtable,
                      void *object)
{
  if (Z_UNLIKELY(self == NULL)) return;

  Z_ASSERT((object == NULL && vtable == NULL) || (object != NULL && vtable != NULL),
           "vtable must be specified if the object is NOT NULL");
  z_memslice_copy(&(self->slice), slice);
  self->vtable = vtable;
  self->object = object;
}

void z_memref_acquire (z_memref_t *self, const z_memref_t *other) {
  if (Z_UNLIKELY(self == NULL)) return;

  Z_ASSERT(other != NULL, "the ref to acquire must be NOT NULL");
  z_memref_set(self, &(other->slice), other->vtable, other->object);
  if (self->object != NULL) {
    self->vtable->inc_ref(self->object);
  }
}

void z_memref_release (z_memref_t *self) {
  if (self == NULL) return;

  if (self->object != NULL) {
    self->vtable->dec_ref(self->object);
    z_memref_reset(self);
  }
}
