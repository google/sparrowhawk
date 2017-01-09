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
#include <sparrowhawk/record_serializer.h>

#include <memory>
#include <string>
using std::string;
#include <vector>
using std::vector;

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <sparrowhawk/items.pb.h>
#include <sparrowhawk/serialization_spec.pb.h>
#include <sparrowhawk/field_path.h>
#include <sparrowhawk/string_utils.h>
#include <re2/re2.h>

namespace speech {
namespace sparrowhawk {

using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Reflection;
using google::protobuf::Message;

namespace {

const char kLabelSeparator[] = ":";
const char kEscapedEscape[] = R"(\\)";
const char kRecordSeparator[] = "|";

}  // namespace

RecordSerializer::RecordSerializer()
    : escape_re_(string("(") +
                 string(kEscapedEscape) +
                 string(R"()|(\)") +
                 string(kRecordSeparator) +
                 string(")")),
      escape_replacement_(string(kEscapedEscape) + string(R"(\0)")),
      string_compiler_(fst::StringTokenType::BYTE) {}

std::unique_ptr<RecordSerializer> RecordSerializer::Create(
    const RecordSpec &record_spec) {
  std::unique_ptr<RecordSerializer> record_serializer(new RecordSerializer());

  // Adds field path, label and default from the spec.
  record_serializer->field_path_ = FieldPath::Create(Token::descriptor());
  if (!record_serializer->field_path_->Parse(record_spec.field_path())) {
    LOG(ERROR) << "FieldPath failed to parse for record spec: "
               << record_spec.field_path();
    return nullptr;
  }
  if (record_spec.has_label()) {
    record_serializer->field_name_ = record_spec.label();
  } else {
    std::vector<string> vector_path =
        SplitString(record_spec.field_path(), ".");
    record_serializer->field_name_ = vector_path.back();
  }
  if (record_spec.has_default_value()) {
    record_serializer->default_value_ = record_spec.default_value();
    if (record_serializer->default_value_.empty()) {
      LOG(ERROR) << "Empty default value for record spec: "
                 << record_spec.field_path();
      return nullptr;
    }
  }

  // Adds record serializers for prefix and suffix records.
  for (const RecordSpec &prefix_spec : record_spec.prefix_spec()) {
    auto prefix_serializer = RecordSerializer::Create(prefix_spec);
    if (prefix_serializer) {
      record_serializer->prefix_serializers_.push_back(
          std::move(prefix_serializer));
    } else {
      return nullptr;
    }
  }
  for (const RecordSpec &suffix_spec : record_spec.suffix_spec()) {
    auto suffix_serializer = RecordSerializer::Create(suffix_spec);
    if (suffix_serializer) {
      record_serializer->suffix_serializers_.push_back(
          std::move(suffix_serializer));
    } else {
      return nullptr;
    }
  }
  return record_serializer;
}

void RecordSerializer::SerializeRecord(string *value,
                                       MutableTransducer *fst) const {
  // Adds a label for the field_name.
  string_compiler_(field_name_ + kLabelSeparator, fst);
  // Escapes record_separator and escape_character in value.
  RE2::GlobalReplace(value, escape_re_, escape_replacement_);
  MutableTransducer fst_value;
  string_compiler_(*value, &fst_value);
  Concat(fst, fst_value);
  // Adds a record_separator to terminate the record.
  MutableTransducer record_separator_fst;
  string_compiler_(kRecordSeparator, &record_separator_fst);
  Concat(fst, record_separator_fst);
}

bool RecordSerializer::SerializeToFstRepeated(const Message &parent,
                                              const FieldDescriptor &field,
                                              const int index,
                                              MutableTransducer *fst) const {
  const Reflection *parent_reflection = parent.GetReflection();
  string value;
  // TODO(drasha) Add better test coverage for different cases.
  switch (field.type()) {
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_STRING: {
      value = parent_reflection->GetRepeatedString(parent, &field, index);
      break;
    }
    case FieldDescriptor::TYPE_BOOL: {
      value = std::to_string(
          parent_reflection->GetRepeatedBool(parent, &field, index));
      break;
    }
    case FieldDescriptor::TYPE_DOUBLE: {
      value = std::to_string(
          parent_reflection->GetRepeatedDouble(parent, &field, index));
      break;
    }
    case FieldDescriptor::TYPE_FLOAT:
      value = std::to_string(
          parent_reflection->GetRepeatedFloat(parent, &field, index));
      break;
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_INT32: {
      value = std::to_string(
          parent_reflection->GetRepeatedInt32(parent, &field, index));
      break;
    }
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_INT64: {
      value = std::to_string(
          parent_reflection->GetRepeatedInt64(parent, &field, index));
      break;
    }
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32: {
      value = std::to_string(
          parent_reflection->GetRepeatedUInt32(parent, &field, index));
      break;
    }
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_UINT64: {
      value = std::to_string(
          parent_reflection->GetRepeatedUInt64(parent, &field, index));
      break;
    }
    case FieldDescriptor::TYPE_ENUM: {
      value = parent_reflection->GetEnum(parent, &field)->name();
      break;
    }
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE: {
      LOG(ERROR) << "Scalar expected for: " << field.full_name();
      return false;
    }
  }
  SerializeRecord(&value, fst);
  return true;
}

bool RecordSerializer::SerializeToFst(const Message &parent,
                                      const FieldDescriptor &field,
                                      MutableTransducer *fst) const {
  const Reflection *parent_reflection = parent.GetReflection();
  string value;
  // TODO(drasha) Add better test coverage for different cases.
  switch (field.type()) {
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_STRING: {
      value = parent_reflection->GetString(parent, &field);
      break;
    }
    case FieldDescriptor::TYPE_BOOL: {
      value = std::to_string(parent_reflection->GetBool(parent, &field));
      break;
    }
    case FieldDescriptor::TYPE_DOUBLE: {
      value = std::to_string(parent_reflection->GetDouble(parent, &field));
      break;
    }
    case FieldDescriptor::TYPE_FLOAT: {
      value = std::to_string(parent_reflection->GetFloat(parent, &field));
      break;
    }
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_INT32: {
      value = std::to_string(parent_reflection->GetInt32(parent, &field));
      break;
    }
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_INT64: {
      value = std::to_string(parent_reflection->GetInt64(parent, &field));
      break;
    }
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32: {
      value = std::to_string(parent_reflection->GetUInt32(parent, &field));
      break;
    }
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_UINT64: {
      value = std::to_string(parent_reflection->GetUInt64(parent, &field));
      break;
    }
    case FieldDescriptor::TYPE_ENUM: {
      value = parent_reflection->GetEnum(parent, &field)->name();
      break;
    }
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE: {
      LOG(ERROR) << "Scalar expected for: " << field.full_name();
      return false;
    }
  }
  SerializeRecord(&value, fst);
  return true;
}

bool RecordSerializer::SerializeAffixes(const Token &token,
                                        MutableTransducer *prefix_fst,
                                        MutableTransducer *suffix_fst) const {
  prefix_fst->SetStart(prefix_fst->AddState());
  prefix_fst->SetFinal(0, 1);
  for (const auto &prefix_serializer : prefix_serializers_) {
     if (!prefix_serializer->Serialize(token, prefix_fst)) {
       return false;
     }
  }
  suffix_fst->SetStart(suffix_fst->AddState());
  suffix_fst->SetFinal(0, 1);
  for (const auto &suffix_serializer : suffix_serializers_) {
     if (!suffix_serializer->Serialize(token, suffix_fst)) {
       return false;
     }
  }
  return true;
}

bool RecordSerializer::Serialize(const Token &token,
                                 MutableTransducer *fst) const {
  const Message *parent;
  const FieldDescriptor *field;
  if (!field_path_->Follow(token, &parent, &field)) {
    LOG(ERROR) << "FieldPath traversal failed for input Message "
               << token.DebugString();
    return false;
  }

  // Checks whether the field being serialized is not set (it is known that it
  // must be a valid field as it parses) in the token, and returns without
  // modifying the fst in this case.
  int field_size;
  bool repeated_field = field->label() == FieldDescriptor::LABEL_REPEATED;
  if (repeated_field) {
    field_size = parent->GetReflection()->FieldSize(*parent, field);
    if (field_size == 0) {
      return true;
    }
  } else if (!parent->GetReflection()->HasField(*parent, field)) {
    if (!default_value_.empty()) {
      string value = default_value_;
      MutableTransducer serialization;
      SerializeRecord(&value, &serialization);
      Concat(fst, serialization);
    }
    return true;
  }

  MutableTransducer prefix_fst, suffix_fst;
  if (!SerializeAffixes(token, &prefix_fst, &suffix_fst)) {
     return false;
  }
  std::vector<MutableTransducer> serializations;
  if (repeated_field) {
    if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
      LOG(ERROR) << "Intermediate repeated message not allowed in field_path, "
                 << "found: " << field->full_name();
      return false;
    } else {
      for (int i = 0; i < field_size; ++i) {
        Concat(fst, prefix_fst);
        MutableTransducer serialization;
        if (!SerializeToFstRepeated(*parent, *field, i, &serialization)) {
          return false;
        }
        Concat(fst, serialization);
        Concat(fst, suffix_fst);
      }
    }
  } else {
    Concat(fst, prefix_fst);
    MutableTransducer serialization;
    if (!SerializeToFst(*parent, *field, &serialization)) {
      return false;
    }
    Concat(fst, serialization);
    Concat(fst, suffix_fst);
  }
  return true;
}

}  // namespace sparrowhawk
}  // namespace speech
