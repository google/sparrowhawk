// Various utilities to replace Google functionality for safe_strtoX
#ifndef SPARROWHAWK_NUMBERS_H_
#define SPARROWHAWK_NUMBERS_H_

#include <stdint.h>
#include <string>
using std::string;

namespace speech {
namespace sparrowhawk {

typedef int32_t int32;
typedef int64_t int64;

bool safe_strtof(const string &value, float *output);

bool safe_strtod(const string &value, double *output);

bool safe_strto32(const string &value, int32 *output);

bool safe_strto64(const string &value, int64 *output);

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_NUMBERS_H_
