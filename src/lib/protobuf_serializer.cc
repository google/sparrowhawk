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
#include <algorithm>
#include <map>

#include <google/protobuf/text_format.h>
#include <sparrowhawk/protobuf_serializer.h>

namespace speech {
namespace sparrowhawk {

typedef ProtobufSerializer::MutableTransducer MutableTransducer;
typedef MutableTransducer::Arc Arc;
typedef Arc::Weight Weight;
typedef ProtobufSerializer::StateId StateId;
typedef fst::MutableArcIterator<MutableTransducer> MutableArcIterator;

using fst::RmEpsilon;
using fst::kNoLabel;
using google::protobuf::Descriptor;
using google::protobuf::EnumValueDescriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;
using google::protobuf::Reflection;
using google::protobuf::Message;

ProtobufSerializer::ProtobufSerializer(const Message *message,
                                       MutableTransducer *fst)
    : message_(message),
      reflection_(message->GetReflection()),
      fst_(fst),
      initial_state_(0) {
}

ProtobufSerializer::ProtobufSerializer(const Message *message,
                                       MutableTransducer *fst,
                                       StateId state)
    : message_(message),
      reflection_(message->GetReflection()),
      fst_(fst),
      initial_state_(state) {
}

ProtobufSerializer::~ProtobufSerializer() {
}

namespace {

// Sort fields by their field number, which is a nice easy ordering on them.
struct OrderByFieldNumber {
  bool operator()(const FieldDescriptor *a, const FieldDescriptor *b) const {
    return a->number() < b->number();
  }
};

struct FindFieldByName {
  explicit FindFieldByName(const char *name) : name_(name) {}
  bool operator()(const FieldDescriptor *descriptor) const {
    return name_ == descriptor->name();
  }
  const string name_;
};

inline string PrintToString(const google::protobuf::MessageLite &message) {
  string output;
  message.SerializeToString(&output);
  return output;
}
}  // anonymous namespace

void ProtobufSerializer::SerializeToFst() {
  fst_->DeleteStates();
  fst_->AddState();
  fst_->SetStart(0);
  fst_->SetFinal(SerializeToFstInternal(), Weight::One());
  RmEpsilon(fst_);
}

StateId ProtobufSerializer::SerializeToFstInternal() {
  FieldDescriptorVector fields;
  reflection_->ListFields(*message_, &fields);
  if (fields.empty()) {
    return initial_state_;  // nothing to do
  }
  // add one extra state to link all the permutations up to.
  StateId finish = fst_->AddState();
  FieldDescriptorVector::const_iterator it =
      std::find_if(fields.begin(),
                   fields.end(),
                   FindFieldByName("preserve_order"));
  bool preserve_order = false;
  const FieldDescriptor *preserve_order_field = NULL;
  if (it != fields.end()) {
    preserve_order = reflection_->GetBool(*message_, *it);
    preserve_order_field = *it;
    // Check to make sure we have a field_order field. If we don't then set
    // truth to false.
    it = std::find_if(fields.begin(),
                      fields.end(),
                      FindFieldByName("field_order"));
    if (it == fields.end()) {
        LOG(WARNING) << "preserve_order is true,"
          << " but no field_order field defined for this message";
      preserve_order = false;
    }
  }
  if (preserve_order) {
    const Descriptor *descriptor = message_->GetDescriptor();
    FieldDescriptorVector fields2;
    for (int i = 0, n = reflection_->FieldSize(*message_, *it); i < n; ++i) {
      const string name = reflection_->GetRepeatedString(*message_, *it, i);
      const FieldDescriptor *field = descriptor->FindFieldByName(name);
      if (field == NULL) {
        // Shouldn't happen - would indicate that ProtobufParser had found a
        // field name which we can't find again now.
        LOG(ERROR) << "Couldn't find field " << name;
      } else {
        fields2.push_back(field);
      }
    }
    fields2.push_back(preserve_order_field);
    SerializePermutation(fields2);
    StripTrailingSpace(finish);
  } else {
    OrderByFieldNumber comp;
    int count = 0;
    do {
      SerializePermutation(fields);
      StripTrailingSpace(finish);
    // TODO(nielse): Remove hack for b/17619322: The complexity of this code is
    // factorial in the number of fields, therefore we limit it to 5040
    // permutations, which is enough for 7 fields (7! = 5040).
    // With more than seven fields we always have at least the standard
    // ordering based on field number.
    } while (std::next_permutation(fields.begin(), fields.end(), comp) &&
             ++count < 5040);
  }
  return finish;
}

void ProtobufSerializer::SerializePermutation(
    const FieldDescriptorVector &fields) {
  StateId state = initial_state_;
  for (int i = 0; i < fields.size(); ++i) {
    const FieldDescriptor *field = fields[i];
    // Never serialize field_order
    // TODO(rws): one inefficiency here is that we do all the sorting and
    // permuting with the field orders entry there, then we simply ignore
    // it. 'Twould be better an 'twere there to begin with. As it is it not
    // wrong, merely redundant.
    if (field->name() == "field_order") {
      continue;
    } else if (field->is_repeated()) {
      // repeated fields have to be handled slightly separately, and we
      // obviously don't generate permutations for them since order typically
      // is important.
      const int n = reflection_->FieldSize(*message_, field);
      for (int j = 0; j < n; ++j) {
        state = SerializeField(field, j, state);
      }
    } else {
      state = SerializeField(field, -1, state);
    }
  }
}

void ProtobufSerializer::StripTrailingSpace(StateId new_final_state) {
  for (int state = fst_->NumStates() - 1; state > 0; --state) {
    MutableArcIterator aiter(fst_, state);
    if (!aiter.Done() && aiter.Value().olabel != ' ') {
      Arc value = aiter.Value();
      fst_->AddArc(value.nextstate, Arc(0, 0, Weight::One(), new_final_state));
      return;
    }
  }
}

StateId ProtobufSerializer::SerializeField(const FieldDescriptor *field,
                                           int index,
                                           StateId state) {
  if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
    state = SerializeString(field->name() + " { ", state);
    const Message *submessage;
    if (index == -1) {
      submessage = &reflection_->GetMessage(*message_, field);
    } else {
      submessage = &reflection_->GetRepeatedMessage(*message_, field, index);
    }
    ProtobufSerializer serializer(submessage, fst_, state);
    state = serializer.SerializeToFstInternal();
    return SerializeString(" } ", state);
  } else {
    const StateId initial_state = state;
    const string name = field->name();
    state = SerializeString(name + ": ", state);
    string value;
    if (field->type() == FieldDescriptor::TYPE_STRING) {
      // Special handling for string fields, where we don't escape internal
      // quotes with backslashes. This can't be disabled in TextFormat::Printer.
      if (index == -1) {
        value = "\"" + reflection_->GetString(*message_, field) + "\"";
      } else {
        value = "\"" +
            reflection_->GetRepeatedString(*message_, field, index) + "\"";
      }
    } else {
      google::protobuf::TextFormat::Printer printer;
      printer.SetUseUtf8StringEscaping(true);
      printer.PrintFieldValueToString(*message_, field, index, &value);
    }
    StateId lastend = SerializeString(value, state);
    lastend = SerializeChar(' ', lastend);

    // Serialize morphosyntatic_features fields optionally, so languages which
    // don't use them can still consume inputs with them.
    if (field->name() == "morphosyntactic_features") {
      fst_->AddArc(initial_state, Arc(0, 0, 1.0, lastend));
    }
    return lastend;
  }
}

StateId ProtobufSerializer::SerializeString(const string &str, StateId state) {
  return SerializeString(str, state, false);
}

StateId ProtobufSerializer::SerializeString(const string &str,
                                            StateId state,
                                            bool optional_quotes) {
  // TODO(pebden): This assumes a byte-oriented FST. We could generalize it
  // to others as well if needed.
  StateId first_state = state;
  for (int i = 0; i < str.size(); ++i) {
    StateId next_state = fst_->AddState();
    fst_->AddArc(state, Arc(str[i], str[i], Weight::One(), next_state));
    if (optional_quotes && (i == 0 || i == str.size() - 1) && str[i] == '"') {
      fst_->AddArc(state, Arc(0, 0, Weight::One(), next_state));
    }
    state = next_state;
  }
  // Add an alternate serialization without the beginning/ending quotes.
  // They're optional, but must be taken together or not at all.
  if (!optional_quotes &&
      str.size() >= 2 && str[0] == '"' && str[str.size() - 1] == '"') {
    StateId end_state = SerializeString(str.substr(1, str.size() - 2),
                                        first_state);
    fst_->AddArc(end_state, Arc(0, 0, Weight::One(), state));
  }
  return state;
}

StateId ProtobufSerializer::SerializeChar(char c, StateId state) {
  // TODO(pebden): Same comments as above apply re byte-oriented FSTs.
  const StateId next_state = fst_->AddState();
  fst_->AddArc(state, Arc(c, c, Weight::One(), next_state));
  return next_state;
}

string ProtobufSerializer::SerializeToString() const {
  return PrintToString(*message_);
}

}  // namespace sparrowhawk
}  // namespace speech
