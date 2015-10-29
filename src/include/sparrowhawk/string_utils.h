// Various utilities to replace Google functionality for strings.
#ifndef SPARROWHAWK_STRING_UTILS_H_
#define SPARROWHAWK_STRING_UTILS_H_

#include <string>
using std::string;
#include <vector>
using std::vector;

namespace speech {
namespace sparrowhawk {

// Splits string s by sep and returns a vector of strings.
vector<string> SplitString(const string &s, const string &delims);

// Splits string s by sep and returns a vector of strings, skipping empties.
vector<string> SplitString(const string &s,
                           const string &delims,
                           bool skip_empty);

// Strips whitespace off the beginning and end
string StripWhitespace(const string &s);

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_STRING_UTILS_H_
