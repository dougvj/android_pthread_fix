#ifdef __ANDROID__
// This prevents the shadowing of the pthread functions
#define __PTHREAD_FIX_NO_SHADOW__
#include "android_pthread_fix.h"
#include <errno.h>
#include <assert.h>

// Must be Power of 2
#define THREADS_MAX 512
#define THREADS_MAX_MASK (THREADS_MAX - 1)

// Structure for cancel state management
typedef struct {
  pthread_t thread;
  atomic_int cancel_state;
  atomic_int cancel_type;
  _Atomic bool cancelled;
  _Atomic bool running;
} _pthread_fix_cancel_info;

// The table of cancel info structures for each thread
static _pthread_fix_cancel_info _pthread_fix_cancel_info_table[THREADS_MAX] =
    {};
// The current thread's cancel info structure
static _Thread_local _pthread_fix_cancel_info *_pthread_fix_this_cancel_info;
// The mutex for the cancel info table, only needed for searching new entries
static pthread_mutex_t _pthread_fix_cancel_info_mutex;

void _pthread_fix_set_running(bool running) {
  _pthread_fix_cancel_info *i = _pthread_fix_this_cancel_info;
  if (i) {
    i->running = running;
  }
}

void pthread_testcancel(void) {
  _pthread_fix_cancel_info *i = _pthread_fix_this_cancel_info;
  if (i) {
    if (i->cancelled) {
      if (i->cancel_state == PTHREAD_CANCEL_ENABLE) {
        pthread_fix_log("Thread %ld cancelled from cancel point\n", i->thread);
        _pthread_fix_exit(PTHREAD_CANCELED);
      }
    }
  }
}

// Get the cancel info for a thread
_pthread_fix_cancel_info *_pthread_fix_cancel_info_get(pthread_t thread) {
  // Just do a simple hash of the thread id
  int start = thread & THREADS_MAX_MASK;
  for (int i = 0; i < THREADS_MAX; i++) {
    int search = (i + start) & THREADS_MAX_MASK;
    if (_pthread_fix_cancel_info_table[search].thread == thread) {
      return &_pthread_fix_cancel_info_table[search];
    }
  }
  return NULL;
}

void _pthread_fix_exit(void *retval) {
  _pthread_fix_cancel_info *i = _pthread_fix_this_cancel_info;
  pthread_t thread = pthread_self();
  assert(i->thread == thread);
  *i = (_pthread_fix_cancel_info){0};
  _pthread_fix_this_cancel_info = NULL;
  pthread_fix_log("Thread %ld exited\n", thread);
  pthread_exit(retval);
}

void _pthread_fix_cancel_handler(int sig) {
  _pthread_fix_cancel_info *i = _pthread_fix_this_cancel_info;
  if (i) {
    i->cancelled = true;
    if (i->cancel_state == PTHREAD_CANCEL_ENABLE) {
      if (i->cancel_type == PTHREAD_CANCEL_ASYNCHRONOUS ||
          i->running == false) {
        pthread_fix_log("Thread %ld cancelled from signal\n", i->thread);
        _pthread_fix_exit(PTHREAD_CANCELED);
      }
    }
  } else {
    pthread_fix_log("signal handler called but no cancel info\n");
  }
}

int pthread_cancel(pthread_t thread) {
  _pthread_fix_cancel_info *i = _pthread_fix_cancel_info_get(thread);
  if (i) {
    i->cancelled = true;
    pthread_kill(thread, SIGUSR1);
  } else {
    pthread_fix_log("No cancel info for thread %ld\n", thread);
    return ESRCH;
  }
  return 0;
}

typedef struct {
  void *(*start_routine)(void *);
  void *arg;
  int return_code;
  _Atomic bool complete;
} _pthread_fix_start_info;

static void *_pthread_start_wrap(void *arg) {
  _pthread_fix_start_info *i = arg;
  pthread_mutex_lock(&_pthread_fix_cancel_info_mutex);
  // Find an empty entry
  _pthread_fix_cancel_info *info = NULL;
  pthread_t thread = pthread_self();
  // Very simple hash
  int start = thread & THREADS_MAX_MASK;
  for (unsigned int i = 0; i < THREADS_MAX; i++) {
    int search = (i + start) & THREADS_MAX_MASK;
    if (_pthread_fix_cancel_info_table[search].thread == 0) {
      info = &_pthread_fix_cancel_info_table[search];
      pthread_fix_log("Found free cancel info slot %d for thread %ld\n", search,
                      thread);
      break;
    }
  }
  if (info == NULL) {
    pthread_fix_log("Could not find free cancel info slot\n");
    pthread_mutex_unlock(&_pthread_fix_cancel_info_mutex);
    i->return_code = EAGAIN;
    i->complete = true;
    return NULL;
  }
  info->thread = thread;
  _pthread_fix_this_cancel_info = info;
  pthread_mutex_unlock(&_pthread_fix_cancel_info_mutex);
  // Install the signal handler for cancellation
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  pthread_sigmask(SIG_UNBLOCK, &set, NULL);
  signal(SIGUSR1, _pthread_fix_cancel_handler);
  void *start_arg = i->arg;
  void *(*start_routine)(void *) = i->start_routine;
  i->return_code = 0;
  i->complete = true;
  // i is no longer valid after this point
  void *ret = start_routine(start_arg);
  _pthread_fix_exit(ret);
  return ret;
}

int _pthread_fix_create(pthread_t *restrict thread,
                        const pthread_attr_t *restrict attr,
                        void *(*start_routine)(void *), void *restrict arg) {
  _pthread_fix_start_info info = {
      .start_routine = start_routine,
      .arg = arg,
      .return_code = 0,
      .complete = false,
  };
  int ret = pthread_create(thread, attr, _pthread_start_wrap, &info);
  if (ret != 0) {
    return ret;
  }
  // Wait for completion
  while (!info.complete) {
    sched_yield();
  }
  return info.return_code;
}

int pthread_setcancelstate(int state, int *oldstate) {
  _pthread_fix_cancel_info *i = _pthread_fix_this_cancel_info;
  if (state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE) {
    return EINVAL;
  }
  if (i) {
    if (oldstate) {
      *oldstate = i->cancel_state;
    }
    i->cancel_state = state;
  }
  return 0;
}

int pthread_setcanceltype(int type, int *oldtype) {
  _pthread_fix_cancel_info *i = _pthread_fix_this_cancel_info;
  if (type != PTHREAD_CANCEL_ASYNCHRONOUS && type != PTHREAD_CANCEL_DEFERRED) {
    return EINVAL;
  }
  if (i) {
    if (oldtype) {
      *oldtype = i->cancel_type;
    }
    i->cancel_type = type;
  }
  return 0;
}


#endif
