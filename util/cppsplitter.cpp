#include <stack>

#include "util/cppsplitter.hpp"

// Split a string into tokens based on spaces, but keep nesting of <> brackets
// intact.
void CppSplitter(const std::string& input, std::deque<std::string>& result) {
  std::stack<char> bracket_stack;
  std::string::const_iterator begin_of_token = input.begin();
  std::string::const_iterator current_position = input.begin();

  while(current_position != input.end()) {
    switch(*current_position) {
      case '(':
      case '<':
      case '[':
        bracket_stack.push('(');
        break;
      case ')':
      case '>':
      case ']':
        bracket_stack.pop();
        break;
      case ' ':
        if (bracket_stack.empty()) {
          // Create a new token and throw it into the result queue.
          result.emplace_back(begin_of_token, current_position);
          begin_of_token = current_position;
          ++begin_of_token;
        }
        break;
      default:
        break;
    }
    ++current_position;
  }
  result.emplace_back(begin_of_token, current_position);
}
