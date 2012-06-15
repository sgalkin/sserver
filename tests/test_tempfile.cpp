#include "test.h"
#include "tempfile.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class StartsWith {
public:
    explicit StartsWith(const std::string& prefix) : prefix_(prefix) {}
    bool operator()(const fs::path& path) const {
        return path.string().find(prefix_) == 0;
    }
private:
    std::string prefix_;
};

BOOST_AUTO_TEST_CASE(test_autoclean) {
    std::string prefix = "aaaa";
    BOOST_REQUIRE(find_if(fs::directory_iterator(fs::current_path()),
                          fs::directory_iterator(),
                          StartsWith(prefix)) == fs::directory_iterator());
    {
        TempFile tf(prefix);
        BOOST_CHECK(fs::exists(tf.name()));
    }
    BOOST_CHECK(find_if(fs::directory_iterator(fs::current_path()),
                        fs::directory_iterator(),
                        StartsWith(prefix)) == fs::directory_iterator());
}
