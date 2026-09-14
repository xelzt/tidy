#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

#define COUNTER 100
#define POISON -1

struct
{
    std::queue<int> q;
    std::mutex m;
    std::condition_variable cv;
}threadData;

void consumer()
{
    while (true) {
        std::unique_lock<std::mutex> lock(threadData.m);
        threadData.cv.wait(lock, [&] {return !threadData.q.empty();});
        
        int element = threadData.q.front();
        threadData.q.pop();
        
        if(element == -1)
        {
            break;
        }

        std::cout << "element: " << element << " data\n";
    }
}

void producer()
{
    for(int i = 0; i < 20; i++) {
        {
            std::lock_guard<std::mutex> lock(threadData.m);
            threadData.q.push(i);
            std::cout << "Pushing: " << i << "\n";
        }
        threadData.cv.notify_one();
    }
    {
        std::lock_guard<std::mutex> lock(threadData.m);
        threadData.q.push(POISON);
    }

    threadData.cv.notify_one();
}



int main()
{
    std::jthread tw(consumer);
    std::jthread tp(producer);
    
    return 0;
}