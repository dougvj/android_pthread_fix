# android_pthread_fix

Missing pthread functionality on android with bionic, namely cancellation semantics.

Include the `pthread_fix.c` and `pthread_fix.h` in your project. This provides
the following function calls:

- `pthread_getname_np` (Exists in bionic but prototype missing)
- `pthread_cancel`
- `pthread_setcancelstate`
- `pthread_setcanceltype`
- `pthread_testcancel`

## Limitations

- Requires managing a separate table of thread information, which right now is
  hardcoded to size 512

- Co-opts the use of SIGUSR1

- Cancellation points are implemented with Macros, which means that if you have
  something like a function pointer struct field named `send` it will break

## Acknowledgements

Work sponsored by [Subsurface Insights](https://subsurfaceinsights.com/)
