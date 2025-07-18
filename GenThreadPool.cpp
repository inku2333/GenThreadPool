  #include "GenThreadPool.h"

GenThreadPool::GenThreadPool(int num_threads)
{
    for(size_t i = 0; i < num_threads; ++i){
        m_workers.emplace_back([this]()->void{work();});
    }
}

GenThreadPool::~GenThreadPool()
{
    m_taskQueue.cancel();
    for(auto &work: m_workers){
        if(work.joinable()){
            work.join();
        }
    }
}

void GenThreadPool::work()
{
    while(true){
        std::function<void ()> task;
        if(!m_taskQueue.pop(task)){
            break; // cancel
        }
        task();
    }
}
