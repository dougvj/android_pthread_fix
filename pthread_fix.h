#ifdef __ANDROID__
#ifndef __PTHREAD_FIX_H__
#define __PTHREAD_FIX_H__
// We have to include all libcalls that are cancellation points
// so that we can wrap them with our own functions.
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

// For debugging
#define pthread_fix_log(...) fprintf(stderr, "pthread_fix: " __VA_ARGS__);
// #define pthread_fix_log(...) do {} while(0)

// Make the defaults 0
#define PTHREAD_CANCEL_ENABLE 0
#define PTHREAD_CANCEL_DISABLE 1
#define PTHREAD_CANCEL_DEFERRED 0
#define PTHREAD_CANCEL_ASYNCHRONOUS 1
#define PTHREAD_CANCELED ((void *)-1)

void _pthread_fix_exit(void *retval);

int _pthread_fix_create(pthread_t *restrict thread,
                        const pthread_attr_t *restrict attr,
                        void *(*start_routine)(void *), void *restrict arg);

void _pthread_fix_set_running(bool running);

// We have to wrap all the libcalls that are cancellation points.
//
// We check if the thread is cancelled and if so we exit the thread
// We also set the running flag to false so that the cancellation
// handler knows that the thread is not running (IE, in a blocking
// call) and can safely issue pthread_exit

// Note that this approach, where we wrap the libcalls at the source-code
// level with defines, is not the best way to do this, we should instead have
// the linker replace the libcalls with our own functions, but this is simpler
// for now.
//
// This is noticable if you have struct fields that are function pointers
// with names that match the libcalls we are wrapping.
//
#define _PTHREAD_FIX_LIBCALL_CANCELCHECK(name, ...)                            \
  ({                                                                           \
    pthread_testcancel();                                                      \
    _pthread_fix_set_running(false);                                           \
    __auto_type ret = name(__VA_ARGS__);                                       \
    _pthread_fix_set_running(true);                                            \
    ret;                                                                       \
  })

#define accept(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(accept, __VA_ARGS__)
#define aio_suspend(...)                                                       \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(aio_suspend, __VA_ARGS__)
#define clock_nanosleep(...)                                                   \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(clock_nanosleep, __VA_ARGS__)
#define close(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(close, __VA_ARGS__)
#define connect(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(connect, __VA_ARGS__)
#define creat(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(creat, __VA_ARGS__)
#define fcntl(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(fcntl, __VA_ARGS__)
#define fdatasync(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(fdatasync, __VA_ARGS__)
#define fsync(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(fsync, __VA_ARGS__)
#define getmsg(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(getmsg, __VA_ARGS__)
#define getpmsg(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(getpmsg, __VA_ARGS__)
#define lockf(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(lockf, __VA_ARGS__)
#define mq_receive(...)                                                        \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(mq_receive, __VA_ARGS__)
#define mq_send(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(mq_send, __VA_ARGS__)
#define mq_timedreceive(...)                                                   \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(mq_timedreceive, __VA_ARGS__)
#define mq_timedsend(...)                                                      \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(mq_timedsend, __VA_ARGS__)
#define msgrcv(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(msgrcv, __VA_ARGS__)
#define msgsnd(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(msgsnd, __VA_ARGS__)
#define msync(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(msync, __VA_ARGS__)
#define nanosleep(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(nanosleep, __VA_ARGS__)
#define open(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(open, __VA_ARGS__)
#define openat(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(openat, __VA_ARGS__)
#define pause(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(pause, __VA_ARGS__)
#define poll(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(poll, __VA_ARGS__)
#define pread(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(pread, __VA_ARGS__)
#define pselect(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(pselect, __VA_ARGS__)
#define pthread_cond_timedwait(...)                                            \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(pthread_cond_timedwait, __VA_ARGS__)
#define pthread_cond_wait(...)                                                 \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(pthread_cond_wait, __VA_ARGS__)
#define pthread_join(...)                                                      \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(pthread_join, __VA_ARGS__)
#define putmsg(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(putmsg, __VA_ARGS__)
#define putpmsg(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(putpmsg, __VA_ARGS__)
#define pwrite(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(pwrite, __VA_ARGS__)
#define read(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(read, __VA_ARGS__)
#define readv(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(readv, __VA_ARGS__)
#define recv(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(recv, __VA_ARGS__)
#define recvfrom(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(recvfrom, __VA_ARGS__)
#define recvmsg(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(recvmsg, __VA_ARGS__)
#define select(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(select, __VA_ARGS__)
#define sem_timedwait(...)                                                     \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(sem_timedwait, __VA_ARGS__)
#define sem_wait(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(sem_wait, __VA_ARGS__)
#define send(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(send, __VA_ARGS__)
#define sendmsg(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(sendmsg, __VA_ARGS__)
#define sendto(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(sendto, __VA_ARGS__)
#define sigpause(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(sigpause, __VA_ARGS__)
#define sigsuspend(...)                                                        \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(sigsuspend, __VA_ARGS__)
#define sigtimedwait(...)                                                      \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(sigtimedwait, __VA_ARGS__)
#define sigwait(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(sigwait, __VA_ARGS__)
#define sigwaitinfo(...)                                                       \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(sigwaitinfo, __VA_ARGS__)
#define sleep(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(sleep, __VA_ARGS__)
#define system(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(system, __VA_ARGS__)
#define tcdrain(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(tcdrain, __VA_ARGS__)
#define usleep(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(usleep, __VA_ARGS__)
#define wait(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(wait, __VA_ARGS__)
#define waitid(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(waitid, __VA_ARGS__)
#define waitpid(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(waitpid, __VA_ARGS__)
#define write(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(write, __VA_ARGS__)
#define writev(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(writev, __VA_ARGS__)

// Linux specific
#define accept4(...) _PTHREAD_FIX_LIBCALL_CANCELCHECK(accept4, __VA_ARGS__)
#define epoll_pwait(...)                                                       \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(epoll_pwait, __VA_ARGS__)
#define epoll_wait(...)                                                        \
  _PTHREAD_FIX_LIBCALL_CANCELCHECK(epoll_wait, __VA_ARGS__)
// What else?

#ifndef __PTHREAD_FIX_NO_SHADOW__
// Wrapper for pthread_create which sets up the cancel info
// structure and calls the real pthread_create
#define pthread_create(thread, attr, start_routine, arg)                       \
  _pthread_fix_create(thread, attr, start_routine, arg)

// We also wrap pthread_exit to make sure we clean up the cancel info
#define pthread_exit(retval) _pthread_fix_exit(retval)
#endif // __PTHREAD_FIX_NOSHADOW__

// Missing prototype in bionic but it appears to exist
void pthread_getname_np(pthread_t thread, char *name, size_t len);

// These are not implemented in bionic, we implement them
int pthread_cancel(pthread_t thread);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);
void pthread_testcancel(void);
#endif // __PTHREAD_FIX_H__
#endif // __ANDROID__
