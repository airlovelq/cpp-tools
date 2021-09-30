#ifndef SINGLETON_H_
#define SINGLETON_H_
#include <mutex>

template<typename T>
class Singleton {
 public:
  ~Singleton() = delete;

 private:
  Singleton() = delete;
  Singleton(const Singleton&) = delete;
  Singleton(Singleton&&) = delete;
  Singleton& operator=(const Singleton&) = delete;
  Singleton& operator=(Singleton&&) = delete;

 public:
  static T* GetInstance() {
    std::call_once(once_, &Singleton::Init);
    assert(value_ != nullptr);
    return value_;
  }

 private:
  static void Init() {
    value_ = new T();
  }

 public:
  static void Destroy() {
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    (void) sizeof(T_must_be_complete_type);

    delete value_;
  }

 private:
  static std::once_flag once_;
  static T* value_;
};

template<typename T>
std::once_flag Singleton<T>::once_;

template<typename T>
T* Singleton<T>::value_ = nullptr;

#endif
