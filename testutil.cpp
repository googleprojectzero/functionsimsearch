#include "testutil.hpp"

// Data describing where to find RarVM::ExecuteStandardFilter in various
// optimization settings and compilers.
std::map<uint64_t, const std::string> id_to_filename = {
  { 0xf89b73cc72cd02c7ULL, "../testdata/unrar.5.5.3.builds/unrar.x64.O0" },
  { 0x5ae018cfafb410f5ULL, "../testdata/unrar.5.5.3.builds/unrar.x64.O2" },
  { 0x51f3962ff93c1c1eULL, "../testdata/unrar.5.5.3.builds/unrar.x64.O3" },
  { 0x584e2f1630b21cfaULL, "../testdata/unrar.5.5.3.builds/unrar.x64.Os" },
  { 0xf7f94f1cdfbe0f98ULL, "../testdata/unrar.5.5.3.builds/unrar.x86.O0" },
  { 0x83fe3244c90314f4ULL, "../testdata/unrar.5.5.3.builds/unrar.x86.O2" },
  { 0x396063026eaac371ULL, "../testdata/unrar.5.5.3.builds/unrar.x86.O3" },
  { 0x924daa0b17c6ae64ULL, "../testdata/unrar.5.5.3.builds/unrar.x86.Os" },
  { 0xb9b4ecb0faefd8cfULL, "../testdata/unrar.5.5.3.builds/VS2015/"
    "unrar32/Release/UnRAR.exe" },
  { 0x720c272a7261ec7eULL, "../testdata/unrar.5.5.3.builds/VS2015/"
    "unrar32/Debug/UnRAR.exe" },
  { 0x22b6ae5553ee8881ULL, "../testdata/unrar.5.5.3.builds/VS2015/"
    "unrar64/Release/UnRAR.exe" },
  { 0x019beb40ff26b418ULL, "../testdata/unrar.5.5.3.builds/VS2015/"
    "unrar64/Debug/UnRAR.exe" }
};

std::map<uint64_t, const std::string> id_to_mode = {
  { 0xf89b73cc72cd02c7ULL, "ELF" },
  { 0x5ae018cfafb410f5ULL, "ELF" },
  { 0x51f3962ff93c1c1eULL, "ELF" },
  { 0x584e2f1630b21cfaULL, "ELF" },
  { 0xf7f94f1cdfbe0f98ULL, "ELF" },
  { 0x83fe3244c90314f4ULL, "ELF" },
  { 0x396063026eaac371ULL, "ELF" },
  { 0x924daa0b17c6ae64ULL, "ELF" },
  { 0xb9b4ecb0faefd8cfULL, "PE" },
  { 0x720c272a7261ec7eULL, "PE" },
  { 0x22b6ae5553ee8881ULL, "PE" },
  { 0x019beb40ff26b418ULL, "PE" }
};

// The addresses of RarVM::ExecuteStandardFilter in the various executables
// listed above.
std::map<uint64_t, uint64_t> id_to_address_function_1 = {
  { 0xf89b73cc72cd02c7ULL, 0x0000000000418400 },
  { 0x5ae018cfafb410f5ULL, 0x0000000000411890 },
  { 0x51f3962ff93c1c1eULL, 0x0000000000416f60 },
  { 0x584e2f1630b21cfaULL, 0x000000000040e092 },
  { 0xf7f94f1cdfbe0f98ULL, 0x000000000805e09c },
  { 0x83fe3244c90314f4ULL, 0x0000000008059f70 },
  { 0x396063026eaac371ULL, 0x000000000805e910 },
  { 0x924daa0b17c6ae64ULL, 0x00000000080566fc },
  { 0xb9b4ecb0faefd8cfULL, 0x0000000000417bb0 },
  { 0x720c272a7261ec7eULL, 0x000000000043e500 },
  { 0x22b6ae5553ee8881ULL, 0x0000000140015AC8 },
  { 0x019beb40ff26b418ULL, 0x000000014004EA80 }
};


