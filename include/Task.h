#pragma once

// Inherit from this class and override execute() method to create custom parallel tasks
class Task {
public:
  Task() {};
  
  virtual void execute() = 0;
};