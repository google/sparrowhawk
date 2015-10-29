#include <sparrowhawk/io_utils.h>

#include <iostream>
#include <fstream>
using std::ifstream;
#include <memory>

#include <sparrowhawk/logger.h>

namespace speech {
namespace sparrowhawk {

string IOStream::LoadFileToString(const string &filename) {
  ifstream strm(filename.c_str(), std::ios_base::in);
  if (!strm) {
    LoggerFatal("Error loading from file %s", filename.c_str());
  }
  strm.seekg(0, strm.end);
  int length = strm.tellg();
  strm.seekg(0, strm.beg);
  std::unique_ptr<char> data;
  data.reset(new char[length + 1]);
  strm.read(data.get(), length);
  data.get()[length] = 0;
  return string(data.get());
}

}  // namespace sparrowhawk
}  // namespace speech
