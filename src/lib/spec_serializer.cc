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
#include <sparrowhawk/spec_serializer.h>

#include <memory>
#include <vector>
using std::vector;

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <sparrowhawk/items.pb.h>
#include <sparrowhawk/serialization_spec.pb.h>
#include <sparrowhawk/style_serializer.h>

namespace speech {
namespace sparrowhawk {

using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Reflection;

namespace {

typedef Serializer::MutableTransducer MutableTransducer;
const char kClassSeparator[] = "|";

}  // namespace

std::unique_ptr<Serializer> Serializer::Create(
    const SerializeSpec &serialize_spec) {
  std::unique_ptr<Serializer> serializer(new Serializer());
  const Descriptor *token_descriptor = Token::descriptor();
  for (const ClassSpec &class_spec : serialize_spec.class_spec()) {
    const FieldDescriptor *class_descriptor =
      token_descriptor->FindFieldByName(class_spec.semiotic_class());
    if (class_descriptor == nullptr) {
      LOG(ERROR) << "Cannot find " << class_spec.semiotic_class()
                 << " field in Token proto";
      return nullptr;
    }
    std::vector<std::unique_ptr<StyleSerializer>> &styles =
        serializer->serializers_[class_descriptor];
    for (const StyleSpec &style_spec : class_spec.style_spec()) {
      auto style_serializer = StyleSerializer::Create(style_spec);
      if (style_serializer) {
        styles.push_back(std::move(style_serializer));
      } else {
        return nullptr;
      }
    }
  }
  return serializer;
}

MutableTransducer Serializer::Serialize(const Token &token) const {
  MutableTransducer fst;
  const Reflection *reflection = token.GetReflection();
  for (const auto &candidate_class : serializers_) {
    if (reflection->HasField(token, candidate_class.first)) {
      string_compiler_(candidate_class.first->name() + kClassSeparator,
                       &fst);
      MutableTransducer fst_styles;
      for (const auto &candidate_style : candidate_class.second) {
        MutableTransducer fst_style;
        fst_style.SetStart(fst_style.AddState());
        fst_style.SetFinal(0, 1);
        if (candidate_style->Serialize(token, &fst_style)) {
          Union(&fst_styles, fst_style);
        }
      }
      Concat(&fst, fst_styles);
    }
  }
  return fst;
}

}  // namespace sparrowhawk
}  // namespace speech
