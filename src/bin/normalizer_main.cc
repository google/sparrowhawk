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
// Very simple stand-alone binary to run Sparrowhawk normalizer on a line of
// text.
//
// It runs the sentence boundary detector on the input, and then normalizes each
// sentence.
//
// As an example of use, build the test data here, and put them somewhere, such
// as tmp/sparrowhawk_test
//
// Then copy the relevant fars and protos there, edit the protos and then run:
//
// blaze-bin/speech/tts/open_source/sparrowhawk/normalizer_main \
//  --config tmp/sparrowhawk_test/sparrowhawk_configuration_af.ascii_proto
//
// Then input a few sentences on one line such as:
//
// Kameelperde het 'n kenmerkende voorkoms, met hul lang nekke en relatief \
// kort lywe. Hulle word 4,3 - 5,7m lank. Die bulle is effens langer as die \
// koeie.

#include <iostream>
#include <memory>
#include <string>
using std::string;
#include <vector>
using std::vector;

#include <sparrowhawk/normalizer.h>

DEFINE_bool(multi_line_text, false, "Text is spread across multiple lines.");
DEFINE_string(config, "", "Path to the configuration proto.");
DEFINE_string(path_prefix, "./", "Optional path prefix if not relative.");

void NormalizeInput(const string& input,
                    speech::sparrowhawk::Normalizer *normalizer) {
  const std::vector<string> sentences = normalizer->SentenceSplitter(input);
  for (const auto& sentence : sentences) {
    string output;
    normalizer->Normalize(sentence, &output);
    std::cout << output << std::endl;
  }
}

int main(int argc, char** argv) {
  using speech::sparrowhawk::Normalizer;
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(argv[0], &argc, &argv, true);
  std::unique_ptr<Normalizer> normalizer;
  normalizer.reset(new Normalizer());
  CHECK(normalizer->Setup(FLAGS_config, FLAGS_path_prefix));
  string input;
  if (FLAGS_multi_line_text) {
    string line;
    while (std::getline(std::cin, line)) {
      if (!input.empty()) input += " ";
      input += line;
    }
    NormalizeInput(input, normalizer.get());
  } else {
    while (std::getline(std::cin, input)) {
      NormalizeInput(input, normalizer.get());
    }
  }
  return 0;
}
