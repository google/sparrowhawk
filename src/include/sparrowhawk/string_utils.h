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
// Various utilities to replace Google functionality for strings.
#ifndef SPARROWHAWK_STRING_UTILS_H_
#define SPARROWHAWK_STRING_UTILS_H_

#include <string>
using std::string;
#include <vector>
using std::vector;

#include <fst/compat.h>
namespace speech {
namespace sparrowhawk {

// Splits string s by sep and returns a vector of strings.
std::vector<string> SplitString(const string &s, const string &delims);

// Splits string s by sep and returns a vector of strings, skipping empties.
std::vector<string> SplitString(const string &s,
                           const string &delims,
                           bool skip_empty);

// Strips whitespace off the beginning and end
string StripWhitespace(const string &s);

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_STRING_UTILS_H_
