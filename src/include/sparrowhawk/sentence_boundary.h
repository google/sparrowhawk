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
// Simple interface for splitting text into sentences. Uses a regular expression
// to define plausible end-of-sentence markers, and allows for a list of
// exceptions --- e.g. abbreviations that end in periods that would not normally
// signal a sentence boundary.
#ifndef SPARROWHAWK_SENTENCE_BOUNDARY_H_
#define SPARROWHAWK_SENTENCE_BOUNDARY_H_

#include <memory>
#include <string>
using std::string;
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <sparrowhawk/regexp.h>

namespace speech {
namespace sparrowhawk {

class SentenceBoundary {
 public:
  explicit SentenceBoundary(const string &regexp);

  // Loads exceptions, such as abbreviations that end in periods, things like
  // "Y!", or whatever.  Note that these are all case sensitive, so one must
  // provide alternate forms if one expects that the form may be cased
  // differently.
  bool LoadSentenceBoundaryExceptions(const string &filename);

  std::vector<string> ExtractSentences(const string &input_text) const;

  // If true, then prefixes each exception in the exception list with a space,
  // so that it when matching against a potential end-of-sentence position, it
  // will force the match to occur only when there is a preceding space, or at
  // the beginning of the string.
  void set_pad_exceptions_with_space_prefix(bool
                                            pad_exceptions_with_space_prefix) {
    pad_exceptions_with_space_prefix_ = pad_exceptions_with_space_prefix;
  }

 private:
  // Returns true if the candidate position is a plausible sentence
  // boundary. Currently uses the regexp and the sentence boundary exceptions
  // list, but could be replaced with something learned.
  bool EvaluateCandidate(const string &input_text, const string &marker) const;

  std::unique_ptr<Regexp> regexp_;
  std::vector<string> sentence_boundary_exceptions_;
  bool pad_exceptions_with_space_prefix_;
  DISALLOW_COPY_AND_ASSIGN(SentenceBoundary);
};

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_SENTENCE_BOUNDARY_H_
