#ifndef GENTHREADPOOL_H
#define GENTHREADPOOL_H


#include <iostream>
#include <functional>
#include <thread>
#include <condition_variable>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>

template <typename T>
class GenBlockQueue
{
public:
    explicit GenBlockQueue(bool cancel = false, int timeout = 1000):
        m_cancel(cancel), m_timeout(timeout){}

    void push(const T &value)
    {
        {
            std::lock_guard<std::mutex> lock(m_lock);
            m_queue.push(value);
        }
        m_condi.notify_one();
    }

//    void push(T&& value)
//    {  // 添加右值引用版本
//        {
//            std::lock_guard<std::mutex> lock(m_lock);
//            m_queue.push(value);
//        }
//        m_condi.notify_one();
//    }

    bool pop(T& value)
    {
        std::unique_lock<std::mutex> lock(m_lock);
        m_condi.wait_for(lock, m_timeout, [this](){
            return !m_queue.empty() || m_cancel;
        });
        if(m_queue.empty()){
            return false; // cancel
        }
        value = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    void cancel()
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_cancel = true;
        m_condi.notify_all();
    }

private:
    bool m_cancel; // true: cancel
    std::queue<T> m_queue;
    std::mutex m_lock;
    std::condition_variable m_condi;
    int m_timeout;
};

template <typename T>
class GenBlockDoubleQueue
{
public:
    explicit GenBlockDoubleQueue(bool cancel = false, int timeout = 1000):
        m_cancel(cancel), m_timeout(timeout){}

    void push(const T &value)
    {
        {
            std::lock_guard<std::mutex> lock(m_produceLock);
            m_produceQueue.push(value);
        }
        m_condi.notify_one();
    }

    bool pop(T& value)
    {
        std::unique_lock<std::mutex> lock(m_consumeLock);
        if(m_consumeQueue.empty() && swapQueue() == 0){
            return false;
        }

        value = std::move(m_consumeQueue.front());
        m_consumeQueue.pop();
        return true;
    }

    // std::atomic<bool>取消cancel的加锁,避免死锁
    void cancel()
    {
        m_cancel.store(true);
        m_condi.notify_all();
    }

private:
    int swapQueue()
    {
        std::unique_lock<std::mutex> lock(m_produceLock);
        m_condi.wait_for(lock, m_timeout, [this](){
            return !m_produceQueue.empty() || m_cancel;
        });
        if (m_produceQueue.empty()) return 0;  // 超时或取消
        std::swap(m_produceQueue, m_consumeQueue);
        return m_consumeQueue.size();
    }

//    bool m_cancel;
    std::atomic<bool> m_cancel; // true: cancel
    std::queue<T> m_produceQueue;
    std::queue<T> m_consumeQueue;
    std::mutex m_produceLock;
    std::mutex m_consumeLock;
    std::condition_variable m_condi;
    int m_timeout;
};

class GenThreadPool
{
public:
    explicit GenThreadPool(int num_threads);
    ~GenThreadPool();

    /***
     * && 保持参数的左值/右值属性
     * 左值:有持久身份的内存位置	右值:可移动的临时对象或即将销毁的值
     * std::forward 完美转发参数原始类型（避免不必要的拷贝）
     * std::forward<Args>(args)... 等价于 std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), ...
     ***/
    template<typename F, typename... Args>
    void post(F &&f, Args &&...args)
    {
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        m_taskQueue.push(task);
    }

private:
    void work();

//    GenBlockQueue<std::function<void ()>> m_taskQueue;
    GenBlockDoubleQueue<std::function<void ()>> m_taskQueue;
    std::vector<std::thread> m_workers;

};

#endif // GENTHREADPOOL_H
