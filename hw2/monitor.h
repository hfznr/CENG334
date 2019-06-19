#ifndef __MONITOR_H
#define __MONITOR_H
#include<pthread.h>

class Monitor {
    pthread_mutex_t  mut;   // this will protect the monitor
public:
    Monitor() {
        pthread_mutex_init(&mut, NULL);
    }
    class Condition {
        Monitor *owner;
        pthread_cond_t cond;
    public:
        Condition(Monitor *o) {     // we need monitor ptr to access the mutex
                owner = o;
                pthread_cond_init(&cond, NULL) ; 
        }
        void wait() {  pthread_cond_wait(&cond, &owner->mut);}
        int timedwait(){
          struct timespec timewait;
          clock_gettime(CLOCK_REALTIME, &timewait);
          timewait.tv_sec += 5;
          return pthread_cond_timedwait(&cond, &owner->mut,&timewait);
        }
        void notify() { pthread_cond_signal(&cond);}
        void notifyAll() { pthread_cond_broadcast(&cond);}
    };
    class Lock {
        Monitor *owner;
    public:
        Lock(Monitor *o) { // we need monitor ptr to access the mutex
            owner = o;
            pthread_mutex_lock(&owner->mut); // lock on creation
        }
        ~Lock() { 
            pthread_mutex_unlock(&owner->mut); // unlock on destruct
        }
        void lock() { pthread_mutex_lock(&owner->mut);}
        void unlock() { pthread_mutex_unlock(&owner->mut);}
    };
};


#define __synchronized__ Lock mutex(this);

#endif