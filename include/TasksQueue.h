#pragma once

#include "Task.h"

// Linked list implementation of a Queue
class TasksQueue {
private:
  struct Node {
    Task * data;
    struct Node * next;
  };

  Node * m_first,
       * m_last;

  int m_size;
public:
  TasksQueue() {
    m_first = nullptr;
    m_last = nullptr;
    m_size = 0;
  }

  ~TasksQueue() {
    for (int i = 0; i < size(); ++i) {
      dequeue();
    }
  }

  bool is_empty() const {
    return m_size == 0;
  }
  
  int size() const {
    return m_size;
  }
  
  void enqueue(Task& task) {
    Node * new_node = new Node();
    new_node->data = &task;
    new_node->next = nullptr;

    if (is_empty()) {
      m_first = new_node;
      m_last = new_node;
    }
    else {
      m_last->next = new_node;
      m_last = new_node;
    }

    ++m_size;
  }
  
  Task* dequeue() {
    if (is_empty()) {
      return nullptr;
    }

    Task * data = m_first->data;
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