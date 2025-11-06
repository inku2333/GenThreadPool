#include <iostream>
#include <atomic>
#include <chrono>
#include "GenThreadPool.h"  // 确保线程池头文件路径正确
#include <mutex>  // 包含互斥锁头文件
#if defined(WIN64) || defined(WIN32)  // 正确：检查WIN64或WIN32是否有一个被定义
#include <windows.h>
#define SYSNUM 1
#else
#define SYSNUM 0
#endif

using namespace std;
using namespace chrono;

int main()
{
    std::mutex cout_mutex;
    cout << "=== 线程池测试开始 ===" << endl;
    const int THREAD_NUM = 4;    // 工作线程数
    const int TASK_NUM = 15;     // 总任务数
    atomic<int> finish_count(0); // 线程安全计数，统计完成的任务数
    atomic<int> sum(0);          // 线程安全累加，验证线程安全

    // 1. 创建线程池（4个工作线程）
    GenThreadPool thread_pool(THREAD_NUM);

    // 2. 提交无参任务（打印线程ID）
    for (int i = 0; i < 5; ++i) {
        thread_pool.post([&, i]() {
            this_thread::sleep_for(milliseconds(100)); // 模拟任务耗时
            std::lock_guard<std::mutex> lock(cout_mutex);
            cout << "无参任务" << i << " | 线程ID: " << this_thread::get_id() << "===" << endl;
            finish_count++;
        });
    }

    // 3. 提交带参数任务（传递数字，累加求和）
    for (int i = 1; i <= 5; ++i) {
        thread_pool.post([&, i]() {
            this_thread::sleep_for(milliseconds(150));
            sum += i; // 原子变量确保线程安全
            std::lock_guard<std::mutex> lock(cout_mutex);
            cout << "带参任务 | 累加值: " << i << " | 线程ID: " << this_thread::get_id() << "===" << endl;
            finish_count++;
        });
    }

    // 4. 提交复杂任务（循环计算，验证并发稳定性）
    for (int i = 0; i < 5; ++i) {
        thread_pool.post([&, i]() {
            int result = 0;
            for (int j = 0; j < 100000; ++j) {
                result += j; // 模拟CPU密集型任务
            }
            std::lock_guard<std::mutex> lock(cout_mutex);
            cout << "复杂任务" << i << " | 计算结果: " << result << " | 线程ID: " << this_thread::get_id() << "===" << endl;
            finish_count++;
        });
    }

    // 5. 等待所有任务完成（避免main提前退出）
    while (finish_count < TASK_NUM) {
        this_thread::sleep_for(milliseconds(50)); // 非忙等，降低CPU占用
    }

    // 6. 输出测试结果
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        cout << "=== 所有任务执行完毕 ===" << endl;
        cout << "累加任务总结果（预期15）: " << sum << endl;
        cout << "实际完成任务数（预期15）: " << finish_count << endl;
    }


    return 0;
}
