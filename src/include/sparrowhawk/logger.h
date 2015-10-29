// Various utilities to replace Google functionality for logging.
#ifndef SPARROWHAWK_LOGGER_H_
#define SPARROWHAWK_LOGGER_H_

// TODO(rws): Write a more respectable logging system or link to some
// open-source substitute.

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
