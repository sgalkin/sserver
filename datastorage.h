#ifndef SSERVER_DATASTORAGE_H_INCLUDED
#define SSERVER_DATASTORAGE_H_INCLUDED

class DataStorage {
public:
  explicit DataStorage(const std::string& filename) : filename_(filename) {}

  void add(const std::string& name, const std::string& email) {
  }
  std::string find(const std::string& name) {
    int fd;
    CHECK_CALL((fd = open(filename_.c_str(), O_RDONLY | O_NONBLOCK)), "open");
    
  }

private:
  std::string& filename_;
};

#endif // SSERVER_DATASTORAGE_H_INCLUDED
