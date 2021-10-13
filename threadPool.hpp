#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include "taskQueue.hpp"
#include <iostream>
#include <string>
#include <string.h>
#include <unistd.h>

template <typename T>
class ThreadPool {
    public:
        ThreadPool(int min, int max);
        ~ThreadPool();
        void addTask(Task<T> task); // 添加任务
        int getBusyNum();           // 获取忙活的线程个数
        int getAliveNum();          // 获取存活的线程个数
    private:
        static void* worker(void* arg);     // 工作线程的任务函数
        static void* manager(void* arg);    // 管理者线程的任务函数
        void threadExit();
    private:
        TaskQueue<T>* m_taskQ;        // 任务队列
        pthread_t m_managerID;        // 管理者线程ID
        pthread_t *m_threadID;        // 工作的线程ID
        int m_minNum;                 // 最小线程数量
        int m_maxNum;                 // 最大线程数量
        int m_busyNum;                // 正在工作的线程个数
        int m_aliveNum;               // 存活的线程个数
        int m_exitNum;                // 要销毁的线程个数
        pthread_mutex_t m_lock;       // 锁整个线程池
        pthread_cond_t m_notEmpty;    // 任务队列是否已空
        static const int number = 2;
        bool m_shutdown = false;      // 是否要销毁线程池
};

template <typename T>
ThreadPool<T>::ThreadPool(int minNum, int maxNum) {
    // 实例化任务队列
    m_taskQ = new TaskQueue<T>;
    
    // 初始化线程池
    m_minNum = minNum;
    m_maxNum = maxNum;
    m_busyNum = 0;
    m_aliveNum = minNum;

    // 根据线程的最大上限给线程数组分配内存
    m_threadID = new pthread_t[maxNum];
    if (m_threadID == nullptr) {
        std::cout << "创建线程数组失败！" << std::endl;
        exit(-1);
    }

    // 初始化
    memset(m_threadID, 0, sizeof(m_threadID));

    // 初始化互斥锁、条件变量
    if (pthread_mutex_init(&m_lock, NULL) != 0 || pthread_cond_init(&m_notEmpty, NULL) != 0) {
        std::cout << "初始化互斥锁/条件变量失败！" << std::endl;
        exit(-1);
    }

    // 根据最小线程个数，创建线程
    for (int i = 0; i < minNum; i++) {
        // this指针指向当前被实例化出来的对象
        pthread_create(&m_threadID[i], NULL, worker, this);
        std::cout << "创建子线程，ID：" << std::to_string(m_threadID[i]) << std::endl;
    }

    // 创建管理者线程，1个
    pthread_create(&m_managerID, NULL, manager, this);
}

template <typename T>
ThreadPool<T>::~ThreadPool() {
    m_shutdown = true;
    // 阻塞回收管理者线程
    pthread_join(m_managerID, NULL);
    // 唤醒所有阻塞的消费者线程
    for (int i = 0; i < m_aliveNum; i++) {
        pthread_cond_signal(&m_notEmpty);
    }
    if (m_taskQ) delete m_taskQ;
    if (m_threadID) delete[] m_threadID;
    pthread_mutex_destroy(&m_lock);
    pthread_cond_destroy(&m_notEmpty);
}

template <typename T>
void ThreadPool<T>::addTask(Task<T> task) {
    if (m_shutdown) return;
    // 添加任务，不需要加锁，任务队列中有锁
    m_taskQ->addTask(task);
    // 唤醒工作线程
    pthread_cond_signal(&m_notEmpty);
}

template <typename T>
int ThreadPool<T>::getBusyNum() {
    pthread_mutex_lock(&m_lock);
    int busyNum = this->m_busyNum;
    pthread_mutex_unlock(&m_lock);
    return busyNum;
}

template <typename T>
int ThreadPool<T>::getAliveNum() {
    pthread_mutex_lock(&m_lock);
    int threadNum = this->m_aliveNum;
    pthread_mutex_unlock(&m_lock);
    return threadNum;
}

