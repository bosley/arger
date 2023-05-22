#include <arger.hpp>
#include <iostream>

void post_help_cb() {
  std::cout << "Help was called" << std::endl;
  std::exit(1);
}

void error_cb(args::errors_e error, const std::string &arg) {
  std::cout << "Error [" << args::error_to_string(error) << "] " << arg
            << std::endl;
  std::exit(0);
}

int main(int argc, char **argv) {
  args::arger_c arger(post_help_cb);
  arger.set_error_cb(error_cb);
  arger.add_argument({"-b", "--bool"}, "A bool", true, true);
  arger.parse(argc, argv);
  return 1;
}
