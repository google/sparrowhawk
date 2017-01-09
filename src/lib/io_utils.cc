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
#include <sparrowhawk/io_utils.h>

#include <iostream>
#include <fstream>
using std::ifstream;
#include <memory>

#include <sparrowhawk/logger.h>

namespace speech {
namespace sparrowhawk {

string IOStream::LoadFileToString(const string &filename) {
  std::ifstream strm(filename.c_str(), std::ios_base::in);
  if (!strm) {
    LoggerFatal("Error opening file %s", filename.c_str());
  }
  strm.seekg(0, strm.end);
  int length = strm.tellg();
  strm.seekg(0, strm.beg);
  std::unique_ptr<char[]> data(new char[length + 1],
                               std::default_delete<char[]>());
  strm.read(data.get(), length);
  if (strm.fail()) {
    LoggerFatal("Error loading from file %s", filename.c_str());
  }
  data.get()[length] = 0;
  return string(data.get(), length);
}

}  // namespace sparrowhawk
}  // namespace speech
