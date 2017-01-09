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
// Utility to access specific subfields within a protocol buffer. FieldPath
// objects make subfields available via Follow().
//

#ifndef SPARROWHAWK_FIELD_PATH_H_
#define SPARROWHAWK_FIELD_PATH_H_

#include <memory>
#include <string>
using std::string;
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

namespace speech {
namespace sparrowhawk {

class FieldPath {
 public:
  // Creates and returns a FieldPath using a descriptor for the type of
  // messages we intend to Follow().
  // Returns a null value if the input pointer is null.
  static std::unique_ptr<FieldPath> Create(const google::protobuf::Descriptor *root_type);

  // Replaces this field_path with input path_string of type:
  //               (message_name.)*scalar_field_name
  // Returns false if an error occurs with either the format of the string or
  // with mismatches of type (e.g. a subfield of an integer) or label (i.e. an
  // index is supplied when the field is not repeated.)
  bool Parse(const string& path_string);

  // Clear all fields from path.
  void Clear();

  inline const google::protobuf::Descriptor *GetRootType() const { return root_type_; }

  // Number of fields on this path. Does not count the root as a field.
  inline int GetLength() const { return path_.size(); }

  // True if GetLength() == 0.
  inline bool IsEmpty() const { return GetLength() == 0; }

  // Follows the path starting from the given base message. *parent is filled
  // in with the immediate parent of the field at the end of the path and *field
  // is filled in with the terminal field's descriptor.
  // You can then use reflection to query the field value.
  //
  // Returns false only if the base message is incorrect (the only error that
  // can't be detected at parsing time); in this case *parent and *field are
  // unchanged.
  bool Follow(const google::protobuf::Message& base, const google::protobuf::Message **parent,
              const google::protobuf::FieldDescriptor **field) const;

 private:
  // Only used by the factory function Create.
  explicit FieldPath(const google::protobuf::Descriptor *root_type)
      : root_type_(root_type) {}

  // Parse intermediate message fields from input path. The parent is initially
  // root_type_ and is finally set to the penultimate field's descriptor.
  bool TraverseIntermediateFields(std::vector<string> path,
                                  const google::protobuf::Descriptor **parent);

  // Parse terminal field "field" with given parent descriptor into path_.
  bool ParseTerminalField(const string &terminal_field_name,
                          const google::protobuf::Descriptor *parent);

  std::vector<const google::protobuf::FieldDescriptor*> path_;
  const google::protobuf::Descriptor *root_type_;
};

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_FIELD_PATH_H_
