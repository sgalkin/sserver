#include "test.h"
#include "../file.h"
#include "../file_io.h"
#include "../fd.h"
#include "../pool.h"
#include "../tempfile.h"
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <list>
#include <set>
#include <fstream>
#include <iterator>

class WriterTest {
public:
    WriterTest() : writer_(1) {}

    void add(const std::string& file, const Record& rec) {
        std::auto_ptr<Pipe> pipe(new Pipe);
        writer_.add_task(WriteTask(file, rec, pipe.get()));
        poll_.add(new Message<Own<Pipe>, Suppress>(pipe.release()));
    }

    void remove(MessageBase* msg) {
        poll_.remove(msg);
    }

    Poll& perform() {
        poll_.perform();
        return poll_;
    }    

private:
    Pool<WriteTask, Writer> writer_;
    Poll poll_;
};

BOOST_AUTO_TEST_CASE(test_file_not_exists) {
    WriterTest wt;
    wt.add("non_existent", Record("aaa3", "bbb3"));
    BOOST_FOREACH(MessageBase* msg, wt.perform()) {
        BOOST_CHECK(msg->is_fail());
        wt.remove(msg);
    }
}

BOOST_AUTO_TEST_CASE(test_data_in_file) {
    TempFile tmp("tests/foobar");
    std::ofstream data(tmp.name().c_str());
    data << "aaa3;bbb3\n";
    data.close();
    {
        WriterTest wt;
        wt.add(tmp.name(), Record("aaa3", "bbb"));
        BOOST_FOREACH(MessageBase* msg, wt.perform()) {
            BOOST_CHECK(msg->is_fail());
            wt.remove(msg);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_no_data_in_file) {
    TempFile tmp("tests/foobar");
    std::ofstream data(tmp.name().c_str());
    data << "aaa3;bbb3\n";
    data.close();
    
    WriterTest wt;
    wt.add(tmp.name(), Record("ccc3", "bbb"));
    BOOST_FOREACH(MessageBase* msg, wt.perform()) {
        BOOST_CHECK(!msg->is_fail());
        wt.remove(msg);
    }

    std::ifstream in(tmp.name().c_str());
    std::list<std::string> tst;
    const char* exp[] = { "aaa3;bbb3", "ccc3;bbb" };
    std::copy(std::istream_iterator<std::string>(in),
              std::istream_iterator<std::string>(),
              std::back_inserter(tst));
    BOOST_CHECK_EQUAL_COLLECTIONS(tst.begin(), tst.end(), exp, exp + 2);
}

static int num = 0;
void add_task(WriterTest& wt, const std::string& fname) {
    static boost::mutex mutex;
    for(int i = 0; i < 16; ++i) {
        std::string name;
        {
            boost::mutex::scoped_lock lock(mutex);
            name = boost::lexical_cast<std::string>(num++);
        }
        wt.add(fname, Record(name, name));
    }
}

BOOST_AUTO_TEST_CASE(test_concurent) {
    boost::thread_group threads;
    TempFile tmp("tests/foobar");    
    WriterTest wt;
    for(int i = 0; i < 16; ++i) {
        threads.create_thread(
            boost::bind(&add_task, boost::ref(wt), boost::cref(tmp.name())));
    }
    threads.join_all();
    int i = 0;
    do {
        BOOST_FOREACH(MessageBase* msg, wt.perform()) {
            ++i;
            BOOST_CHECK(!msg->is_fail());
            wt.remove(msg);
        }
    } while(i < 16 * 16);

    std::ifstream in(tmp.name().c_str());
    std::set<std::string> tst;
    std::copy(std::istream_iterator<std::string>(in),
              std::istream_iterator<std::string>(),
              std::inserter(tst, tst.end()));
    std::set<std::string> exp;
    for(int i = 0; i != 16 * 16; ++i) {
        std::stringstream s;
        s << i << ";" << i;
        exp.insert(s.str());
    }
    BOOST_CHECK_EQUAL_COLLECTIONS(tst.begin(), tst.end(), exp.begin(), exp.end());
}

BOOST_AUTO_TEST_CASE( test_find_empty ) {
    Find<File> find("foo");
    File file("tests/empty");
    BOOST_CHECK(find.perform(&file));
    BOOST_CHECK(!find.data());
    BOOST_CHECK(find.is_eof());
}

BOOST_AUTO_TEST_CASE( test_find_not_found ) {
    Find<File> find("foo");
    File file("tests/test.dat");
    BOOST_CHECK(find.perform(&file));
    BOOST_CHECK(!find.data());
    BOOST_CHECK(find.perform(&file));
    BOOST_CHECK(find.is_eof());
}

BOOST_AUTO_TEST_CASE( test_find_found_short ) {
    Find<File> find("test_bar");
    File file("tests/test.dat");
    BOOST_CHECK(find.perform(&file));
    BOOST_CHECK_EQUAL(*find.data(), "data_test_bar");
}

BOOST_AUTO_TEST_CASE( test_find_found_short_2 ) {
    Find<File> find("test_baz");
    File file("tests/test.dat");
    BOOST_CHECK(find.perform(&file));
    BOOST_CHECK_EQUAL(*find.data(), "data_test_baz");
}

BOOST_AUTO_TEST_CASE( test_find_found_long ) {
    char buf[98306];
    memset(buf, 'a', sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    Find<File> find(buf);
    File file("tests/test.dat");
    BOOST_CHECK(find.perform(&file));
    BOOST_CHECK_EQUAL(*find.data(), "foo");
}
