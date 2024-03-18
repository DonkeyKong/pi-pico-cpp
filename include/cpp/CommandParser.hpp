#pragma once 

#include "Math.hpp"

#include <iostream>
#include <istream>
#include <sstream>
#include <string>
#include <map>
#include <functional>

class CommandParser
{
private:
  using CommandFunc = std::function<bool(std::istream&)>;

  char inBuf[1024];
  int pos = 0;
  std::string lastCmd;
  std::map<std::string, CommandFunc> commands;

  template <typename T>
  static T parseArgFromStream(std::istream& ss)
  {
    T val;
    ss >> val;
    return val;
  }

  template <typename Ret, typename... Arg>
  void addCommandInternal(const std::string& name, std::function<Ret(Arg...)> cmdFunc)
  {
    commands[name] = [cmdFunc](std::istream& ss)
    {
      auto args = std::make_tuple(parseArgFromStream<Arg>(ss)...);
      if (ss.fail())
      {
        std::cout << "Argument parse error" << std::endl;
        return false;
      }
      if constexpr(std::is_same_v<Ret, bool>)
      {
        return std::apply(cmdFunc, args);
      }
      else
      {
        std::apply(cmdFunc, args);
        return true;
      }
    };
  }

public:

  CommandParser() = default;

  // Add a command with the given name, that executes a given
  // callable function. This can be a lambda, function pointer, 
  // or std::function. Note that all the arguments for the callable
  // function must be parsable from a stream using the >> operator.
  // The callable may return a bool to indicate success.
  // All other, or void, return types, are ignored and success is assumed.
  template <typename CallableType>
  void addCommand(const std::string& name, CallableType callable)
  {
    // Using std::function to explicitly convert performs template
    // deduction. Then passing to another function that takes a 
    // std::function with template args lets us get at them.
    addCommandInternal(name, std::function(callable));
  }

  void processStdIo()
  {
    while (true)
    {
      int inchar = getchar_timeout_us(0);
      if (inchar > 31 && inchar < 127 && pos < 1023)
      {
        inBuf[pos++] = (char)inchar;
        std::cout << (char)inchar << std::flush; // echo to client
      }
      else if (inchar == '\b' && pos > 0) // handle backspaces
      {
        --pos;
        std::cout << "\b \b" << std::flush;
      }
      else if (inchar == '\t' && lastCmd.size() < 1023) // handle tab to insert last command
      {
        while (pos-- > 0) std::cout << "\b \b" << std::flush;
        memcpy(inBuf, lastCmd.data(), lastCmd.size());
        pos = lastCmd.size();
        std::cout << lastCmd << std::flush;
      }
      else if (inchar == '\n')
      {
        inBuf[pos] = '\0';
        std::cout << std::endl; // echo to client
        lastCmd = inBuf;
        processCommand(inBuf);
        pos = 0;
      }
      else
      {
        return;
      }
    }
  }

  void processCommand(std::string cmdAndArgs)
  {
    std::stringstream ss(cmdAndArgs);
    std::string name;
    ss >> name;

    for (auto& [cmdName, cmdFunc] : commands)
    {
      if (name == cmdName)
      {
        if (cmdFunc(ss))
        {
          std::cout << "ok" << std::endl;
        }
        else
        {
          std::cout << "err" << std::endl;
        }
        return;
      }
    }
    std::cout << "unknown command error" << std::endl;
  }
};