#ifndef FUNCTIONMETADATA_HPP
#define FUNCTIONMETADATA_HPP

#include <map>
#include <string>
#include <vector>

// A class that provides metainformation about a given function.
// Reads and parses a space-delimited file with the following fields:
//
// FileID FileName FunctionAddress FunctionName IsVulnerable
class FunctionMetadataStore {
public:
  FunctionMetadataStore(const std::string& csvfile);

  bool GetFunctionName(uint64_t file_id, uint64_t address, std::string* out);
  bool GetFileName(uint64_t file_id, std::string* out);
  bool FunctionHasVulnerability(uint64_t file_id, uint64_t address);

  void AddFunctionName(uint64_t file_id, uint64_t address, const std::string&
    function_name);
  void SetFunctionIsVulnerable(uint64_t file_id, uint64_t address, bool val);
  void AddFileName(uint64_t file_id, const std::string& file_name);
private:
  std::map<uint64_t, std::string> file_id_to_name_;
  std::map<std::pair<uint64_t, uint64_t>, std::string> function_to_name_;
  std::map<std::pair<uint64_t, uint64_t>, bool> function_to_vuln_;
  std::string csvfile_name_;
};


#endif // FUNCTIONMETADATA_HPP
