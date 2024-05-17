#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

int shared_resource = 0;

#define NUM_ITERS 1000000
#define NUM_THREADS 100

volatile int mutex = 0;

void lock() {
    volatile int key = 1;
    for (; key != 0; ) {
        __asm__ __volatile__(
            "xchg %0, %1\n"
            : "+m" (mutex), "+r" (key)
        );
        if (key) sleep(0); // yield for speed up
    }
}

void unlock() {
    mutex = 0;
}

void* thread_func(void* arg) {
    int tid = *(int*)arg;
    
    lock();
    for (int i = 0; i < NUM_ITERS; ++i) ++shared_resource;
    unlock();
    // for (int i = 0; i < NUM_ITERS; ++i) {
    //     lock();
    //     ++shared_resource;
    //     unlock();
    // }
    
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    int tids[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        tids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &tids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("shared: %d\n", shared_resource);
    return 0;
}
