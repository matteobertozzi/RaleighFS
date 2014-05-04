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

#include <zcl/debug.h>
#include <zcl/atomic.h>
#include <zcl/task-rq.h>
#include <zcl/task.h>
#include <zcl/mem.h>

void z_vtask_reset (z_vtask_t *vtask, uint8_t type) {
  z_memzero(vtask, sizeof(z_vtask_t));
  vtask->flags.type = type;
}

void z_vtask_resume (z_vtask_t *vtask) {
  z_vtask_t *parent = vtask->parent;
  switch (parent->flags.type) {
    case Z_VTASK_TYPE_RQ: {
      z_task_rq_t *rq = z_container_of(parent, z_task_rq_t, vtask);
      z_task_rq_add(rq, vtask);
    }
  }
}

void z_vtask_exec (z_vtask_t *vtask) {
  do {
    vtask->vtime += 1;
    switch (vtask->flags.type) {
      case Z_VTASK_TYPE_RQ: {
        z_task_rq_t *rq = z_container_of(vtask, z_task_rq_t, vtask);
        vtask = z_task_rq_fetch(rq);
        z_atomic_dec(&(rq->refs));
        break;
      }
      case Z_VTASK_TYPE_TASK: {
        z_task_rq_t *parent = z_container_of(vtask->parent, z_task_rq_t, vtask);
        z_task_t *task = z_container_of(vtask, z_task_t, vtask);
        switch (z_task_exec(task)) {
          case Z_TASK_COMPLETED:
          case Z_TASK_ABORTED:
            vtask = NULL;
            break;
          case Z_TASK_CONTINUATION_FUNC:
            break;
          case Z_TASK_CONTINUATION:
            vtask = NULL;
            if (parent) {
              z_task_rq_fini(parent);
            }
            break;
          case Z_TASK_YIELD:
            vtask = NULL;
            break;
        }
        break;
      }
      default: {
        Z_LOG_ERROR("BAD vtask type %d\n", vtask->flags.type);
        vtask = NULL;
        break;
      }
    }
  } while (vtask != NULL);
}
