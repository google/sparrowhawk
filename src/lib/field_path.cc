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
#include <sparrowhawk/field_path.h>

#include <memory>
#include <string>
using std::string;
#include <vector>
using std::vector;

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <sparrowhawk/string_utils.h>

namespace speech {
namespace sparrowhawk {

using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;

std::unique_ptr<FieldPath> FieldPath::Create(
    const Descriptor *root_type) {
  if (root_type == nullptr) {
    return nullptr;
  } else {
    std::unique_ptr<FieldPath> field_path(new FieldPath(root_type));
    return field_path;
  }
}

void FieldPath::Clear() {
  path_.clear();
}

bool FieldPath::Follow(const Message &base, const Message **parent,
                             const FieldDescriptor **field) const {
  if (base.GetDescriptor() != root_type_) {
    LOG(ERROR) << "Input Message to Follow is of type "
               << base.GetDescriptor()->name()
               << " while the field_path root type is " << root_type_->name();
    return false;
  }
  const Message *inner_message = &base;
  int size = path_.size();
  for (int i = 0; i < size - 1; ++i) {
    // Iterating over singular messages.
    inner_message = &inner_message->GetReflection()->GetMessage(*inner_message,
                                                                path_[i]);
  }
  *parent = inner_message;
  *field = path_[size - 1];
  return true;
}

// Helper function to go through the intermediate message fields.
bool FieldPath::TraverseIntermediateFields(
    std::vector<string> path,
    const google::protobuf::Descriptor **parent) {
  for (int i = 0; i < path.size() - 1; ++i) {
    string &field_name = path[i];
    const FieldDescriptor *field = (*parent)->FindFieldByName(field_name);
    if (field == nullptr) {
      LOG(ERROR) << (*parent)->full_name()
                 << " does not contain a field named '"
                 << field_name << "'.";
      return false;
    }
    if (field->type() != FieldDescriptor::TYPE_MESSAGE) {
      LOG(ERROR) << "Non-terminal field " << field->full_name()
                 << " is not a message.";
      return false;
    }
    path_.push_back(field);
    *parent = field->message_type();
  }
  return true;
}

// Helper function to parse the terminal scalar field.
bool FieldPath::ParseTerminalField(const string &terminal_field_name,
                                         const Descriptor *parent) {
  const FieldDescriptor *terminal_field =
      parent->FindFieldByName(terminal_field_name);
  if (terminal_field == nullptr) {
    LOG(ERROR) << parent->full_name() << " does not contain a field named '"
               << terminal_field_name << "'.";
    return false;
  } else if (terminal_field->type() == FieldDescriptor::TYPE_MESSAGE) {
    LOG(ERROR) << "Terminal field " << terminal_field->full_name()
               << " is a message.";
    return false;
  } else {
    path_.push_back(terminal_field);
  }
  return true;
}

bool FieldPath::Parse(const string &path_string) {
  // Overwriting without clearing the field_path is illegal.
  if (!IsEmpty()) {
    LOG(ERROR) << "Cannot overwrite field_path. Use Clear() to reset.";
    return false;
  }
  std::vector<string> path = SplitString(path_string, ".");
  const Descriptor *parent = root_type_;
  if (TraverseIntermediateFields(path, &parent) &&
      ParseTerminalField(path.back(), parent)) {
    return true;
  }
  Clear();
  return false;
}

}  // namespace sparrowhawk
}  // namespace speech
