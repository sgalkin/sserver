#ifndef SSERVER_POOL_H_INCLUDED
#define SSERVER_POOL_H_INCLUDED

#include "fd.h"
#include "code.h"
#include "log.h"
#include <boost/thread/thread.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/bind.hpp>
#include <algorithm>
#include <queue>

static const unsigned MIN_THREADS = 4;

template<typename Task, typename Handler>
class Pool {
    typedef std::queue<Task> Tasks;

public:
    Pool() : running_(true) {
        create_threads(std::max(boost::thread::hardware_concurrency(), MIN_THREADS));
    }

    explicit Pool(int thread_num) : running_(true) {
        create_threads(thread_num);
    }

    ~Pool() {
        running_ = false; // is race posible?
        std::for_each(pipes_.begin(), pipes_.end(), boost::bind(&notify<Pipe>, _1, OK));
        pool_.join_all();
    }

    void add_task(const Task& task) {
        static unsigned thread_id = 0;
        {
            boost::mutex::scoped_lock lock(queue_mutex_);
            queue_.push(task);
        }
        notify(pipes_[thread_id++ % pipes_.size()]); // called from only thread
    }

private:
    void create_threads(int thread_num) {
        DEBUG("Creating thread pool with " << thread_num << " threads");
        for(int i = 0; i < thread_num; ++i) {
            pipes_.push_back(new Pipe);
            pool_.create_thread(boost::bind(&Pool::work, this, &pipes_.back()));
        }
    }

    void work(Pipe* pipe) {
        DEBUG("Working thread started");
        do {
            Handler handler(pipe);
            do {
                {
                    boost::mutex::scoped_lock lock(queue_mutex_);
                    if(!queue_.empty()) {
                        handler.add_task(queue_.front());
                        queue_.pop();
                    }
                }
                try {
                    handler.perform();
                } catch(std::runtime_error& ex) {
                    ERROR("Error in working thread `" 
                          << ex.what() << "' restarting handler...");
                    break;
                } catch(...) {
                    ERROR("Unexpected error in working thread restarting handler... ");
                    break;
                }
            } while(running_);
        } while(running_);
        DEBUG("Working thread stopped");
    }

    boost::thread_group pool_;
    boost::ptr_vector<Pipe> pipes_;

    boost::mutex queue_mutex_;
    Tasks queue_;
    bool running_;
};

#endif // SSERVER_POOL_H_INCLUDED
