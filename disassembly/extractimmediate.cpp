#include <algorithm>
#include <regex>
#include <string>

// this regex was provided courtesy of mark brand.
constexpr char extraction_regex[] =
  "(?:\\W|0x|^)([[:xdigit:]]+)(?:h|\\W|$)";

// The following code is the ugliest-imaginable solution to extracting operands,
// but I fear I do not know any better.
size_t ExtractImmediateFromString(const std::string& operand,
  std::vector<uint64_t>* results) {
  static std::regex re(extraction_regex, std::regex_constants::ECMAScript);
  size_t count = 0;
  std::sregex_iterator next(operand.begin(), operand.end(), re);
  std::sregex_iterator end;
  while (next != end) {
    std::smatch match = *next;
    for (size_t i = 0; i < match.size(); ++i) {
      std::string immediate = match[i].str();
      uint64_t val = strtoull(immediate.c_str(), nullptr, 16);
      if (val != 0) {
        // The regular expression sometimes extracts the same immediate twice.
        // TODO(thomasdullien): Fix the regular expression and then remove this
        // code.
        if ((results->size() > 0) && (results->back() == val)) {
          continue;
        }
        results->push_back(val);
      }
    }
    next++;
    ++count;
  }
  return count;
}

