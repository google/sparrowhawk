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
#include <sparrowhawk/style_serializer.h>

#include <memory>
#include <string>
using std::string;
#include <vector>
using std::vector;

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <sparrowhawk/items.pb.h>
#include <sparrowhawk/serialization_spec.pb.h>
#include <sparrowhawk/field_path.h>
#include <sparrowhawk/record_serializer.h>
#include <sparrowhawk/string_utils.h>

namespace speech {
namespace sparrowhawk {

using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Reflection;
using google::protobuf::TextFormat;
using google::protobuf::Message;

bool StyleSerializer::CreateRecordSerializers(
    const StyleSpec &style_spec,
    const std::unique_ptr<StyleSerializer> &style_serializer) {
  for (const RecordSpec &record_spec : style_spec.record_spec()) {
    auto record_serializer = RecordSerializer::Create(record_spec);
    if (record_serializer) {
      style_serializer->record_serializers_.push_back(
          std::move(record_serializer));
    } else {
      return false;
    }
  }
  return true;
}

bool StyleSerializer::SetRequiredFieldPaths(
    const StyleSpec &style_spec,
    const std::unique_ptr<StyleSerializer> &style_serializer) {
  const Descriptor *token_descriptor = Token::descriptor();
  for (const string &required_fields : style_spec.required_fields()) {
    std::vector<FieldPath> any_of;
    for (const auto &required_field :
         SplitString(required_fields, "|")) {
      std::unique_ptr<FieldPath> field_path =
          FieldPath::Create(token_descriptor);
      any_of.push_back(*field_path);
      if (!any_of.back().Parse(required_field)) {
        LOG(ERROR) << "FieldPath failed to parse for required field: "
                   << required_field;
        return false;
      }
    }
    style_serializer->required_fields_.push_back(std::move(any_of));
  }
  return true;
}

bool StyleSerializer::SetProhibitedFieldPaths(
    const StyleSpec &style_spec,
    const std::unique_ptr<StyleSerializer> &style_serializer) {
  const Descriptor *token_descriptor = Token::descriptor();
  for (const string &prohibited_field : style_spec.prohibited_fields()) {
    std::vector<FieldPath> &prohibited_fields =
        style_serializer->prohibited_fields_;
    std::unique_ptr<FieldPath> field_path =
        FieldPath::Create(token_descriptor);
    prohibited_fields.push_back(*field_path);
    if (!prohibited_fields.back().Parse(prohibited_field)) {
      LOG(ERROR) << "FieldPath failed to parse for prohibited field: "
                 << prohibited_field;
      return false;
    }
  }
  return true;
}

std::unique_ptr<StyleSerializer> StyleSerializer::Create(
    const StyleSpec &style_spec) {
  std::unique_ptr<StyleSerializer> style_serializer(new StyleSerializer());
  if (!CreateRecordSerializers(style_spec, style_serializer) ||
      !SetRequiredFieldPaths(style_spec, style_serializer) ||
      !SetProhibitedFieldPaths(style_spec, style_serializer)) {
    return nullptr;
  }
  return style_serializer;
}

bool StyleSerializer::IsFieldSet(const Message &root,
                                 const FieldPath &field_path) const {
  const Message *parent;
  const FieldDescriptor *field;
  if (!field_path.Follow(root, &parent, &field)) {
    LOG(ERROR) << "FieldPath traversal failed for input Message "
               << root.DebugString();
    return false;
  }
  const Reflection *parent_reflection = parent->GetReflection();
  if (field->label() == FieldDescriptor::LABEL_REPEATED) {
    // The field is assumed to be a scalar here.
    if (parent_reflection->FieldSize(*parent, field) == 0) {
      return false;
    }
  } else if (!parent_reflection->HasField(*parent, field)) {
    return false;
  }
  return true;
}

bool StyleSerializer::CheckRequiredFields(const Token &token) const {
  for (const std::vector<FieldPath> &field_paths : required_fields_) {
    bool found = false;
    for (const FieldPath &field_path : field_paths) {
      if (IsFieldSet(token, field_path)) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

bool StyleSerializer::CheckProhibitedFields(const Token &token) const {
  for (const FieldPath &field_path : prohibited_fields_) {
    if (IsFieldSet(token, field_path)) {
      return false;
    }
  }
  return true;
}

bool StyleSerializer::Serialize(const Token &token,
                                MutableTransducer *serialization) const {
  if (!CheckRequiredFields(token) || !CheckProhibitedFields(token)) {
    return false;
  }
  for (const auto &record_serializer : record_serializers_) {
    if (!record_serializer->Serialize(token, serialization)) {
      LOG(ERROR) << "Record serialization failure for token " + token.name();
      return false;
    }
  }
  return true;
}


}  // namespace sparrowhawk
}  // namespace speech
