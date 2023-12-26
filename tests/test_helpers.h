#ifndef SP_TESTS_TEST_HELPERS_H_
#define SP_TESTS_TEST_HELPERS_H_

#include <string>

class safe {
 public:
  safe() : id_("default"), leak_(new int()) {}
  explicit safe(const std::string& name)
      : id_(name), leak_(new int()) {}
  safe(safe&& other) noexcept {
    id_ = std::move(other.id_);
    leak_ = other.leak_;
    other.leak_ = nullptr;
  }
  safe(const safe& other)
      : id_(other.id_), leak_(new int()) {}
  safe& operator=(const safe& other) {
    id_ = other.id_;
    return *this;
  }
  safe& operator=(safe&& other) noexcept {
    id_ = std::move(other.id_);
    return *this;
  }
  virtual ~safe() { delete leak_; }

  bool operator==(const safe& other) const { return other.id_ == id_; }
  bool operator!=(const safe& other) const { return other.id_ != id_; }

  friend std::ostream& operator<<(std::ostream& os, const safe& obj) {
    os << obj.id_;
    return os;
  }

 protected:
  std::string id_;
  int* leak_;
};

#endif  // SP_TESTS_TEST_HELPERS_H_
