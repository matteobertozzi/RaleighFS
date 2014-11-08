#include <zcl/histogram.h>
#include <zcl/allocator.h>
#include <zcl/task-rq.h>
#include <zcl/vtask.h>
#include <zcl/task.h>
#include <zcl/global.h>
#include <zcl/array.h>
#include <zcl/humans.h>
#include <zcl/iopoll.h>
#include <zcl/memory.h>
#include <zcl/hashmap.h>
#include <zcl/thread.h>
#include <zcl/waitcond.h>
#include <zcl/mutex.h>
#include <zcl/spinlock.h>
#include <zcl/dbuffer.h>
#include <zcl/avl16.h>
#include <zcl/spsc.h>
#include <zcl/ipc.h>
#include <zcl/txn.h>
#include <stdio.h>

#define __print_size(type)                                              \
  do {                                                                  \
    char buffer[32];                                                    \
    z_human_size(buffer, 32, sizeof(type));                             \
    printf("%25s %10s (%5d) %6.2f 64s\n",                               \
           #type, buffer, (int)sizeof(type), (float)sizeof(type)/64);   \
  } while (0)

int main (int argc, char **argv) {
  printf("core/global\n");
  __print_size(z_global_conf_t);

  printf("core/memory\n");
  __print_size(z_allocator_t);
  __print_size(z_memory_t);

  printf("data/buffer\n");
  __print_size(z_dbuf_node_t);
  __print_size(z_dbuf_writer_t);
  __print_size(z_dbuf_reader_t);

  printf("dispatch\n");
  __print_size(z_task_t);
  __print_size(z_vtask_t);
  __print_size(z_vtask_tree_t);
  __print_size(z_vtask_queue_t);
  __print_size(z_task_rq_t);
  __print_size(z_task_rq_rr_t);
  __print_size(z_task_rq_fifo_t);
  __print_size(z_task_rq_fair_t);

  printf("io/poll\n");
  __print_size(z_iopoll_entity_t);
  __print_size(z_iopoll_stats_t);
  __print_size(z_iopoll_engine_t);

  printf("io/ipc\n");
  __print_size(z_ipc_server_t);
  __print_size(z_ipc_client_t);

  printf("io/ipc-msg\n");
  __print_size(z_ipc_msg_client_t);
  __print_size(z_ipc_msg_rdbuf_t);
  __print_size(z_ipc_msg_t);

  printf("system\n");
  __print_size(z_thread_t);
  __print_size(z_mutex_t);
  __print_size(z_spinlock_t);
  __print_size(z_wait_cond_t);

  printf("metrics\n");
  __print_size(z_histogram_t);

  printf("txn\n");
  __print_size(z_txn_t);
  __print_size(z_txn_mvcc_t);
  __print_size(z_txn_mgr_t);

  printf("util/array\n");
  __print_size(z_array_t);

  printf("util/map\n");
  __print_size(z_hash_map_t);

  printf("util/ringbuf\n");
  __print_size(z_spsc_t);

  printf("util/sset\n");
  __print_size(z_avl16_node_t);
  __print_size(z_avl16_iter_t);

  return(0);
}
