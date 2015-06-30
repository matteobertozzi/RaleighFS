/*
 *   Copyright 2007-2013 Matteo Bertozzi
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

#include <zcl/version.h>
#include <zcl/debug.h>
#include <zcl/eloop.h>
#include <zcl/time.h>
#include <zcl/ipc.h>

#include <r5l/version.h>

#include <string.h>
#include <signal.h>
#include <stdio.h>

#include "server.h"

static r5l_server_t *__server;
static void __signal_handler (int signum) {
  r5l_server_stop_signal(__server, signum);
}

static void __dump_info_header (FILE *stream) {
  fprintf(stream, "%s %s (build-nr: %s git-rev: %s)\n",
                  R5L_SERVER_NAME, R5L_SERVER_VERSION_STR,
                  R5L_SERVER_BUILD_NR, R5L_SERVER_GIT_REV);
  r5l_info_dump(stream);
  zcl_info_dump(stream);
}

int main (int argc, char **argv) {
  z_ipc_server_t *server[2];

  __dump_info_header(stdout);

  z_debug_open();
  if ((__server = r5l_server_open()) == NULL)
    return(1);

  /* Initialize signals */
  signal(SIGINT,  __signal_handler);
  signal(SIGTERM, __signal_handler);

  server[0] = udo_udp_plug(__server->eloop + 0,  "127.0.0.1", "11123", __server);
  server[1] = echo_tcp_plug(__server->eloop + 0, "127.0.0.1", "11124", __server);

  z_event_loop_start(__server->eloop + 0, 1);
  z_event_loop_wait(__server->eloop + 0);

  z_ipc_unplugs(server, 2);
  r5l_server_close(__server);
  z_debug_close();
  return(0);
}
