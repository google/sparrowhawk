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
// Various utilities to replace Google functionality for logging.
#ifndef SPARROWHAWK_LOGGER_H_
#define SPARROWHAWK_LOGGER_H_

// TODO(rws): Write a more respectable logging system or link to some
// open-source substitute.

#include <fst/compat.h>
namespace speech {
namespace sparrowhawk {


}  // namespace sparrowhawk
}  // namespace speech


#define LoggerFormat(format) \
  string(string("[%s:%s:%d] ") + format).c_str()

#define LoggerMessage(type, format, ...)         \
  fprintf(stderr, \
          LoggerFormat(format), \
          type,              \
          __FILE__,             \
          __LINE__,             \
          ##__VA_ARGS__)

#define LoggerDebug(format, ...) LoggerMessage("DEBUG", format, ##__VA_ARGS__)

#define LoggerError(format, ...) LoggerMessage("ERROR", format, ##__VA_ARGS__)

#define LoggerFatal(format, ...) { \
  LoggerMessage("FATAL", format, ##__VA_ARGS__);        \
  exit(1); }                                        \

#define LoggerInfo(format, ...) LoggerMessage("INFO", format, ##__VA_ARGS__)

#define LoggerWarn(format, ...) LoggerMessage("WARNING", format, ##__VA_ARGS__)


#endif  // SPARROWHAWK_LOGGER_H_
