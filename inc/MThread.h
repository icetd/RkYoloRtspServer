#ifndef THREAD_H
#define THREAD_H

#include <thread>
#include <chrono>
#include <atomic>

class MThread
{
public:
    MThread();
    virtual ~MThread();

    std::thread::id getId();

    void start();
    void detach();
    void stop();
    void join();
    void sleep(int sec);
    bool isStoped();

    virtual void run() = 0;

private:
    std::atomic<bool> stopState;
    std::thread th;
};

#endif
