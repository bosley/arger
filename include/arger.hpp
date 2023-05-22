/*
MIT License

Copyright (c) 2023 bosley

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef ARGER_ARGUMENT_PARSER_HPP
#define ARGER_ARGUMENT_PARSER_HPP

#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>

namespace args {

//! \brief Errors
enum class errors_e {
  DUPLICATE_DEFINITION,      //! Duplicate definition
  MISSING_REQUIRED_ARGUMENT, //! Missing required argument
  INCORRECT_ARGUMENT_TYPE,   //! Incorrect argument type
  EXPECTED_VALUE,            //! Expected value but none found
};

//! \brief Convert an error to a string
static std::string error_to_string(const errors_e e) {
  switch (e) {
  case errors_e::DUPLICATE_DEFINITION:
    return "Duplicate definition";
  case errors_e::MISSING_REQUIRED_ARGUMENT:
    return "Missing required argument";
  case errors_e::INCORRECT_ARGUMENT_TYPE:
    return "Incorrect argument type";
  case errors_e::EXPECTED_VALUE:
    return "Expected value";
  default:
    return "Unknown error";
  }
}

//! \brief Argument type
using arg_t = std::variant<std::string, int64_t, double, bool>;

//! \brief Callback for after a help call so we can clean up
//! 			 and exit if desired before checking for "required"
//! items
using post_help_cb_f = std::function<void()>;

//! \brief Callback function for errors
using error_cb_f = std::function<void(const args::errors_e, const std::string)>;

//! \brief Argument parser
class arger_c {
public:
  arger_c() = delete;
  arger_c(const arger_c &) = delete;
  arger_c(arger_c &&) = delete;
  arger_c &operator=(const arger_c &) = delete;
  arger_c &operator=(arger_c &&) = delete;

  //! \brief Clean up all allocated arguments
  ~arger_c() {
    for (auto &def : argument_defs_) {
      if (def.allocated_arg) {
        delete def.allocated_arg;
      }
    }
  }

  //! \brief Constructor
  //! \param post_help_cb Callback function to be called
  //!    		 in the event help is printed.
  //! \note The callback funtion is meant to allow the user to clean
  //!				up and exit after help is called if that is the
  //! desired 				behavior. If no callback is provided, the program will
  //! continue to parse arguments and may throw errors if there
  //! are missing required arguments.
  arger_c(post_help_cb_f post_help_cb) : post_help_cb_(post_help_cb) {}

  //! \brief Constructor
  //! \param error_cb Callback function to be called in the event
  //!					an error is encountered.
  void set_error_cb(error_cb_f error_cb) { error_cb_ = error_cb; }

  //! \brief Add an argument to the parser
  //! \param arg The argument to add
  bool add_argument(const std::set<std::string> arg,
                    const std::string &description,
                    const arg_t default_value = "", bool required = false) {
    return add_arg(arg, description, default_value, required, false);
  }

  //! \brief Add a flag to the parser
  //! \param arg The flag to add
  //! \param description The description of the flag
  //! \param default_value The default value of the flag
  bool add_flag(const std::set<std::string> arg, const std::string &description,
                bool default_value, bool required = false) {
    return add_arg(arg, description, default_value, required, true);
  }

  //! \brief Get the value of an argument
  //! \paramt T The type of the argument
  //! \param arg The argument to get
  template <typename T> std::optional<T> get_arg(const std::string &arg) {
    if (arguments_map_.find(arg) == arguments_map_.end()) {
      return std::nullopt;
    }
    std::stringstream ss(arguments_map_[arg]->value);
    T val;
    ss >> val;
    return val;
  }

  //! \brief Get the program name
  //! \return The program name
  std::string get_program_name() const { return program_name_; }

  //! \brief Enable/ Disable automatic help
  //! \param enable True to enable, false to disable
  //! \note If automatic help is enabled, the parser will
  //!				automatically print the help message if the help
  //!flag 				is found. If automatic help is disabled, the user 				must call their own
  //!help function
  void set_auto_help(bool enable) { auto_help_enable_ = enable; }

  //! \brief Get all items that were not matched
  //! \return A vector of unmatched items
  std::vector<std::string> get_unmatched_args() const { return unmatchd_args_; }

  //! \brief Parse the arguments
  //! \param argc The number of arguments
  //! \param argv The arguments
  //! \return True if the arguments were parsed successfully
  bool parse(int argc, char **argv) {
    std::vector<std::string> args(argv, argv + argc);

    program_name_ = args[0];
    for (std::size_t i = 0; i < args.size(); ++i) {
      // If auto help is enabled and the help flag is found,
      // call the post help callback and continue
      if (auto_help_enable_ && (args[i] == "-h" || args[i] == "--help")) {
        print_help();
        if (post_help_cb_) {
          post_help_cb_();
        }
        continue;
      }

      if (arguments_map_.find(args[i]) == arguments_map_.end()) {
        unmatchd_args_.push_back(args[i]);
        continue;
      }
      auto arg = arguments_map_[args[i]];
      if (!(arg->is_flag ? handle_flag(args[i]) : handle_argument(args, i))) {
        return false;
      }
    }

    // Check to make sure all required arguments were found
    for (auto &arg : argument_defs_) {
      if (arg.allocated_arg->req_and_found.has_value() &&
          !arg.allocated_arg->req_and_found.value()) {
        if (error_cb_) {
          std::string concated;
          for (auto &arg : arg.args) {
            concated += arg + " ";
          }
          error_cb_(errors_e::MISSING_REQUIRED_ARGUMENT, concated);
        }
        return false;
      }
    }
    return true;
  }

private:
  struct argument_s {
    std::optional<bool> req_and_found{std::nullopt};
    std::string value;
    bool is_flag{false};
  };

  struct arg_def_s {
    std::set<std::string> args;
    std::string description;
    std::string default_value;
    argument_s *allocated_arg{nullptr};
  };

  post_help_cb_f post_help_cb_{nullptr};
  error_cb_f error_cb_{nullptr};
  bool auto_help_enable_{true};

  std::vector<std::string> unmatchd_args_;
  std::vector<arg_def_s> argument_defs_;
  std::unordered_map<std::string, argument_s *> arguments_map_;
  std::string program_name_;

  std::string to_string(const arg_t &arg) {
    if (std::holds_alternative<std::string>(arg)) {
      return std::get<std::string>(arg);
    } else if (std::holds_alternative<int64_t>(arg)) {
      return std::to_string(std::get<int64_t>(arg));
    } else if (std::holds_alternative<double>(arg)) {
      return std::to_string(std::get<double>(arg));
    } else if (std::holds_alternative<bool>(arg)) {
      return std::to_string(std::get<bool>(arg));
    }
    return "unknown";
  }

  bool add_arg(const std::set<std::string> &arg, const std::string &description,
               const arg_t &default_value, bool required, bool is_flag) {
    for (const auto &a : arg) {
      if (arguments_map_.find(a) != arguments_map_.end()) {
        if (error_cb_) {
          error_cb_(errors_e::DUPLICATE_DEFINITION, a);
        }
        return false;
      }
    }

    argument_s *argument =
        new argument_s{(required ? std::optional<bool>(false) : std::nullopt),
                       to_string(default_value), is_flag};

    argument_defs_.push_back(
        arg_def_s{arg, description, to_string(default_value), argument});

    for (const auto &a : arg) {
      arguments_map_[a] = argument;
    }
    return true;
  }

  inline void print_help() {
    std::cout << "Usage: " << program_name_ << " [options]\n" << std::endl;
    std::cout << "Options:\n" << std::endl;

    for (const auto &arg : argument_defs_) {
      for (const auto &a : arg.args) {
        std::cout << a << " ";
      }
      std::cout << "\tdesc:" << arg.description << "\tdefault:"
                << (arg.default_value.empty() ? "<none>" : arg.default_value)
                << "\t"
                << (arg.allocated_arg->req_and_found ? "<required>"
                                                     : "<optional>")
                << "\n"
                << std::endl;
    }
    if (post_help_cb_) {
      post_help_cb_();
    }
  }

  inline bool handle_flag(const std::string &arg) {
    arguments_map_[arg]->value = "true";
    conditionally_mark_found(arg);
    return true;
  }

  inline bool handle_argument(std::vector<std::string> &args,
                              std::size_t &idx) {
    auto arg = args[idx];
    if (idx + 1 >= args.size()) {
      if (error_cb_) {
        error_cb_(errors_e::EXPECTED_VALUE, arg);
      }
      return false;
    }
    auto value = args[++idx];
    arguments_map_[arg]->value = value;
    conditionally_mark_found(arg);
    return true;
  }

  inline void conditionally_mark_found(const std::string &arg) {
    if (arguments_map_[arg]->req_and_found.has_value()) {
      arguments_map_[arg]->req_and_found = true;
    }
  }
};

} // namespace args

#endif
