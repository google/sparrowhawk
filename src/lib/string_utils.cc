#include <sparrowhawk/string_utils.h>

#include <string>
using std::string;
#include <vector>
using std::vector;

namespace speech {
namespace sparrowhawk {

vector<string> SplitString(const string &s, const string &delims) {
  return SplitString(s, delims, false);
}

vector<string> SplitString(const string &s,
                           const string &delims,
                           bool skip_empty) {
  vector<string> out;
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
