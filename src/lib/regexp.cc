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
#include <sparrowhawk/regexp.h>

#include <memory>
#include <sparrowhawk/logger.h>

namespace speech {
namespace sparrowhawk {

Regexp::Regexp() {
  re_ = nullptr;
  nsubexp_ = -1;
}

Regexp::~Regexp() {
  Clear();
}

void Regexp::Clear() {
  delete re_;
  re_ = nullptr;
  nsubexp_ = -1;
}

int Regexp::nsubexp() const {
  return nsubexp_;
}

bool Regexp::ok() const {
  return re_ != nullptr && re_->ok();
}

bool Regexp::Compile(const string &pattern) {
  Clear();
  RE2::Options options;
  options.set_longest_match(true);
  options.set_log_errors(false);
  re_ = new RE2(pattern, options);

  if (re_ == nullptr) {
    LoggerError("Error in allocating regexp \"%s\"", pattern.c_str());
    return false;
  }
  if (!re_->ok()) {
    LoggerError("Error in allocating regexp \"%s\": %s",
                pattern.c_str(),
                re_->error().c_str());
    Clear();
    return false;
  }
  nsubexp_ = re_->NumberOfCapturingGroups();
  return true;
}

bool Regexp::CheckMatch(const string &input) const {
  if (ok()) {
    return RE2::PartialMatch(input, *re_);
  } else {
    return false;
  }
}

bool Regexp::CheckFullMatch(const string &input) const {
  if (ok()) {
    return RE2::FullMatch(input, *re_);
  } else {
    return false;
  }
}

bool Regexp::CheckMatch(const string &input, const string &pattern) {
  return RE2::PartialMatch(input, pattern);
}

int Regexp::GetAllMatches(const string &input,
                          std::vector<RegMatch> *matches) const {
  if (!ok()) {
    return 0;
  }
  int nmatches = 0;
  int offset = 0;
  int end_pos = input.size();
  matches->clear();
  re2::StringPiece input_piece(input);

  std::unique_ptr<re2::StringPiece[]> matched_pieces(new re2::StringPiece[1 + nsubexp_]);
  bool result = re_->Match(input_piece,
                           offset,
                           end_pos,
                           RE2::UNANCHORED,
                           matched_pieces.get(),
                           1 + nsubexp_);
  RegMatch re_info;
  while (result) {
    nmatches++;
    re_info.sub_str.clear();
    re_info.sub_start.clear();
    re_info.sub_end.clear();
    re_info.full_str = "";
    re_info.n_sub = nsubexp_;
    int match_offset = matched_pieces[0].data() - input.c_str();
    int match_length = matched_pieces[0].length();

    re_info.start_char = match_offset;
    re_info.end_char = match_offset + match_length;
    re_info.len = match_length;
    re_info.full_str = matched_pieces[0].as_string();

    for (int i = 1; i <= nsubexp_; ++i) {
      re_info.sub_str.push_back(matched_pieces[i].as_string());
      int sub_match_start = matched_pieces[i].data() - input.c_str();
      re_info.sub_start.push_back(sub_match_start);
      re_info.sub_end.push_back(sub_match_start + matched_pieces[i].length());
    }

    matches->push_back(re_info);
    offset = re_info.end_char;
    result = re_->Match(input_piece,
                       offset,
                       end_pos,
                       RE2::UNANCHORED,
                       matched_pieces.get(),
                       1 + nsubexp_);
  }
  return nmatches;
}

}  // namespace sparrowhawk
}  // namespace speech
