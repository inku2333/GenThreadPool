#ifndef GENCASQUEUE_H
#define GENCASQUEUE_H

#include <boost/lockfree/queue.hpp> // compare-and-swap queue
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <queue>
#include <thread>

template <typename T>
class GenBlockDoubleQueue {
public:
    explicit GenBlockDoubleQueue(size_t capacity, bool cancel = false, int timeout = 1000)
        : m_produceQueue(capacity),
          m_cancel(cancel),
          m_timeout(timeout) {}

    // 支持左值引用的push
    void push(const T& value) {
        while (!m_produceQueue.push(value)) {
            // 队列满时等待消费者取走数据
            // 主动让出当前线程的CPU时间片，允许调度器选择其他线程运行‌
            // 适用于自旋锁或忙等待场景，减少CPU空转消耗‌
            // 不保证立即切换线程，具体行为依赖操作系统调度器‌
            std::this_thread::yield();
        }
        m_condi.notify_one();
    }

    // 支持右值引用的push
    void push(T&& value) {
        while (!m_produceQueue.push(std::move(value))) {
            std::this_thread::yield();
        }
        m_condi.notify_one();
    }

    bool pop(T& value) {
        // 1. 先尝试从消费队列直接获取数据（无锁）
        if (popFromConsumeQueue(value)) {
            return true;
        }

        // 2. 消费队列为空，尝试交换队列
        if (swapQueues()) {
            // 交换后消费队列有数据
            return popFromConsumeQueue(value);
        }

        // 3. 交换失败（超时或取消）
        return false;
    }

    void cancel() {
        m_cancel.store(true, std::memory_order_release);
        m_condi.notify_all();
    }

private:
    // 从消费队列弹出元素（需要消费锁保护）
    bool popFromConsumeQueue(T& value) {
        std::lock_guard<std::mutex> lock(m_consumeLock);
        if (m_consumeQueue.empty()) {
            return false;
        }
        value = std::move(m_consumeQueue.front());
        m_consumeQueue.pop();
        return true;
    }

    // 交换生产队列和消费队列
    bool swapQueues() {
        std::unique_lock<std::mutex> lock(m_swapLock);

        // 再次检查消费队列（可能在等待锁的过程中已有数据）
        if (!m_consumeQueue.empty()) {
            return true;
        }

        // 等待生产队列有数据或超时或取消
        if (m_condi.wait_for(lock, std::chrono::milliseconds(m_timeout), [this] {
                return !m_produceQueue.empty() || m_cancel.load(std::memory_order_acquire);
            }))
        {
            // 取消状态
            if (m_cancel.load(std::memory_order_acquire)) {
                return false;
            }

            // 交换队列：将生产队列数据转移到消费队列
            transferProduceToConsume();
            return true;
        }

        // 超时
        return false;
    }

    // 将生产队列数据转移到消费队列
    void transferProduceToConsume() {
        T value;
        std::lock_guard<std::mutex> consumeLock(m_consumeLock);

        // 从无锁队列批量取出数据到消费队列
        while (m_produceQueue.pop(value)) {
            m_consumeQueue.push(std::move(value));
        }
    }

private:
    // 原子取消标志
    std::atomic<bool> m_cancel;

    // 无锁生产队列（多生产者安全）
    boost::lockfree::queue<T> m_produceQueue;

    // 消费队列（需要锁保护）
    std::queue<T> m_consumeQueue;

    // 交换队列时的锁（保护队列交换过程）
    std::mutex m_swapLock;

    // 消费队列操作锁
    std::mutex m_consumeLock;

    // 条件变量（用于队列交换等待）
    std::condition_variable m_condi;

    // 超时时间（毫秒）
    std::chrono::milliseconds m_timeout;
};

#endif // GENCASQUEUE_H
