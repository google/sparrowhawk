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
// Various utilities to replace Google functionality for I/O.
#ifndef SPARROWHAWK_IO_UTILS_H_
#define SPARROWHAWK_IO_UTILS_H_

#include <string>
using std::string;

#include <fst/compat.h>
namespace speech {
namespace sparrowhawk {

class IOStream {
 public:
  static string LoadFileToString(const string &filename);
};

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_IO_UTILS_H_
