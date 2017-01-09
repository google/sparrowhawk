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
// The normalizer is the main part of Sparrowhawk. Loosely speaking it follows
// the discussion of the (Google-internal) Kestrel system as described in
//
// Ebden, Peter and Sproat, Richard. 2015. The Kestrel TTS text normalization
// system. Natural Language Engineering, Issue 03, pp 333-353.
//
// After sentence segmentation (sentence_boundary.h), the individual sentences
// are first tokenized with each token being classified, and then passed to the
// normalizer. The system can output as an unannotated string of words, and
// richer annotation with links between input tokens, their input string
// positions, and the output words is also available.

#ifndef SPARROWHAWK_NORMALIZER_H_
#define SPARROWHAWK_NORMALIZER_H_

#include <string>
using std::string;
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <sparrowhawk/items.pb.h>
#include <sparrowhawk/sentence_boundary.h>
#include <sparrowhawk/sparrowhawk_configuration.pb.h>
#include <sparrowhawk/rule_system.h>
#include <sparrowhawk/spec_serializer.h>

namespace speech {
namespace sparrowhawk {

class Normalizer {
 public:
  Normalizer();

  ~Normalizer();

  // The functions definitions have been split across two files, normalizer.cc
  // and normalizer_utils.cc, just to keep things a littler tidier. Below we
  // indicate where each function is found.

  // normalizer.cc
  // Method to load and set data for each derived method
  bool Setup(const string &configuration_proto, const string &pathname_prefix);

  // normalizer.cc
  // Interface to the normalization system for callers that want to be agnostic
  // about utterances.
  bool Normalize(const string &input, string *output) const;

  // normalizer.cc
  // Interface to the normalization system for callers that want to be agnostic
  // about utterances. Shows the token/word alignment.
  bool NormalizeAndShowLinks(const string &input, string *output) const;

  // normalizer_utils.cc
  // Helper for linearizing words from an utterance into a string
  string LinearizeWords(Utterance *utt) const;

  // normalizer_utils.cc
  // Helper for showing the indices of all tokens, words and their alignment
  // links.
  string ShowLinks(Utterance *utt) const;

  // normalizer.cc
  // Preprocessor to use the sentence splitter to break up text into
  // sentences. An application would normally call this first, and then
  // normalize each of the resulting sentences.
  std::vector<string> SentenceSplitter(const string &input) const;

 private:
  // normalizer.cc
  // Internal interface to normalization.
  bool Normalize(Utterance *utt, const string &input) const;

  // normalizer_utils.cc
  // As in Kestrel, adds a phrase and silence.
  // TODO(rws): Possibly remove this since it is actually not being used.
  void AddPhraseToUtt(Utterance *utt) const { AddPhraseToUtt(utt, false); }

  // normalizer_utils.cc
  void AddPhraseToUtt(Utterance *utt, bool addword) const;

  // normalizer_utils.cc
  // Adds a single word to the end of the Word stream
  Word* AddWord(Utterance *utt, Token *token,
                const string &spelling) const;

  // normalizer_utils.cc
  // Function to add the words in the string 'name' onto the
  // end of the Word stream.
  Word* AddWords(Utterance *utt, Token *token,
                 const string &name) const;

  // Finds the index of the provided token.
  int TokenIndex(Utterance *utt, Token *token) const;
  // normalizer_utils.cc
  // As with Peter's comment in
  // speech/patts2/modules/kestrel/verbalize_general.cc, clear out all the mucky
  // fields that we don't want verbalization to see.
  void CleanFields(Token *markup) const;

  // normalizer_utils.cc
  // Returns the substring of the input between left and right
  string InputSubstring(int left, int right) const;

  // normalizer.cc
  // Performs tokenization and classification on the input utterance, the first
  // step of normalization
  bool TokenizeAndClassifyUtt(Utterance *utt, const string &input) const;

  // normalizer_utils.cc
  // Serializes the contents of a Token to a string
  string ToString(const Token &markup) const;

  // normalizer.cc
  // Verbalizes semiotic classes, defaulting to verbatim verbalization for
  // something that is marked as a semiotic class but for which the
  // verbalization grammar fails.
  bool VerbalizeSemioticClass(const Token &markup, string *words) const;

  // normalizer.cc
  // Performs verbalization on the input utterance, the second step of
  // normalization
  bool VerbalizeUtt(Utterance *utt) const;

  string input_;
  std::unique_ptr<RuleSystem> tokenizer_classifier_rules_;
  std::unique_ptr<RuleSystem> verbalizer_rules_;
  std::unique_ptr<SentenceBoundary> sentence_boundary_;
  std::unique_ptr<Serializer> spec_serializer_;
  std::set<string> sentence_boundary_exceptions_;

  DISALLOW_COPY_AND_ASSIGN(Normalizer);
};

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_NORMALIZER_H_
