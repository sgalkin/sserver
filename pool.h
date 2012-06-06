#ifndef SSERVER_POOL_H_INCLUDED
#define SSERVER_POOL_H_INCLUDED

#include "fd.h"
#include "log.h"
#include <boost/thread/thread.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <queue>

template<typename Task, typename Handler>
class Pool {
    typedef std::queue<Task> Tasks;

public:
    Pool() {
        create_threads(std::max(boost::thread::hardware_concurrency(), 4u));
    }

    explicit Pool(int thread_num) {
        create_threads(thread_num);
    }

    ~Pool() {
        // TODO: notify all
        // TODO: set stop flag
        pool_.join_all();
    }

    void join() {
        // TODO: implement it
    }

    void add_task(const Task& task) {
        {
            boost::mutex::scoped_lock lock(queue_mutex_);
            queue_.push(task);
        }
        // TODO: notify random it does not matter who get the job
    }

private:
    void create_threads(int thread_num) {
        DEBUG("Creating thread pool with " << thread_num << " threads");
        for(int i = 0; i < thread_num; ++i) {
            pipes_.push_back(new Pipe);
            pool_.create_thread(
                boost::bind(&Pool::work, this, boost::ref(pipes_.back())));
        }
    }

    void work(Pipe& pipe) {
        DEBUG("Starting working thread: " << boost::this_thread::get_id());
        while(true) {
            Handler handler;
            while(true) {
                {
                    boost::mutex::scoped_lock lock(queue_mutex_);
                    if(!queue_.empty()) {
                        handler.add(queue_.front());
                        queue_.pop();
                    }
                }
                try {
                    handler.perform(pipe);
                    // TODO check stop flag
                } catch(std::runtime_error& ex) {
                    ERROR("Error in working thread: "
                          << boost::this_thread::get_id() << " "
                          << ex.what());
                    break;
                } catch(...) {
                    ERROR("Unexpected error in working thread: "
                          << boost::this_thread::get_id());
                    break;
                }
            }
            DEBUG("Restarging handler in thread: " << boost::this_thread::get_id());
        }
    }

    boost::thread_group pool_;
    boost::ptr_vector<Pipe> pipes_;

    boost::mutex queue_mutex_;
    Tasks queue_;
};

#endif // SSERVER_POOL_H_INCLUDED
