#include <arger.hpp>
#include <iostream>

void post_help_cb() {
  std::cout << "Help was called" << std::endl;
  std::exit(1);
}

void error_cb(args::errors_e error, const std::string &arg) {
  std::cout << "Error [" << args::error_to_string(error) << "] " << arg
            << std::endl;
  std::exit(1);
}

int main(int argc, char **argv) {
  args::arger_c arger(post_help_cb);
  arger.set_error_cb(error_cb);
  arger.add_argument({"-b", "--bool"}, "A bool", false, true);
  arger.parse(argc, argv);
  auto arg = arger.get_arg<bool>("-b");

  std::cout << "bool: " << arg.value() << std::endl;

  return !arg.value();
}
