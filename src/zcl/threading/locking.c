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

#include <zcl/threading.h>
#include <zcl/locking.h>
#include <zcl/atomic.h>
#include <zcl/debug.h>

/* ============================================================================
 *  Read-Write lock
 */
#define Z_RWLOCK_NUM_READERS_MASK         (0x7fffffff)
#define Z_RWLOCK_WRITE_FLAG               (1 << 31)

void z_rwlock_init (z_rwlock_t *lock) {
  lock->state = 0;
}

void z_rwlock_read_lock (z_rwlock_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    expval = curval & Z_RWLOCK_NUM_READERS_MASK;   /* no wrlock expval */
    newval = expval + 1;                           /* Add me as reader */
  });
}

void z_rwlock_read_unlock (z_rwlock_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    expval = curval;       /* I expect a write lock and other readers */
    newval = expval - 1;   /* Drop me as reader */
  });
}

void z_rwlock_write_lock (z_rwlock_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    expval = curval & Z_RWLOCK_NUM_READERS_MASK;  /* I expect some 0+ readers */
    newval = Z_RWLOCK_WRITE_FLAG | expval;        /* I want to lock the other writers */
  });

  /* Wait pending reads */
  while ((lock->state & Z_RWLOCK_NUM_READERS_MASK) > 0)
    z_thread_yield();
}

void z_rwlock_write_unlock (z_rwlock_t *lock) {
  lock->state = 0;
}

int z_rwlock_try_read_lock (z_rwlock_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    if (curval & Z_RWLOCK_WRITE_FLAG)
      return(0);

    expval = curval & Z_RWLOCK_NUM_READERS_MASK;  /* I expect some 0+ readers */
    newval = expval + 1;                          /* Add me as reader */
  });
  return(1);
}

int z_rwlock_try_write_lock (z_rwlock_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    if (curval & Z_RWLOCK_WRITE_FLAG)
      return(0);

    expval = curval & Z_RWLOCK_NUM_READERS_MASK;      /* I expect some 0+ readers */
    newval = Z_RWLOCK_WRITE_FLAG | expval;  /* I want to lock the other writers */
  });

  /* Wait pending reads */
  while ((lock->state & Z_RWLOCK_NUM_READERS_MASK) > 0)
    z_thread_yield();
  return(1);
}

/* ============================================================================
 *  Read-Write-Commit Semaphore
 */
int z_rwcsem_init (z_rwcsem_t *lock) {
  lock->state = 0;
  return(0);
}

int z_rwcsem_try_acquire_read (z_rwcsem_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    if (curval & Z_RWCSEM_COMMIT_FLAG)
      return(0);

    expval = curval & Z_RWCSEM_RW_MASK;
    newval = expval + 1;
  });
  return(1);
}

int z_rwcsem_release_read (z_rwcsem_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    expval = curval;
    newval = expval - 1;
  });
  return(newval);
}

int z_rwcsem_try_acquire_write (z_rwcsem_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    if (curval & Z_RWCSEM_CW_MASK)
      return(0);

    expval = curval & Z_RWCSEM_READERS_MASK;
    newval = expval | Z_RWCSEM_WRITE_FLAG;
  });
  return(1);
}

int z_rwcsem_release_write (z_rwcsem_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    expval = curval;
    newval = expval & Z_RWCSEM_LC_MASK;
  });
  return(newval);
}

void z_rwcsem_set_commit_flag (z_rwcsem_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    expval = curval;
    newval = expval | Z_RWCSEM_COMMIT_FLAG;
  });
}

int z_rwcsem_has_commit_flag (z_rwcsem_t *lock) {
  return(!!(z_atomic_load(&(lock->state)) & Z_RWCSEM_COMMIT_FLAG));
}

int z_rwcsem_try_acquire_commit (z_rwcsem_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    if (curval != Z_RWCSEM_COMMIT_FLAG)
      return(0);

    expval = Z_RWCSEM_COMMIT_FLAG;
    newval = Z_RWCSEM_COMMIT_FLAG | Z_RWCSEM_WRITE_FLAG;
  });
  return(1);
}

int z_rwcsem_release_commit (z_rwcsem_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    expval = curval;
    newval = expval & Z_RWCSEM_LOCK_FLAG;
  });
  return(newval);
}

void z_rwcsem_set_lock_flag (z_rwcsem_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    expval = curval;
    newval = expval | Z_RWCSEM_LOCK_FLAG;
  });
}

int z_rwcsem_has_lock_flag (z_rwcsem_t *lock) {
  return(!!(z_atomic_load(&(lock->state)) & Z_RWCSEM_LOCK_FLAG));
}

int z_rwcsem_try_acquire_lock (z_rwcsem_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    if (curval != Z_RWCSEM_LOCK_FLAG)
      return(0);

    expval = Z_RWCSEM_LOCK_FLAG;
    newval = Z_RWCSEM_LOCK_FLAG | Z_RWCSEM_WRITE_FLAG;
  });
  return(1);
}

int z_rwcsem_try_release_lock (z_rwcsem_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    expval = curval;
    newval = 0;
  });
  return(newval);
}

int z_rwcsem_try_switch (z_rwcsem_t *lock, z_rwcsem_op_t op_current, z_rwcsem_op_t op_next) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    if (op_current == Z_RWCSEM_WRITE) {
      if (op_next == Z_RWCSEM_READ) {
        // write -> read
        if (curval & Z_RWCSEM_COMMIT_FLAG)
          return(0);
        expval = curval & Z_RWCSEM_RW_MASK;
        newval = (expval & Z_RWCSEM_LC_MASK) + 1;
      } else if (op_next == Z_RWCSEM_COMMIT) {
        // write -> commit
        return(curval == (Z_RWCSEM_COMMIT_FLAG | Z_RWCSEM_WRITE_FLAG));
      } else {
        return(0);
      }
    } else {
      return(0);
    }
  });
  return(newval);
}

int z_rwcsem_try_acquire (z_rwcsem_t *lock, z_rwcsem_op_t operation) {
  switch (operation) {
    case Z_RWCSEM_READ:
      return(z_rwcsem_try_acquire_read(lock));
    case Z_RWCSEM_WRITE:
      return(z_rwcsem_try_acquire_write(lock));
    case Z_RWCSEM_COMMIT:
      return(z_rwcsem_try_acquire_commit(lock));
    case Z_RWCSEM_LOCK:
      return(z_rwcsem_try_acquire_lock(lock));
  }
  /* Never reached */
  return(-1);
}

int z_rwcsem_release (z_rwcsem_t *lock, z_rwcsem_op_t operation) {
  switch (operation) {
    case Z_RWCSEM_READ:
      return(z_rwcsem_release_read(lock));
    case Z_RWCSEM_WRITE:
      return(z_rwcsem_release_write(lock));
    case Z_RWCSEM_COMMIT:
      return(z_rwcsem_release_commit(lock));
    case Z_RWCSEM_LOCK:
      return(z_rwcsem_try_release_lock(lock));
  }
  /* Never reached */
  return(-1);
}
