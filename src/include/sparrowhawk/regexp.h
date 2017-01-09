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
// Various utilities to replace Google wrapper for re2.
// Wrapper class regex library, it's very basic, and wraps the re2
// regexp library.

#ifndef SPARROWHAWK_REGEXP_H_
#define SPARROWHAWK_REGEXP_H_

#include <string>
using std::string;
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <re2/re2.h>

namespace speech {
namespace sparrowhawk {

// A regmatch is one match result - there may be one or more per string.
struct RegMatch {
  int start_char;
  int end_char;
  string full_str;
  // number of sub-expressions
  int n_sub;
  int len;
  // if the regexp contained subexpressions
  std::vector<string> sub_str;
  std::vector<int> sub_start;
  std::vector<int> sub_end;
};

class Regexp {
 public:
  Regexp();
  ~Regexp();

  // Compiles a regexp. Returns true if compile successful.
  bool Compile(const string &pattern);

  // The number of sub expressions for this regexp.
  int nsubexp() const;

  // Checks for any match at all. Returns true if match.
  bool CheckFullMatch(const string &input) const;

  // Checks for any match at all. Returns true if match.
  bool CheckMatch(const string &input) const;

  // Checks for any match at all. Returns true if match.
  static bool CheckMatch(const string &input, const string &pattern);

  // Gets vector of start and end chars for all matching string parts
  // returns number of matches.  Fills the matches vector with RegMatch objects.
  int GetAllMatches(const string &input,
                    std::vector<RegMatch> *matches) const;

  // Accessor for boolean whether this has been successfully compiled
  bool ok() const;

  // Deletes and resets internal data.
  void Clear();

 private:
  // The underlying compiled regexp object internal structure
  RE2 *re_;

  int32 nsubexp_;

  DISALLOW_COPY_AND_ASSIGN(Regexp);
};

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_REGEXP_H_
