#pragma once 

#include "Math.hpp"

#include <iostream>
#include <istream>
#include <sstream>
#include <string>
#include <cstring>
#include <map>
#include <functional>

// Pico SDK headers
#include <pico/stdlib.h>
#include <pico/stdio.h>

class CommandParser
{
private:
  using CommandFunc = std::function<bool(std::istream&)>;
  using GetterFunc = std::function<void(std::ostream&)>;

  struct Command
  {
    std::string args;
    std::string help;
    CommandFunc func;
  };

  struct Property
  {
    std::string help;
    GetterFunc get;
    CommandFunc set;
  };

  char inBuf[1024];
  int pos = 0;
  std::string lastCmd;
  std::map<std::string, Command> commands;
  std::map<std::string, Property> properties;
  bool echoOn = true;

  template <typename T>
  static T parseArgFromStream(std::istream& ss)
  {
    T val;
    ss >> val;
    return val;
  }

  template <typename Ret, typename... Arg>
  void addCommandInternal(const std::string& name, const std::string& argStr, const std::string& helpStr, std::function<Ret(Arg...)> cmdFunc)
  {
    commands[name] = 
    {
      argStr,
      helpStr,
      [cmdFunc](std::istream& ss)
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
      }
    };
  }

  template <typename Arg>
  static std::string getArgName()
  {
    if constexpr(std::is_integral_v<Arg>)
    {
      return "[int]";
    }
    else if constexpr(std::is_arithmetic_v<Arg>)
    {
      return "[num]";
    }
    else if constexpr(std::is_same_v<std::string>)
    {
      return "[str]";
    }
    else
    {
      return "[val]";
    }
  }

  template <typename Ret, typename... Arg>
  void addCommandInternal(const std::string& name, std::function<Ret(Arg...)> cmdFunc)
  {
    std::stringstream argString;
    ((argString << getArgName<Arg>() << " "),...);
    addCommandInternal(name, argString.str(), "", cmdFunc);
  }

public:

  CommandParser()
  {
    addCommand("help", "", "Print this help information", [this](){ printHelp(); });
    addCommand("echo", "", "Enable or disable comms echo", [this](bool enable){ echo(enable); });
  }

  void printHelp()
  {
    std::cout << std::endl;
    std::cout << "Command Listing:" << std::endl;
    std::cout << std::endl;
    for (const auto& [name, cmd] : commands)
    {
      int padding = std::max(1, (int)(32 - name.size() - cmd.args.size() - 5));
      std::cout << "    " << name << " " << cmd.args << std::string(padding, ' ')
                << (cmd.help.empty() ? "No help string provided" : cmd.help) << std::endl;
    }
    

    if (commands.count("set") == 0 && commands.count("get") == 0)
    {
      std::cout << "    get [property]              Get a property's value" << std::endl;
      std::cout << "    set [property] [value]      Set a property's value (if writable)" << std::endl;
      std::cout << std::endl;
      std::cout << "Properties:" << std::endl;
      std::cout << std::endl;
      for (const auto& [name, property] : properties)
      {
        int padding = std::max(1, (int)(32 - name.size() - 10 ));
        const char* access = (property.set ? "(r/w)" : "(r/o)");
        std::cout << "    " << name << " " << access << std::string(padding, ' ') << property.help << std::endl;
      }
    }
    std::cout << std::endl;
  }

  void printPropertyValues()
  {
    std::cout << std::endl;
    std::cout << "Properties:" << std::endl;
    std::cout << std::endl;
    for (const auto& [name, property] : properties)
    {
      std::cout << "    " << name << " ";
      property.get(std::cout);
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }

  void echo(bool enable)
  {
    echoOn = enable;
  }

  // Add a command with the given name, that executes a given
  // callable function. This can be a lambda, function pointer, 
  // or std::function. Note that all the arguments for the callable
  // function must be parsable from a stream using the >> operator.
  // The callable may return a bool to indicate success.
  // All other, or void, return types, are ignored and success is assumed.
  template <typename CallableType>
  void addCommand(const std::string& name, std::string argStr, std::string helpStr, CallableType callable)
  {
    // Using std::function to explicitly convert performs template
    // deduction. Then passing to another function that takes a 
    // std::function with template args lets us get at them.
    addCommandInternal(name, argStr, helpStr, std::function(callable));
  }

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

  // Add a property that can be get and set from the command parser using "get" and "set" commands
  // Parsing and printing are done with ostream<< and istream>> so keep that in mind when registering
  // a custom type.
  // Note: The property itself is passed by reference and has no lifetime management.
  template <typename T>
  void addProperty(std::string name, T& property, bool readOnly = false, std::string help = "")
  {
    properties[name] = { help };

    properties[name].get = [&property](std::ostream& os)
    {
      os << property;
    };

    if (!readOnly)
    {
      properties[name].set = [name, &property](std::istream& is)
      {
        is >> property;
        if (is.fail())
        {
          return false;
        }
        return true;
      };
    }
  }

  // Call in a loop to capture and execute commands from stdin
  void processStdIo()
  {
    while (true)
    {
      int inchar = stdio_getchar_timeout_us(0);
      if (inchar > 31 && inchar < 127 && pos < 1023)
      {
        inBuf[pos++] = (char)inchar;
        if (echoOn) std::cout << (char)inchar << std::flush; // echo to client
      }
      else if (inchar == '\b' && pos > 0) // handle backspaces
      {
        --pos;
        if (echoOn) std::cout << "\b \b" << std::flush;
      }
      else if (inchar == '\t' && lastCmd.size() < 1023) // handle tab to insert last command
      {
        if (echoOn) while (pos-- > 0) std::cout << "\b \b" << std::flush;
        std::memcpy(inBuf, lastCmd.data(), lastCmd.size());
        pos = lastCmd.size();
        if (echoOn) std::cout << lastCmd << std::flush;
      }
      else if (inchar == '\n')
      {
        inBuf[pos] = '\0';
        if (echoOn) std::cout << std::endl; // echo to client
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

  // Process a command and its arguments as if recieved from stdin
  void processCommand(std::string cmdAndArgs)
  {
    std::stringstream ss(cmdAndArgs);
    std::string name;
    ss >> name;

    for (auto& [cmdName, cmd] : commands)
    {
      if (name == cmdName)
      {
        if (cmd.func(ss))
        {
          std::cout << "[ok]" << std::endl;
        }
        else
        {
          std::cout << "[fail]" << std::endl;
        }
        return;
      }
    }

    std::string propertyName;
    ss >> propertyName;

    if (name == "set" && properties.count(propertyName) > 0)
    {
      if (properties[propertyName].set && properties[propertyName].set(ss))
      {
        std::cout << "[ok]" << std::endl;
      }
      else
      {
        std::cout << "[fail]" << std::endl;
      }
      return;
    }

    if (name == "get" && properties.count(propertyName) > 0)
    {
      properties[propertyName].get(std::cout);
      std::cout << std::endl << "[ok]" << std::endl;
      return;
    }

    std::cout << "[err]" << std::endl;
  }
};