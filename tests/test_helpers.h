#ifndef SP_TESTS_TEST_HELPERS_H_
#define SP_TESTS_TEST_HELPERS_H_

#include <string>

class subject {
 public:
  subject() : id_("default"), leak_(new int()) {}
  explicit subject(const std::string& name)
      : id_(name), leak_(new int()) {}
  subject(subject&& other) noexcept {
    id_ = std::move(other.id_);
    leak_ = other.leak_;
    other.leak_ = nullptr;
  }
  subject(const subject& other)
      : id_(other.id_), leak_(new int()) {}
  subject& operator=(const subject& other) {
    id_ = other.id_;
    return *this;
  }
  subject& operator=(subject&& other) noexcept {
    id_ = std::move(other.id_);
    return *this;
  }
  virtual ~subject() { delete leak_; }

  bool operator==(const subject& other) const { return other.id_ == id_; }
  bool operator!=(const subject& other) const { return other.id_ != id_; }

  friend std::ostream& operator<<(std::ostream& os, const subject& obj) {
    os << obj.id_;
    return os;
  }

 protected:
  std::string id_;
  int* leak_;
};

class large {
 public:
  large() : id_("default"), leak_(new int()) {}
  explicit large(const std::string& name)
      : id_(name), leak_(new int()) {}
  large(large&& other) noexcept {
    id_ = std::move(other.id_);
    leak_ = other.leak_;
    other.leak_ = nullptr;
  }
  large(const large& other)
      : id_(other.id_), leak_(new int()) {}
  large& operator=(const large& other) {
    id_ = other.id_;
    return *this;
  }
  large& operator=(large&& other) noexcept {
    id_ = std::move(other.id_);
    return *this;
  }
  virtual ~large() { delete leak_; }

  bool operator==(const large& other) const { return other.id_ == id_; }
  bool operator!=(const large& other) const { return other.id_ != id_; }

  friend std::ostream& operator<<(std::ostream& os, const large& obj) {
    os << obj.id_;
    return os;
  }

 protected:
  std::string id_;
  int* leak_;
  int some_ = 0;
  int fields_ = 0;
  int to_ = 0;
  int make_ = 0;
  int class_ = 0;
  int big_ = 0;
};

#endif  // SP_TESTS_TEST_HELPERS_H_
