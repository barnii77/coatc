#include <string>
#include <vector>

namespace codegen {
std::string linkExternalLibraries(std::string obj, std::vector<std::string> const &static_libs, std::vector<std::string> const &dynamic_libs);
}  // namespace codegen
