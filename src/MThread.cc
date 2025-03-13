#include "MThread.h"
#include "log.h"

MThread::MThread()
{}

MThread::~MThread()
{
    if (!this->isStoped()) {
        this->stop();
    }

    if (this->th.joinable()) {
        this->th.join();
    }
}

void MThread::start()
{
    std::thread thr(&MThread::run, this);
    this->th = std::move(thr);
}

void MThread::stop()
{
    this->stopState = true;
    if (this->th.joinable()) {
        this->th.join();
    }
}

void MThread::join()
{
    this->th.join();
}
void MThread::detach()
{
    this->th.detach();
}

void MThread::sleep(int sec)
{
    std::this_thread::sleep_for(std::chrono::seconds(sec));
}

std::thread::id MThread::getId()
{
    return this->th.get_id();
}

bool MThread::isStoped()
{
    return this->stopState;
}
