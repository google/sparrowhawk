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
#include <sparrowhawk/string_utils.h>

#include <string>
using std::string;
#include <vector>
using std::vector;

namespace speech {
namespace sparrowhawk {

std::vector<string> SplitString(const string &s, const string &delims) {
  return SplitString(s, delims, false);
}

std::vector<string> SplitString(const string &s,
                           const string &delims,
                           bool skip_empty) {
  std::vector<string> out;
  if (s.empty()) {
    return out;
  }

  string::size_type len = s.length(), i = 0, pos = 0;
  do {
    if ((i = s.find_first_of(delims, pos)) == string::npos) {
      string substring = s.substr(pos);
      if (skip_empty && substring.empty()) continue;
      out.push_back(substring);
    } else {
      if (pos != i) {
        string substring = s.substr(pos, i - pos);
        if (skip_empty && substring.empty()) continue;
        out.push_back(substring);
      }
      pos = i + 1;
    }
  } while (i != string::npos && pos < len);
  return out;
}

string StripWhitespace(const string &s) {
  int start = s.find_first_not_of(" \t\n");
  if (start == string::npos) return "";
  int end = s.find_last_not_of(" \t\n");
  return s.substr(start, end - start + 1);
}

}  // namespace sparrowhawk
}  // namespace speech
