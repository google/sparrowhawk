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
// Recursively serializes a single record in the spec and concatenates onto a
// transducer.
//
// Typically the serialized field content looks like
//           <field_name>:<field_value>|
// Note that nothing is serialized if the field corresponding to the record_spec
// field_path is missing in the token.
//
// This is used by the StyleSerializer for serializing all the records in a
// given style. It constructs the RecordSerializer for each record in the
// style_spec. Given a token it sequentially invokes the Serialize function of
// the records in the style being serialized.

#ifndef SPARROWHAWK_RECORD_SERIALIZER_H_
#define SPARROWHAWK_RECORD_SERIALIZER_H_

#include <memory>
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <thrax/grm-manager.h>
#include <sparrowhawk/items.pb.h>
#include <sparrowhawk/serialization_spec.pb.h>
#include <sparrowhawk/field_path.h>
#include <re2/re2.h>

namespace speech {
namespace sparrowhawk {

class RecordSerializer {
 public:
  typedef fst::StdVectorFst MutableTransducer;

  // Creates and returns a RecordSerializer from the record_spec by noting the
  // field path and path label the record and recursively building
  // record_serializers for prefix and suffix specs.
  // Returns a null value if the spec is not well-formed.
  static std::unique_ptr<RecordSerializer> Create(
      const RecordSpec &record_spec);

  // Serializes a token using the record spec, returns true only if the token
  // serializes correctly as per the record spec. For the input token, it
  // recursively traverses field_paths noted in the record_serializer and its
  // affix_serializers and concatenates serialized field content onto the
  // input fst.
  bool Serialize(const Token &token, MutableTransducer *fst) const;

 private:
  typedef MutableTransducer::Arc Arc;
  typedef Arc::StateId StateId;
  typedef Arc::Weight Weight;
  typedef fst::StringCompiler<Arc> StringCompiler;

  // Only used by the factory function Create.
  RecordSerializer();

  // Serializers for prefix specs in the specification.
  std::vector<std::unique_ptr<RecordSerializer>> prefix_serializers_;

  // Serializers for suffix specs in the specification.
  std::vector<std::unique_ptr<RecordSerializer>> suffix_serializers_;

  // Field path for the record_spec field.
  std::unique_ptr<FieldPath> field_path_;

  // String denoting the terminating field's name for the record spec.
  string field_name_;

  // Default value to be emitted when field is not set.
  string default_value_;

  // Pattern to be escaped - record_separator or escape_character.
  RE2 escape_re_;

  // Replacement string for escape pattern - prepended escape_character.
  string escape_replacement_;

  // String Compiler for making fsts from strings.
  StringCompiler string_compiler_;

  // Serializes a record, escaping record_separator and escape_character.
  // Also serializes various factorizations as parallel arcs into the FST.
  void SerializeRecord(string *value,
                       MutableTransducer *fst) const;

  // Assumes that the (non-repeated) field is set for the parent, and checks
  // that it corresponds to a scalar value. Also, in this case, adds an arc to
  // fst between states start and end, optionally adding a new state for end if
  // a sentinel is passed for end. It is an error to invoke this with a
  // repeated field.
  bool SerializeToFst(const google::protobuf::Message &parent,
                      const google::protobuf::FieldDescriptor &field,
                      MutableTransducer *fst) const;

  // Assumes that the (repeated) field is set for the parent, and checks that it
  // corresponds to a scalar value. Also, in this case, adds an arc to
  // fst between states start and end, optionally adding a new state for end if
  // a sentinel is passed for end. It is an error to invoke this with a
  // non-repeated field.
  bool SerializeToFstRepeated(const google::protobuf::Message &parent,
                              const google::protobuf::FieldDescriptor &field,
                              const int index,
                              MutableTransducer *fst) const;

  // Recursively serializes prefix and suffix records into respective
  // transducers using appropriate record serializers.
  bool SerializeAffixes(const Token &token,
                        MutableTransducer *prefix_fst,
                        MutableTransducer *suffix_fst) const;

  DISALLOW_COPY_AND_ASSIGN(RecordSerializer);
};

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_RECORD_SERIALIZER_H_
