#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <QThread>
#include <QSemaphore>

namespace WingletUI {

template <typename T> class WorkerThread : public QThread
{
public:
    WorkerThread(): initLock(0) {}

    T *getWorkerObj() {
        // Forces this to wait until the thread creates the object
        initLock.acquire(1);
        return workerObj;
    }

protected:
    virtual void run() override {
        workerObj = new T(this);
        initLock.release(1);
        (void) exec();
        delete workerObj;
    }
private:
    T *workerObj;
    QSemaphore initLock;  // Guards the initialized WifiMonitor object until the thread starts up
};

} // namespace WingletUI

#endif // WORKERTHREAD_H
