#ifndef EXTRACTIMMEDIATE_HPP
#define EXTRACTIMMEDIATE_HPP

bool isNotHexCharacter(char c);
size_t ExtractImmediateFromString(const std::string& operand,
  std::vector<uint64_t>* results);
 
#endif // EXTRACTIMMEDIATE_HPP
