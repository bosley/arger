# arger

A simple argument parser for C++ applications

```cpp

#include <arger.hpp>
#include <iostream>

void post_help_cb() {
  std::cout << "Help was called" << std::endl;
  std::exit(0);
}

void error_cb(args::errors_e error, const std::string &arg) {
  std::cout << "Error [" << args::error_to_string(error) << "] " << arg
            << std::endl;
  std::exit(1);
}

int main(int argc, char **argv) {
  args::arger_c arger(post_help_cb);
  arger.set_error_cb(error_cb);
  arger.add_flag({"-b", "--bool"}, "A bool", false);
  arger.add_argument({"-i", "--include_dirs"}, "Include directories");
  arger.parse(argc, argv);

  std::string dirs = arger.get_arg<std::string>("--include_dirs");

  std::cout << "Dirs: " << dirs << std::endl;

  return 0;
}

```
