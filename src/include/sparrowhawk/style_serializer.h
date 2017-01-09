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
// Iteratively serializes the records in a style_spec which are serially
// concatenated onto a transducer.
//
// Typically the serialized field content looks like
//           (<field_name>:<field_value>|)*
// where each unit is the serialization of a record.
//
// This is used by the Serializer for serializing all the styles in a given
// semiotic class. It constructs the StyleSerializer for each style in the
// class_spec permitted by the prohibited/requested values. Given a token it
// sequentially invokes the Serialize function of the styles in the class being
// serialized.

#ifndef SPARROWHAWK_STYLE_SERIALIZER_H_
#define SPARROWHAWK_STYLE_SERIALIZER_H_

#include <memory>
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <google/protobuf/message.h>
#include <thrax/grm-manager.h>
#include <sparrowhawk/items.pb.h>
#include <sparrowhawk/serialization_spec.pb.h>
#include <sparrowhawk/field_path.h>
#include <sparrowhawk/record_serializer.h>

namespace speech {
namespace sparrowhawk {

class StyleSerializer {
 public:
  typedef fst::StdVectorFst MutableTransducer;

  // Creates and returns a StyleSerializer from the style_spec by creating
  // record_serializers for all its record_specs and storing field_paths of
  // required and prohibited fields.
  // Returns a null value if the spec is not well-formed.
  static std::unique_ptr<StyleSerializer> Create(const StyleSpec &style_spec);

  // Serializes a token using the style spec, returns true only for valid
  // styles satisfying required/prohibited field constraints. If so, all the
  // records in the style are serialized onto the input fst.
  bool Serialize(const Token &token, MutableTransducer *serialization) const;

 private:
  // Only used by the factory function Create.
  StyleSerializer() {}

  // Populates record_serializers_ using style_spec.
  static bool CreateRecordSerializers(const StyleSpec &style_spec,
      const std::unique_ptr<StyleSerializer> &style_serializer);

  // Populates required_fields_ using style_spec.
  static bool SetRequiredFieldPaths(const StyleSpec &style_spec,
      const std::unique_ptr<StyleSerializer> &style_serializer);

  // Populates prohibited_fields_ using style_spec.
  static bool SetProhibitedFieldPaths(const StyleSpec &style_spec,
      const std::unique_ptr<StyleSerializer> &style_serializer);

  // Checks required_fields_ in token.
  bool CheckRequiredFields(const Token &token) const;

  // Checks prohibited_fields_ in token.
  bool CheckProhibitedFields(const Token &token) const;

  // FieldPaths to required fields in the specification.
  std::vector<std::vector<FieldPath>> required_fields_;

  // FieldPaths to prohibited fields in the specification.
  std::vector<FieldPath> prohibited_fields_;

  // Record serializers for the record specs in the style.
  std::vector<std::unique_ptr<RecordSerializer>> record_serializers_;

  // Takes as input a message and a target field path ending in a scalar field
  // to within the input message and returns true if the field at the end of the
  // path is set. It further assumes that all the intermediate messages are
  // non-repeated, although the terminating field itself may be repeated.
  bool IsFieldSet(const google::protobuf::Message &root,
                  const FieldPath &field_path) const;

  DISALLOW_COPY_AND_ASSIGN(StyleSerializer);
};

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_STYLE_SERIALIZER_H_
