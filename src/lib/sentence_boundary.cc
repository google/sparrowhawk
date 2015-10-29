#include <sparrowhawk/sentence_boundary.h>

#include <memory>
#include <string>
using std::string;
#include <vector>
using std::vector;

#include <sparrowhawk/io_utils.h>
#include <sparrowhawk/logger.h>
#include <sparrowhawk/regexp.h>
#include <sparrowhawk/string_utils.h>

namespace speech {
namespace sparrowhawk {

SentenceBoundary::SentenceBoundary(const string &regexp) :
    pad_exceptions_with_space_prefix_(true) {
  regexp_.reset(new Regexp);
  if (!regexp_->Compile(regexp)) {
    LoggerFatal("SentenceBoundary failed with bad regexp: %s", regexp.c_str());
  }
}

bool SentenceBoundary::LoadSentenceBoundaryExceptions(const string &filename) {
  string raw = IOStream::LoadFileToString(filename);
  vector<string> tokens = SplitString(raw, "\n", true /* skip_empty */);
  for (auto token : tokens) {
    token = StripWhitespace(token);
    // Having it as an unordered list is of course not very efficient for
    // search, but we do not expect these lists to be very long.
    // We pad with a space before it since most scripts that use end-of-sentence
    // markers ambiguously to denote abbreviations also use spaces to delimit
    // words.
    // TODO(rws): We should extend this to regexps to handle things like German
    // ordinals.
    if (pad_exceptions_with_space_prefix_)
      sentence_boundary_exceptions_.push_back(" " + token);
  }
  return true;
}

vector<string> SentenceBoundary::ExtractSentences(
    const string &input_text) const {
  vector<RegMatch> potentials;
  regexp_->GetAllMatches(input_text, &potentials);
  vector<int> cutpoints;
  int last = 0, i;
  for (i = 0; i < potentials.size(); ++i) {
    const int start = potentials[i].start_char;
    const int end = potentials[i].end_char;
    const string text_before = input_text.substr(last, start - last);
    const string marker = input_text.substr(start, end - start);
    const string text_after = input_text.substr(end);
    if (EvaluateCandidate(text_before, marker)) {
      cutpoints.push_back(end);
      last = end;
    }
  }
  vector<string> result;
  last = 0;
  string sentence;
  for (int i = 0; i < cutpoints.size(); ++i) {
    sentence = StripWhitespace(input_text.substr(last, cutpoints[i] - last));
    if (!sentence.empty()) result.push_back(sentence);
    last = cutpoints[i];
  }
  sentence = StripWhitespace(input_text.substr(last));
  if (!sentence.empty()) result.push_back(sentence);
  return result;
}

bool SentenceBoundary::EvaluateCandidate(const string &input_text,
                                         const string &marker) const {
  // Gets the previous sentence and the marker, minus any trailing whitespace.
  string previous = StripWhitespace(input_text + marker);
  int previous_length = previous.size();
  for (const auto exception : sentence_boundary_exceptions_) {
    int length = exception.size();
    if (length <= previous_length &&
        previous.substr(previous_length - length, length) == exception)
      return false;
    // If the exception starts with a space because we have added one, then also
    // check to see if this was the first token --- i.e. matches the entire
    // previous "sentence".
    if (pad_exceptions_with_space_prefix_) {
      string stripped_exception = StripWhitespace(exception);
      if (previous == stripped_exception) return false;
    }
  }
  return true;
}

}  // namespace sparrowhawk
}  // namespace speech
