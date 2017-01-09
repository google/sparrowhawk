Sparrowhawk - Release 1.0

Sparrowhawk is an open-source implementation of Google's Kestrel text-to-speech
text normalization system.  It follows the discussion of the Kestrel system as
described in:

Ebden, Peter and Sproat, Richard. 2015. The Kestrel TTS text normalization
system. Natural Language Engineering, Issue 03, pp 333-353.

After sentence segmentation (sentence_boundary.h), the individual sentences are
first tokenized with each token being classified, and then passed to the
normalizer. The system can output as an unannotated string of words, and richer
annotation with links between input tokens, their input string positions, and
the output words is also available.

REQUIREMENTS:

  This version is known to work under Linux using g++ (>= 4.6) and
  MacOS X using XCode 5. Expected to work wherever adequate POSIX
  (dlopen, ssize_t, basename), c99 (snprintf, strtoll, <stdint.h>),
  and C++11 (<unordered_set>, <unordered_map>, <forward_list>) support
  are available.

  You must have installed the following packages:

  - OpenFst 1.5.4 or higher (www.openfst.org)
  - Thrax 1.2.2 or higher (http://www.openfst.org/twiki/bin/view/GRM/Thrax)
  - re2 (https://github.com/google/re2)
  - protobuf (http://protobuf.googlecode.com/files/protobuf-2.5.0.tar.gz ---
    see e.g. http://jugnu-life.blogspot.com/2013/09/install-protobuf-25-on-ubuntu.html)
  	
INSTALLATION:
  Follow the generic GNU build system instructions in ./INSTALL.  We
  recommend configuring with --enable-static=no for faster
  compiles. 

  NOTE: In some versions of Mac OS-X we have noticed a problem with configure
  whereby it fails to find fst.h. If this occurs, try configuring as follows: 

  CPPFLAGS=-I/usr/local/include LDFLAGS=-L/usr/local/lib ./configure

USAGE:
  Assuming you've installed under the default /usr/local, the library will be
  in /usr/local/lib, and the headers in /usr/local/include/sparrowhawk.

  To use in your own program, include <sparrowhawk/normalizer.h> and compile
  with '-I /usr/local/include'. The compiler must support C++11 (for g++ add the
  flag "-std=c++11"). Link against /usr/local/lib/libsparrowhawk.so and
  -ldl. Set your LD_LIBRARY_PATH (or equivalent) to contain /usr/local/lib.  The
  linking is, by default, dynamic so that the Fst and Arc type DSO extensions
  can be used correctly if desired.

DOCUMENTATION: 
  See ./NEWS for updates since the last release.
