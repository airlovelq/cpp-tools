#ifndef LOCK_FREE_QUEUE_H_
#define LOCK_FREE_QUEUE_H_

#include <atomic>

template <typename T>
struct QueueItem {
  T data;
  std::atomic<QueueItem<T>*> next;
};

template <typename T>
class LockFreeQueue {
 public:
  template <typename M = T>
  LockFreeQueue(typename std::enable_if<!std::is_pointer<M>::value>::type* = 0)
    : length_(0) {
    QueueItem<T>* base_item = new QueueItem<T>;
    base_item->data = T();
    base_item->next = nullptr;
    head_.store(base_item);
    tail_.store(base_item);
  }

  template <typename M = T>
  LockFreeQueue(typename std::enable_if<std::is_pointer<M>::value>::type* = 0)
    : length_(0) {
    // construct a null head and tail item
    QueueItem<T>* base_item = new QueueItem<T>;
    base_item->data = nullptr;
    base_item->next = nullptr;
    head_.store(base_item);
    tail_.store(base_item);
  }

  ~LockFreeQueue() {
    QueueItem<T>* cur = head_;
    QueueItem<T>* next = nullptr;
    while (cur != nullptr) {
      next = cur->next;
      delete cur;
      cur = next;
    }
  }

 private:
  LockFreeQueue(const LockFreeQueue&) = delete;
  LockFreeQueue(LockFreeQueue&&) = delete;
  LockFreeQueue& operator=(LockFreeQueue&&) = delete;
  LockFreeQueue& operator=(const LockFreeQueue&) = delete;

 public:
  int Push(T value) {
    QueueItem<T>* new_item = new QueueItem<T>;
    new_item->data = value;
    new_item->next = nullptr;

    QueueItem<T>* old_tail = nullptr;
    QueueItem<T>* null_node = nullptr;
    // put new item into tail's next, if failed for concurrent, retry
    do {
      old_tail = tail_.load();
    } while (!old_tail->next.compare_exchange_weak(null_node, new_item)); // this line is for thread sync

    tail_.compare_exchange_weak(old_tail, new_item);
    length_.fetch_add(1);
    return 0;
  }

  template <typename M = T>
  typename std::enable_if<std::is_pointer<M>::value, M>::type Pop(bool& get) {
    QueueItem<T>* old_head = nullptr;
    QueueItem<T>* next = nullptr;
    T res = nullptr;
    do {
      old_head = head_.load();
      assert(old_head);
      next = old_head->next.load();
      if (!next){
        get = false;
        return nullptr;
      } else {
        res = next->data;
      }
    } while (!head_.compare_exchange_weak(old_head, next));
    length_.fetch_sub(1);
    delete old_head;
    get = true;
    return res;
  }

  template <typename M = T>
  typename std::enable_if<!std::is_pointer<M>::value, M>::type Pop(bool& get) {
    QueueItem<T>* old_head = nullptr;
    QueueItem<T>* next = nullptr;
    T res = T();
    do {
      old_head = head_.load();
      assert(old_head);
      next = old_head->next.load();
      if (!next){
        get = false;
        return T();
      } else {
        res = next->data;
      }
    } while (!head_.compare_exchange_weak(old_head, next));
    length_.fetch_sub(1);
    delete old_head;
    get = true;
    return res;
  }

  uint32_t length() const { return length_; }
  bool empty() const { return length_ == 0; }

 private:
  std::atomic<QueueItem<T>*> head_;  // item ptr ahead of first item, read head->next value in Pop
  std::atomic<QueueItem<T>*> tail_;
  std::atomic_uint32_t length_;
};

#endif
