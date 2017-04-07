#pragma once

// Thread safe implementation of a Queue using a Singly Linked list
template <class T>
class ThreadSafeQueue {
private:
  struct Node {
    T data;
    struct Node * next;
  };

  Node * m_first,
       * m_last;

  int m_size;
public:
  ThreadSafeQueue() {
    m_first = nullptr;
    m_last = nullptr;
    m_size = 0;
  }

  ~ThreadSafeQueue() {
    for (int i = 0; i < size(); ++i) {
      dequeue();
    }
  }

  bool empty() const {
    return m_size == 0;
  }
  
  int size() const {
    return m_size;
  }
  
  void enqueue(T& t) {
    Node * new_node = new Node();
    new_node->data = t;
    new_node->next = nullptr;

    if (empty()) {
      m_first = new_node;
      m_last = new_node;
    }
    else {
      m_last->next = new_node;
      m_last = new_node;
    }

    ++m_size;
  }
  
  T dequeue() {
    if (empty()) {
      return nullptr;
    }

    T data = m_first->data;
    if (size() == 1) {
      free(m_first);
      m_first = nullptr;
      m_last = nullptr;
    }
    else {
      Node * new_first = m_first->next;
      free(m_first);
      m_first = new_first;
    }
    --m_size;
    return data;
  }


};