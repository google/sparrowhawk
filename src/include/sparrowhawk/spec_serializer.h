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
// Serializes a token based on a given spec for simple, fast verbalization.
// Iteratively serializes the styles in a class_spec which are concatenated as
// parallel arcs onto a transducer, which is returned as output.

#ifndef SPARROWHAWK_SPEC_SERIALIZER_H_
#define SPARROWHAWK_SPEC_SERIALIZER_H_

#include <map>
#include <memory>
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <google/protobuf/descriptor.h>
#include <thrax/grm-manager.h>
#include <sparrowhawk/items.pb.h>
#include <sparrowhawk/serialization_spec.pb.h>
#include <sparrowhawk/style_serializer.h>

namespace speech {
namespace sparrowhawk {

class Serializer {
 public:
  typedef fst::StdVectorFst MutableTransducer;

  // Creates and returns a Serializer from the serialize_spec by creating
  // style_serializers for all its style_specs and storing the name of the
  // semiotic class.
  // Returns a null value if the spec is not well-formed.
  static std::unique_ptr<Serializer> Create(
      const SerializeSpec &serialize_spec);

  // Serializes a token using the serialization spec, i.e. builds an fst
  // corresponding to the serialization of the token. Appends a label for the
  // semiotic class name at the front and then adds parallel arcs for the
  // different valid style_specs.
  MutableTransducer Serialize(const Token &token) const;

 private:
  typedef MutableTransducer::Arc Arc;
  typedef fst::StringCompiler<Arc> StringCompiler;

  // Only used by the factory function Create.
  Serializer() : string_compiler_(fst::StringTokenType::BYTE) {}

  // String Compiler for making fsts from strings.
  StringCompiler string_compiler_;

  // Map to store the serialization indexed by field descriptors.
  std::map<const google::protobuf::FieldDescriptor*,
  std::vector<std::unique_ptr<StyleSerializer>>> serializers_;

  DISALLOW_COPY_AND_ASSIGN(Serializer);
};

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_SPEC_SERIALIZER_H_
