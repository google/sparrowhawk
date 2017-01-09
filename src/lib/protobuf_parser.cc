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
#include <sparrowhawk/protobuf_parser.h>

#include <algorithm>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <sparrowhawk/items.pb.h>
#include <sparrowhawk/logger.h>
#include <sparrowhawk/numbers.h>

namespace speech {
namespace sparrowhawk {

using google::protobuf::Descriptor;
using google::protobuf::EnumValueDescriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;
using google::protobuf::Reflection;

ProtobufParser::ProtobufParser(const Transducer *fst)
    : fst_(fst),
      state_(fst->Start()),
      last_state_(fst->Start()),
      ilabel_(0),
      olabel_(0),
      token_start_(0),
      last_token_end_(0) {}

ProtobufParser::~ProtobufParser() {}

bool ProtobufParser::ParseTokensFromFST(Utterance *utt,
                                        bool set_semiotic_class,
                                        bool fix_lookahead) {
  if (state_ == fst::kNoStateId) {
    LoggerError("Attempt to parse tokens from invalid state.");
    return false;
  }
  string label;
  while (ConsumeLabel(&label)) {
    if (label != "tokens") {
      LoggerError("Unknown top-level label [%s]", label.c_str());
      LogError();
      return false;
    } else if (fix_lookahead) {
      FixLookahead(utt);
    }
    NextState();  // Consume opening brace.
    Token *token = utt->mutable_linguistic()->add_tokens();
    if (!ParseMessage(false, token)) {
      LogError();
      return false;
    }
    UpdateTokenIndices(token, set_semiotic_class, fix_lookahead);
  }
  return true;
}

bool ProtobufParser::ParseMessageFromFST(google::protobuf::Message *message) {
  return ParseMessage(true, message);
}

bool ProtobufParser::ConsumeLabel(string *label) {
  label->clear();
  while (NextState()) {
    if (((olabel_ == ' ' && label->empty()) || olabel_ == 0)) {
      continue;
    } else if (isalpha(olabel_) || olabel_ == '_') {
      label->push_back(olabel_);
    } else if (olabel_ == '}' && label->empty()) {
      label->push_back(olabel_);
      break;
    } else {
      if (olabel_ != ':' && olabel_ != ' ') {
        PrevState();
      }
      break;
    }
  }
  ConsumeWhitespace();
  return !label->empty();
}

void ProtobufParser::ConsumeWhitespace() {
  while (NextState()) {
    if (olabel_ != ' ' && olabel_ != 0) {
      PrevState();
      break;
    }
  }
}

bool ProtobufParser::NextState() {
  ArcIterator arc(*fst_, state_);
  if (arc.Done()) {
    return false;
  }
  ilabel_ = arc.Value().ilabel;
  olabel_ = arc.Value().olabel;
  if (ilabel_) {
    // Don't aggregate leading whitespace against a token.
    if (ilabel_ == ' ' && token_name_.empty()) {
      ++token_start_;
    } else {
      token_name_.push_back(ilabel_);
    }
  }
  last_state_ = state_;
  state_ = arc.Value().nextstate;
  return true;
}

void ProtobufParser::PrevState() {
  state_ = last_state_;
  // Have to undo any input aggregation we might have done.
  if (ilabel_) {
    if (ilabel_ == ' ' && token_name_.empty()) {
      --token_start_;
    } else if (!token_name_.empty()) {
      token_name_.erase(token_name_.size() - 1);
    }
  }
}

bool ProtobufParser::ParseMessage(bool eof_allowed, Message *message) {
  const Descriptor *descriptor = message->GetDescriptor();
  const Reflection *reflection = message->GetReflection();
  string label;
  // Record of the order in which the fields came in
  std::vector<string> field_order;
  while (true) {
    if (!ConsumeLabel(&label)) {
      if (eof_allowed) {
        return RecordFieldOrder(message, field_order);
      } else {
        LoggerError("Failed to consume field label.");
        return false;
      }
    }
    if (label == "}") {
      return RecordFieldOrder(message, field_order);  // End of message
    }
    // Disallow field_order in parsing (and of course in serialization). We
    // don't want the grammar writer to specify this and have the parser add
    // additional information since therein massive confusion lies.
    if (label == "field_order") {
      LoggerError("field_order should not be specified in the input");
      return false;
    }
    const FieldDescriptor *field_descriptor =
        descriptor->FindFieldByName(label);
    if (field_descriptor == NULL) {
      LoggerError("Unknown field: [%s]", label.c_str());
      return false;
    }
    field_order.push_back(label);
    if (field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      NextState();  // consume opening brace.
      Message *submessage;
      if (field_descriptor->is_repeated()) {
        submessage = reflection->AddMessage(message, field_descriptor);
      } else {
        submessage = reflection->MutableMessage(message, field_descriptor);
      }
      // EOF is never allowed for nested messages.
      if (!ParseMessage(false, submessage)) {
        return false;
      }
    } else {
      string value;
      if (!ParseFieldValue(&value)) {
        return false;
      }
      SetField(message, reflection, field_descriptor, value);
    }
    ConsumeWhitespace();
  }
  // Shouldn't be possible to get here.
  return false;
}

bool ProtobufParser::ParseFieldValue(string *value) {
  while (NextState()) {
    if (olabel_ == '"') {
      return ParseQuotedFieldValue(false, value);
    } else if (olabel_ == ' ') {
      return true;
    } else if (olabel_ == '}') {
      PrevState();  // Unconsume the brace, ParseMessage wants it.
      return true;
    } else if (olabel_) {
      value->push_back(olabel_);
    }
  }
  LoggerError("Unexpected EOF while reading field");
  return false;
}

bool ProtobufParser::ParseQuotedFieldValue(bool ignore_backslashes,
                                           string *value) {
  const StateId initial_state = state_;
  bool last_backslash = false;
  while (NextState()) {
    if (olabel_ == '\\' && !last_backslash) {
      last_backslash = !ignore_backslashes;
    } else if (olabel_ == '"' && !last_backslash) {
      return true;  // Unescaped quote finishes the field.
    } else if (olabel_) {
      value->push_back(olabel_);
      last_backslash = false;
    }
  }
  if (!ignore_backslashes) {
    LoggerWarn("Failure reading field; will try ignoring backslashes.");
    value->clear();
    state_ = initial_state;
    return ParseQuotedFieldValue(true, value);
  } else {
    LoggerError("Unexpected EOF while reading field");
    return false;
  }
}

bool ProtobufParser::RecordFieldOrder(Message *message,
                                      const std::vector<string> &field_order) {
  const Descriptor *descriptor = message->GetDescriptor();
  const Reflection *reflection = message->GetReflection();
  const FieldDescriptor *preserve_field =
      descriptor->FindFieldByName("preserve_order");
  const FieldDescriptor *order_field =
      descriptor->FindFieldByName("field_order");
  if (preserve_field == NULL ||
      !reflection->GetBool(*message, preserve_field)) {
    return true;  // Not an error; just nothing to do.
  } else if (order_field == NULL || !order_field->is_repeated()) {
    // This is an error: we specified that we wanted to preserve the order, but
    // we don't have repeated field_order field to store the fields in.
    LoggerError("preserve_order requested but no field_order repeated"
                "string fields");
    return false;
  }
  for (int i = 0; i < field_order.size(); ++i) {
    reflection->AddString(message, order_field, field_order[i]);
  }
  return true;
}

// Used to work out the length of a utf8 string in Unicode chars
// (instead of just bytes).
struct CountUtf8NonContinuationChars {
  bool operator()(unsigned char c) const { return c < 192; }
};

void ProtobufParser::UpdateTokenIndices(Token *token, bool set_semiotic_class,
                                        bool fix_lookahead) {
  // Work out the number of Unicode chars in the token.
  int unicode_size = std::count_if(token_name_.begin(), token_name_.end(),
                                   CountUtf8NonContinuationChars());
  // Strip trailing whitespace from token.
  const int token_end_original = token_start_ + unicode_size;
  const int last_not_space = token_name_.find_last_not_of(' ');
  if (last_not_space != string::npos) {
    unicode_size -= token_name_.size() - last_not_space - 1;
    token_name_.erase(last_not_space + 1);
  }
  // Update position indices on token.
  const int token_end = token_start_ + unicode_size;
  token->set_start_index(token_start_);
  last_token_end_ = token_end - 1;  // -1 because points at last char
  token->set_end_index(last_token_end_);
  if (token->has_name()) {
    token->set_wordid(token->name());
  } else {
    token->set_name(token_name_);
    if (set_semiotic_class) token->set_type(Token::SEMIOTIC_CLASS);
  }
  token_start_ = token_end_original;
  last_token_name_ = token_name_;
  token_name_.clear();
}

void ProtobufParser::FixLookahead(Utterance *utt) {
  // Fixes problem caused by lookahead FSTs whereby input labels are associated
  // with the previous token when the two tokens aren't space separated -
  // the first character of one token gets aggregated against the previous one.
  // We simply move that character forward to the current token.
  if (last_token_end_ == token_start_ - 1
      && utt->linguistic().tokens_size() > 0) {
    Token *prev = utt->mutable_linguistic()->mutable_tokens(
        utt->linguistic().tokens_size() - 1);
    // Don't change the actual token's name if it's been output from the grammar
    string *prev_name = last_token_name_ == prev->name() ? prev->mutable_name()
                                                         : &last_token_name_;
    if (!prev_name->empty()) {
      token_name_.insert(0, 1, *prev_name->rbegin());
      prev_name->erase(prev_name->size() - 1);
      prev->set_end_index(prev->end_index() - 1);
      token_start_--;
    }
  }
}

void ProtobufParser::LogError() {
  // Log the entire string that we've failed to parse.
  string message;
  state_ = fst_->Start();
  while (NextState()) {
    if (olabel_) {
      message.push_back(olabel_);
    }
  }
  LoggerError("Full input: [%s]", message.c_str());
}

// Helper macro for SetField function, pinched from text_format.cc.
// If the field is repeated, we must use Add***, else Set***.
#define SET_FIELD(type, value)                           \
  if (field->is_repeated()) {                            \
    return reflection->Add##type(message, field, value); \
  } else {                                               \
    return reflection->Set##type(message, field, value); \
  }

void ProtobufParser::SetField(google::protobuf::Message *message,
                              const google::protobuf::Reflection *reflection,
                              const google::protobuf::FieldDescriptor *field,
                              const string &value) const {
  // Awkwardly, there is no nice "set the field to this string" function,
  // we have to invent our own switch statement. Joy.
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_STRING:
      SET_FIELD(String, value);
    case FieldDescriptor::CPPTYPE_BOOL:
      SET_FIELD(Bool, value == "true");
    case FieldDescriptor::CPPTYPE_FLOAT: {
      float value_float;
      bool value_set = safe_strtof(value, &value_float);
      if (value_set) {
        SET_FIELD(Float, value_float);
      } else {
        LoggerError("Unable to convert string to float.");
      }
      break;
    }
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      double value_double;
      bool value_set = safe_strtod(value, &value_double);
      if (value_set) {
        SET_FIELD(Float, value_double);
      } else {
        LOG(ERROR) << "Unable to convert string to double.";
      }
      break;
    }
    case FieldDescriptor::CPPTYPE_INT32: {
      int32 value_int32;
      bool value_set = safe_strto32(value, &value_int32);
      if (value_set) {
        SET_FIELD(Int32, value_int32);
      } else {
        LoggerError("Unable to convert string to int32.");
      }
      break;
    }
    case FieldDescriptor::CPPTYPE_INT64: {
      int64 value_int64;
      bool value_set = safe_strto64(value, &value_int64);
      if (value_set) {
        SET_FIELD(Int64, value_int64);
      } else {
        LoggerError("Unable to convert string to int64.");
      }
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      const Descriptor *descriptor = message->GetDescriptor();
      const EnumValueDescriptor *enum_desc =
          descriptor->FindEnumValueByName(value);
      if (enum_desc == NULL) {
        LoggerError("Unknown enumeration value %d", value.c_str());
        return;
      }
      if (field->is_repeated()) {
        return reflection->AddEnum(message, field, enum_desc);
      } else {
        return reflection->SetEnum(message, field, enum_desc);
      }
    }
    case FieldDescriptor::CPPTYPE_MESSAGE: {
      LoggerError("Can't set message fields with ProtobufParser::SetField.");
      return;
    }
    default: {
      LoggerError("Unknown field type %s", field->cpp_type());
      return;
    }
  }
}

}  // namespace sparrowhawk
}  // namespace speech
