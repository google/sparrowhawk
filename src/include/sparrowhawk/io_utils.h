// Various utilities to replace Google functionality for I/O.
#ifndef SPARROWHAWK_IO_UTILS_H_
#define SPARROWHAWK_IO_UTILS_H_

#include <string>
using std::string;

namespace speech {
namespace sparrowhawk {

class IOStream {
 public:
  static string LoadFileToString(const string &filename);
};

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_IO_UTILS_H_
