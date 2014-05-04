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

#include <zcl/mutex.h>
#include <zcl/debug.h>

#include <execinfo.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

struct debug_conf {
  z_mutex_t lock;
  int log_level;
};

static struct debug_conf __current_debug_conf;

static const char __weekday_name[7][3] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char __month_name[12][3] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char *__log_level[6] = {
  "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE",
};

static void __date_time (char *buffer) {
  struct tm datetime;
  time_t t;

  t = time(NULL);
  localtime_r(&t, &datetime);

  snprintf(buffer, 25, "%.3s %.3s %.2d %d %.2d:%.2d:%.2d ",
           __weekday_name[datetime.tm_wday],
           __month_name[datetime.tm_mon],
           datetime.tm_mday, 1900 + datetime.tm_year,
           datetime.tm_hour, datetime.tm_min, datetime.tm_sec);
}

void __z_log (FILE *fp, int level, int errnum,
              const char *file, int line, const char *func,
              const char *format, ...)
{
  char datetime[25];
  va_list ap;

  z_mutex_lock(&(__current_debug_conf.lock));
  __date_time(datetime);
  fprintf(fp, "[%s] %-5s %s:%d %s() - ",
          datetime, __log_level[level], file, line, func);

  va_start(ap, format);
  vfprintf(fp, format, ap);
  va_end(ap);

  if (errnum != 0) {
    fprintf(fp, ": errno %d %s", errnum, strerror(errnum));
  }

  fprintf(fp, "\n");
  z_mutex_unlock(&(__current_debug_conf.lock));
}

void __z_assert (const char *file, int line, const char *func,
                 int vcond, const char *condition,
                 const char *format, ...)
{
  char datetime[25];
  va_list ap;

  z_mutex_lock(&(__current_debug_conf.lock));
  __date_time(datetime);
  fprintf(stderr, "[%s] ASSERTION (%s) at %s:%d in function %s() failed. ",
          datetime, condition, file, line, func);

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);

  fprintf(stderr, "\n");
  z_mutex_unlock(&(__current_debug_conf.lock));

  abort();
}

void z_dump_stack_trace (FILE *fp, int levels) {
  void *trace[64];
  char **symbols;
  int i, size;

  if (Z_UNLIKELY(levels == 0 || levels > 64))
    levels = 64;

  size = backtrace(trace, levels);
  if ((symbols = backtrace_symbols(trace, size)) == NULL) {
    perror("backtrace_symbols()");
    return;
  }

  z_mutex_lock(&(__current_debug_conf.lock));
  fprintf(fp, "Obtained %zd stack frames.\n", size);
  for (i = 0; i < size; i++) {
    fprintf(fp, "%s\n", symbols[i]);
  }
  z_mutex_unlock(&(__current_debug_conf.lock));

  free(symbols);
}

void z_debug_open (void) {
  z_mutex_alloc(&(__current_debug_conf.lock));
  __current_debug_conf.log_level = Z_LOG_LEVEL_DEFAULT;
}

void z_debug_close (void) {
  z_mutex_free(&(__current_debug_conf.lock));
}

int z_debug_get_log_level (void) {
  return(__current_debug_conf.log_level);
}

void z_debug_set_log_level (int level) {
  z_mutex_lock(&(__current_debug_conf.lock));
  __current_debug_conf.log_level = level;
  z_mutex_unlock(&(__current_debug_conf.lock));
}
