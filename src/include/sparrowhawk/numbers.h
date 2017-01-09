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
// Various utilities to replace Google functionality for safe_strtoX
#ifndef SPARROWHAWK_NUMBERS_H_
#define SPARROWHAWK_NUMBERS_H_

#include <stdint.h>
#include <string>
using std::string;

#include <fst/compat.h>
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
