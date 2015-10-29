#include <sparrowhawk/numbers.h>

#include <errno.h>
#include <string>
using std::string;

namespace speech {
namespace sparrowhawk {

#define CONVERT(value, output) \
  char *endptr; \
  *output = strtof(value.c_str(), &endptr); \
  if (errno == ERANGE) return false; \
  if (endptr < value.c_str() + value.size()) return false; \
  return true;

bool safe_strtof(const string &value, float *output) {
  CONVERT(value, output);
}

bool safe_strtod(const string &value, double *output) {
  CONVERT(value, output);
}

bool safe_strto32(const string &value, int32 *output) {
  CONVERT(value, output);
}

bool safe_strto64(const string &value, int64 *output) {
  CONVERT(value, output);
}

#undef CONVERT

}  // namespace sparrowhawk
}  // namespace speech
