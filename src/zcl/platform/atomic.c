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

#include <zcl/atomic.h>

#if !defined(Z_ATOMIC_SUPPORT)
int z_atomic_cas (z_atomic_int_t *atom, int e, int n) {
    int cmp;

    z_spin_lock(&(atom->lock));
    if ((cmp = (atom->value == e)))
        atom->value = n;
    z_spin_unlock(&(atom->lock));

    return(cmp);
}

int z_atomic_add (z_atomic_int_t *atom, int value) {
    int r;

    z_spin_lock(&(atom->lock));
    atom->value += value;
    r = atom->value;
    z_spin_unlock(&(atom->lock));

    return(r);
}

int z_atomic_sub (z_atomic_int_t *atom, int value) {
    int r;

    z_spin_lock(&(atom->lock));
    atom->value -= value;
    r = atom->value;
    z_spin_unlock(&(atom->lock));

    return(r);
}
#endif /* !Z_ATOMIC_SUPPORT */