// 工作线程任务函数
template <typename T>
void* ThreadPool<T>::worker(void* arg) {
    ThreadPool *pool = static_cast<ThreadPool*>(arg);
    while (1) {
        // 访问任务队列（共享资源）加锁
        pthread_mutex_lock(&pool->m_lock);

        // 判断任务队列是否为空，若为空工作线程阻塞
        while (pool->m_taskQ->taskNum() == 0 && !pool->m_shutdown) {
            std::cout << "thread " << std::to_string(pthread_self()) << " waiting..." << std::endl;
            // 阻塞线程
            pthread_cond_wait(&pool->m_notEmpty, &pool->m_lock);

            // 判断是否要销毁线程
            if (pool->m_exitNum > 0) {
                pool->m_exitNum--;
                if (pool->m_aliveNum > pool->m_minNum) {
                    pool->m_aliveNum--;
                    pthread_mutex_unlock(&pool->m_lock);
                    pool->threadExit();
                }
            }
        }

        // 判断线程池是否被摧毁
        if (pool->m_shutdown) {
            pthread_mutex_unlock(&pool->m_lock);
            pool->threadExit();
        }

        // 从任务队列中取出一个任务
        Task<T> task = pool->m_taskQ->getTask();
        // 工作的线程+1
        pool->m_busyNum++;
        // 任务队列解锁
        pthread_mutex_unlock(&pool->m_lock);
        // 执行任务
        std::cout << "thread " << std::to_string(pthread_self()) << " start working..." << std::endl;
        task.function(task.arg1, task.arg2);
        // delete task.arg1;
        // delete task.arg2;
        task.arg1 = -1;
        task.arg2 = -1;

        // 任务处理结束
        std::cout << "thread " << std::to_string(pthread_self()) << " finish work..." << std::endl;
        pthread_mutex_lock(&pool->m_lock);
        pool->m_busyNum--;
        pthread_mutex_unlock(&pool->m_lock);
    }
    return nullptr;
}

// 管理者线程任务函数
template <typename T>
void* ThreadPool<T>::manager(void* arg) {
    ThreadPool *pool = static_cast<ThreadPool*>(arg);
    // 若线程池没有关闭，就一直检测
    while (!pool->m_shutdown) {
        // 每5秒检测一次
        sleep(5);
        // 取出线程池中的任务数和线程数量
        // 取出工作的线程池数量
        pthread_mutex_lock(&pool->m_lock);
        int queueSize = pool->m_taskQ->taskNum();
        int liveNum = pool->m_aliveNum;
        int busyNum = pool->m_busyNum;
        pthread_mutex_unlock(&pool->m_lock);

        // 创建线程
        // 当前任务个数 > 存活的线程数 && 存活的线程数 < 最大线程个数
        if (queueSize > liveNum && liveNum < pool->m_maxNum) {
            // 加锁
            pthread_mutex_lock(&pool->m_lock);
            int num = 0;
            for (int i = 0; i < pool->m_maxNum && num < number && pool->m_aliveNum < pool->m_maxNum; i++) {
                if (pool->m_threadID[i] == 0) {
                    pthread_create(&pool->m_threadID[i], NULL, worker, pool);
                    num++;
                    pool->m_aliveNum++;
                }
            }
            pthread_mutex_unlock(&pool->m_lock);
        }

        // 销毁多余的线程
        // 工作线程 * 2 < 存活的线程 && 存活的线程 > 最小线程数量
        if (busyNum * 2 < liveNum && liveNum > pool->m_minNum) {
            pthread_mutex_lock(&pool->m_lock);
            pool->m_exitNum = number;
            pthread_mutex_unlock(&pool->m_lock);
            for (int i = 0; i < number; i++) {
                pthread_cond_signal(&pool->m_notEmpty);
            }
        }
    }
    return nullptr;
}

// 线程退出
template <typename T>
void ThreadPool<T>::threadExit() {
    pthread_t tid = pthread_self();
    for (int i = 0; i < m_maxNum; i++) {
        if (m_threadID[i] == tid) {
            std::cout << "threadExit() function: thread " << std::to_string(tid) << " exiting..." << std::endl;
            m_threadID[i] = 0;
            break;
        }
    }
    pthread_exit(NULL);
}

#endif