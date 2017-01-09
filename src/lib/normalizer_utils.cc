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
// TODO(rws): This is small enough now that maybe we don't really need this
// separate file.
//
// More definitions for the Normalizer class, put here because they are icky
// low-level hanky panky.

// utt->AppendToken()
// utt->AppendWord()

#include <memory>
#include <string>
using std::string;

#include <sparrowhawk/items.pb.h>
#include <sparrowhawk/normalizer.h>
#include <sparrowhawk/protobuf_serializer.h>
#include <sparrowhawk/string_utils.h>

namespace speech {
namespace sparrowhawk {

// Same as in Kestrel: add a phrase boundary at the beginning and ending of the
// utterance.

void Normalizer::AddPhraseToUtt(Utterance* utt, bool addword) const {
  Token* token = utt->mutable_linguistic()->add_tokens();
  token->set_type(Token::PUNCT);
  token->set_name("");
  token->set_phrase_break(true);
  if (addword) AddWord(utt, token, "sil");
}

int Normalizer::TokenIndex(Utterance* utt, Token *token) const {
  for (int i = 0; i < utt->linguistic().tokens_size(); ++i) {
    const class Token *t = &(utt->linguistic().tokens(i));
    if (t == token) {
      return i;
    }
  }
  return -1;
}

Word* Normalizer::AddWord(Utterance* utt,
                          Token* token,
                          const string& spelling) const {
  Word* word = utt->mutable_linguistic()->add_words();
  int word_index = utt->linguistic().words_size() - 1;
  if (!token->has_first_daughter() || token->first_daughter() == -1) {
    token->set_first_daughter(word_index);
  }
  token->set_last_daughter(word_index);
  word->set_parent(TokenIndex(utt, token));
  word->set_spelling(spelling);
  word->set_id(spelling);
  return word;
}

// Similar to Kestrel, but without the lexicon().ContainsWordId(spelling) logic,
// which we want to shunt to later processing.
// We assume that if someone puts a "," in the verbalization grammar, they mean
// for this to represent a phrase boundary, so we add in logic here fore that.

Word* Normalizer::AddWords(Utterance* utt, Token* token,
                           const string& words) const {
  std::vector<string> word_names = SplitString(words, " \t\n");
  Word* word = NULL;

  for (int i = 0; i < word_names.size(); ++i) {
    if (word_names[i] == ",")
      word = AddWord(utt, token, "sil");
    else
      word = AddWord(utt, token, word_names[i]);
  }
  return word;  // return last word added.
}

void Normalizer::CleanFields(Token* markup) const {
  markup->clear_first_daughter();
  markup->clear_last_daughter();
  markup->clear_type();
  markup->clear_skip();
  markup->clear_next_space();
  markup->clear_phrase_break();
  markup->clear_start_index();
  markup->clear_end_index();
  markup->clear_name();
}

string Normalizer::InputSubstring(int left, int right) const {
  if (left < 0 || right >= input_.size() || left > right) return "";
  return input_.substr(left, right - left + 1);
}

string Normalizer::LinearizeWords(Utterance* utt) const {
  string output;
  for (int i = 0; i < utt->linguistic().words_size(); ++i) {
    if (i) output.append(" ");
    output.append(utt->linguistic().words(i).spelling());
  }
  return output;
}

string Normalizer::ShowLinks(Utterance *utt) const {
  string output;
  for (int i = 0; i < utt->linguistic().tokens_size(); ++i) {
    output.append("Token:\t" + std::to_string(i) + "\t");
    output.append(utt->linguistic().tokens(i).name() + "\t");
    // Start and end positions in the input string.
    output.append(std::to_string(utt->linguistic().tokens(i).start_index()));
    output.append(",");
    output.append(std::to_string(utt->linguistic().tokens(i).end_index()));
    output.append("\t");
    // First and last word daughters.
    output.append(std::to_string(utt->linguistic().tokens(i).first_daughter()));
    output.append(",");
    output.append(std::to_string(utt->linguistic().tokens(i).last_daughter()));
    output.append("\n");
  }
  for (int i = 0; i < utt->linguistic().words_size(); ++i) {
    output.append("Word:\t" + std::to_string(i) + "\t");
    output.append(utt->linguistic().words(i).spelling());
    output.append("\t" + std::to_string(utt->linguistic().words(i).parent()));
    output.append("\n");
  }
  return output;
}

string Normalizer::ToString(const Token& markup) const {
  ProtobufSerializer serializer(&markup, NULL);
  return serializer.SerializeToString();
}

}  // namespace sparrowhawk
}  // namespace speech
