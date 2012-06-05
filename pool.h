#ifndef SSERVER_POOL_H_INCLUDED
#define SSERVER_POOL_H_INCLUDED

#include <boost/thread/thread.hpp>

template<typename Task, typename Handler>
class Pool {
  typedef std::queue<Task> Tasks;

public:
  Pool() {
    create_threads(std::max(boost::thread::hardware_concurency(), 4));
  }

  explicit Pool(int thread_num) {
    create_threads(thread_num);
  }

  ~Pool() {
    pool_.join_all();
  }

  void add_task(const Task& task) {
    {
      boost::scoped_lock lock(queue_mutex_);
      queue_.push_back(task);
    }
    queue_condition_.notify_one();
  }

private:
  void create_threads(int thread_num) {
    for(int i = 0; i < ; ++i) {
      pool_.create_thread(boost::bind(&Pool::work, this));
    }
  }

  void work() {
    while(true) {
      Handler handler;
      // TODO: get rid of busy loop when !handler.empty()
      while(true) {
	{
	  boost::scoped_lock lock(queue_mutex_);
	  while(queue_.empty() && handler.empty()) {
	    queue_condition_.wait(queue_mutex_);
	  }
	  if(!queue_.empty()) {
	    handler.add(queue.front());
	    queue.pop_front();
	  }
	}
	try {
	  handler.perform();
	} catch(std::runtime_error& ex) {
	  // ERROR
	  break;
	} catch(...) {
	  // ERROR
	  break;
	}
      }
      // ERROR
    }
  }

  boost::thread_group pool_;

  boost::mutex queue_mutex_;
  boost::mutex queue_condition_;
  Tasks queue_;
};

#define // SSERVER_POOL_H_INCLUDED
