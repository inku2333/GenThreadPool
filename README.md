2025.7.18 ：  使用atomic避免死锁，注释中保留了原本避免死锁的方式

2025.7.18 ：  删除注释，可以查看历史版本；优化代码，添加右值push（std::move）；添加boost::lockfree::queue实现的线程queue

后续优化：可以用boost::lockfree::queue（内置CAS实现）优化std::queue（已实现，没有深究CAS的原理了）

CAS的坑以后再填吧，核心就是compare_exchange_weak，可以实现无锁队列之类的，本质还是调用硬件层的锁；CAS主要处理 高频交易/游戏服务器 等超高并发（10^6 ops/s，10-50ns，低冲突时性能线性增长，高冲突时可能退化至‌万级操作/秒），普通锁的并发10^4–10^5 ops/s，100-200ns，也足够了；
