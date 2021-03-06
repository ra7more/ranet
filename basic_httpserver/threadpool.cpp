//
// Created by Jialei Wang on 2019/3/27.
//
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include "threadpool.h"

void *thread_pool_work(void *arg){
    ThreadPool *pool = static_cast<ThreadPool*>(arg);
    for(;;){
        if(pthread_mutex_lock(&pool->lock)< 0){
            perror("pthread_mutex_lock");
            abort();
        }
        while(pool->queue.empty()){
            if(pthread_cond_wait(&pool->notify, &pool->lock) <0){
                perror("pthread_cond_wait");
                abort();
            }
        }
        auto task = pool->queue.front();
        pool->queue.pop();
        if(pthread_mutex_unlock(&pool->lock) < 0){
            perror("pthread_mutex_unlock");
            abort();
        }
        (*task.first)(task.second);
    }
    return NULL;
}

ThreadPool::ThreadPool(size_t _worker_num):worker_num(_worker_num),threads(worker_num)
{
    threads.reserve(worker_num);
    if(pthread_mutex_init(&lock,NULL) < 0){
        perror("pthread_mutex_init");
        abort();
    }
    if(pthread_cond_init(&notify, NULL) < 0){
        perror("pthread_cond_init");
        abort();
    }
    for(size_t i=0; i < worker_num ; ++i){
        if(pthread_create(&threads[i], NULL, thread_pool_work, static_cast<void*>(this)) < 0){
            perror("pthread_create");
            abort();
        }
    }
}

void ThreadPool::add(void (*func)(void *), void *arg)
{
    if(pthread_mutex_lock(&lock) <0){
        perror("pthread_mutex_lock");
        abort();
    }
    queue.push(std::make_pair(func,arg));
    if(pthread_cond_signal(&notify) < 0) {
        perror("pthread_cond_signal");
        abort();
    }
    if(pthread_mutex_unlock(&lock) <0){
        perror("pthread_mutex_unlock");
        abort();
    }
}