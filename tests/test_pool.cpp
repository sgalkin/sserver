#include "test.h"
#include "../pool.h"
#include "../fd.h"
#include <boost/thread.hpp>
#include <set>

class Sleeper {
public:
    static size_t threads_count() { return threads_.size(); }
    static size_t tasks_count() { return tasks_.size(); }

    void add_task(int id) {
        {
            boost::mutex::scoped_lock lock(mutex_);
            tasks_.insert(id);
        }
    }

    void perform(Pipe&) {
        {
            boost::mutex::scoped_lock lock(mutex_);
            threads_.insert(boost::this_thread::get_id());
        } 
        sleep(1); 
    }

private:
    static boost::mutex mutex_;
    static std::set<boost::thread::id> threads_;
    static std::set<int> tasks_;
};

boost::mutex Sleeper::mutex_;
std::set<boost::thread::id> Sleeper::threads_;
std::set<int> Sleeper::tasks_;


BOOST_AUTO_TEST_CASE( test_pool ) {
    set_level(Logger::DEBUG);
    size_t size = 16;
    time_t start = time(0);
    {
        Pool<int, Sleeper> p(size);
        for(size_t i = 0; i < 2 * size; ++i) {
            p.add_task(i);
        }
        sleep(3); // TODO: stop pool gently
    }
    BOOST_CHECK_EQUAL(Sleeper::threads_count(), size);
    BOOST_CHECK_EQUAL(Sleeper::tasks_count(), 2 * size);
    // TODO: make test more accurate
    BOOST_CHECK(time(0) - start < 5);
}
