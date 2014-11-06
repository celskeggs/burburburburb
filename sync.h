#include <pthread.h>

#define MUTEX_DEF(x) pthread_mutex_t x;
#define MUTEX_INIT(x) pthread_mutex_init(&x);
#define MUTEX_DESTROY(x) pthread_mutex_destroy(&x);
#define MUTEX_LOCK(x) pthread_mutex_lock(&x);
#define MUTEX_UNLOCK(x) pthread_mutex_unlock(&x);
