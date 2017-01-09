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
// This is a class to serialize protocol buffers directly
// into a FST in preparation for them to be verbalized.
// The main advantage of this for us is that we produce a FST with multiple
// orderings which the verbalizer can consume however it wants; this
// removes the necessity for the prior reordering hacks etc.
//
// As with ProtobufParser, this class is not threadsafe as it stores
// internal state; the expectation is to create temporary local instances
// of it rather than persisting a single shared instance.

#ifndef SPARROWHAWK_PROTOBUF_SERIALIZER_H_
#define SPARROWHAWK_PROTOBUF_SERIALIZER_H_

#include <vector>
using std::vector;

#include <fst/compat.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <thrax/grm-manager.h>
#include <re2/re2.h>

namespace speech {
namespace sparrowhawk {

using thrax::GrmManager;
class Utterance;

class ProtobufSerializer {
 public:
  typedef GrmManager::MutableTransducer MutableTransducer;
  typedef MutableTransducer::Arc::StateId StateId;

  // Serializes message into given fst.
  ProtobufSerializer(const google::protobuf::Message *message,
                     MutableTransducer *fst);
  ~ProtobufSerializer();

  // Serializes the message into the FST.
  void SerializeToFst();

  // Serializes the message into a string
  string SerializeToString() const;

 protected:
  typedef google::protobuf::FieldDescriptor FieldDescriptor;
  typedef std::vector<const FieldDescriptor *> FieldDescriptorVector;
  typedef fst::StateIterator<MutableTransducer> StateIterator;
  typedef fst::ArcIterator<MutableTransducer> ArcIterator;
  typedef MutableTransducer::Arc Arc;
  typedef Arc::Label Label;

  // Internal constructor that allows selecting the state to begin from.
  ProtobufSerializer(const google::protobuf::Message *message,
                     MutableTransducer *fst,
                     StateId state);

  // Serializes the entire message into the FST, and returns the final state id.
  StateId SerializeToFstInternal();

  // Serializes a single permutation into the FST.
  void SerializePermutation(const FieldDescriptorVector &fields);

  // Serializes a single field into the FST.
  StateId SerializeField(const FieldDescriptor *field,
                         int index,
                         StateId state);

  // Serializes a string into the FST.
  StateId SerializeString(const string &str, StateId state);

  // As above, allowing control of whether quotes are optional or not.
  StateId SerializeString(const string &str,
                          StateId state,
                          bool optional_quotes);

  // Serializes a single character into the FST.
  StateId SerializeChar(char c, StateId state);

  // Links the last arc that has a non-space output symbol to the new final
  // state by adding an epsilon arc from this arc's destination state to the new
  // final state, cutting out unnecessary whitespace and connecting multiple
  // permutations to a common destination.
  void StripTrailingSpace(StateId new_final_state);

  const google::protobuf::Message *message_;
  const google::protobuf::Reflection *reflection_;
  MutableTransducer *fst_;
  const StateId initial_state_;
  static const RE2 kReTrailingZeroes;
  static const int kReNumMatchGroups;

 private:
  DISALLOW_COPY_AND_ASSIGN(ProtobufSerializer);
};

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_PROTOBUF_SERIALIZER_H_
