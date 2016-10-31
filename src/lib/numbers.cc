// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2015 and onwards Google, Inc.
#include <sparrowhawk/numbers.h>

#include <errno.h>
#include <stdlib.h>
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
