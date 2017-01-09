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
// This is a basic parser for reading protobufs directly from
// FSTs. The main advantage this offers for the moment is the
// ability to track token start/end points, but later can be
// extended to other types and may ultimately be portable to
// Android.
//
// This class is not thread safe since it needs to store internal
// parse state. The expectation is to create temporary local instances
// of it rather than persisting a single shared instance.

#ifndef SPARROWHAWK_PROTOBUF_PARSER_H_
#define SPARROWHAWK_PROTOBUF_PARSER_H_

#include <string>
using std::string;
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <thrax/grm-manager.h>

namespace speech {
namespace sparrowhawk {

using thrax::GrmManager;
class Utterance;
class Token;

class ProtobufParser {
 public:
  typedef GrmManager::Transducer Transducer;

  explicit ProtobufParser(const Transducer *fst);
  ~ProtobufParser();

  // Parses tokens from the member FST into the Token stream of the
  // utterance. Note that, as the name suggests, it *cannot* parse
  // other streams such as Word, Specification, etc.
  // This assumes that the FST has a unique path through it
  // (ie. has been created via ShortestPath())
  bool ParseTokensFromFST(Utterance *utt,
                          bool set_semiotic_class = true,
                          bool fix_lookahead = false);

  // Parses the given message from the member FST.
  // Message must have been registered with ProtobufField for this
  // to succeed.
  // This assumes that the FST has a unique path through it
  // (ie. has been created via ShortestPath())
  bool ParseMessageFromFST(google::protobuf::Message *message);

 protected:
  typedef fst::StateIterator<Transducer> StateIterator;
  typedef fst::ArcIterator<Transducer> ArcIterator;
  typedef Transducer::Arc::Label Label;
  typedef Transducer::StateId StateId;

  // Parses a single message from the FST. The message name and opening brace
  // have already been consumed; this goes until the closing brace.
  // If eof_allowed is true then it's not a failure to reach the end of the FST
  // before finding a closing brace.
  bool ParseMessage(bool eof_allowed, google::protobuf::Message *message);

  // Parses a single field value from the FST. The field name has already been
  // consumed, this just stores the value in the given string.
  bool ParseFieldValue(string *value);

  // As above, but deals with a quoted field which is rather trickier due to
  // escaping and suchforth. The first quote has already been consumed.
  bool ParseQuotedFieldValue(bool ignore_backslashes, string *value);

  // Consumes a single token label from the FST, ie. a message or field name.
  // Returns true if label found, false if not.
  bool ConsumeLabel(string *label);

  // Consumes any output whitespace from the FST.
  void ConsumeWhitespace();

  // Moves to the next state in the FST. Returns true if one was found, false
  // if the end has been reached.
  bool NextState();

  // Backs up to the previous state. Can only back up once, so should only be
  // called once between each call to NextState().
  void PrevState();

  // Updates start/end indices on a token that we've just parsed.
  void UpdateTokenIndices(Token *token,
                          bool set_semiotic_class,
                          bool fix_lookahead);

  // Logs an error message on parsing fail.
  void LogError();

  // Records the field orders if there is a preserve_order field and it's true
  bool RecordFieldOrder(google::protobuf::Message *message,
                        const std::vector<string> &field_order);

  // Applies fixes to the token names caused by lookahead FSTs.
  void FixLookahead(Utterance *utt);

  // Sets the content of a field.
  void SetField(google::protobuf::Message *message,
                const google::protobuf::Reflection *reflection,
                const google::protobuf::FieldDescriptor *descriptor,
                const string &value) const;

  // FST we're parsing from.
  const Transducer *fst_;
  // Current state that we're up to.
  StateId state_;
  // The previous state
  StateId last_state_;
  // Input/output labels from the last arc.
  Label ilabel_;
  Label olabel_;
  // Start index of the current token.
  int token_start_;
  // End index of the immediately preceding token.
  int last_token_end_;
  // Name of the current token (ie. its input text).
  string token_name_;
  // Name (input labels) of the immediately preceding token.
  string last_token_name_;
};

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_PROTOBUF_PARSER_H_
