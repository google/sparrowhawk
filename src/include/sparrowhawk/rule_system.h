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
// A rule system consists of a cascaded set of grammar targets defined by
// Thrax. See rule_order.proto for a description of what each rule complex can
// contain.
#ifndef SPARROWHAWK_RULE_SYSTEM_H_
#define SPARROWHAWK_RULE_SYSTEM_H_

#include <map>
#include <memory>
#include <string>
using std::string;

#include <fst/compat.h>
#include <google/protobuf/text_format.h>
#include <thrax/grm-manager.h>
#include <sparrowhawk/rule_order.pb.h>

namespace speech {
namespace sparrowhawk {

using fst::LabelLookAheadRelabeler;
using fst::StdArcLookAheadFst;
using fst::StdILabelLookAheadFst;
using fst::StdOLabelLookAheadFst;
using thrax::GrmManager;

typedef fst::Fst<fst::StdArc> Transducer;
typedef fst::VectorFst<fst::StdArc>  MutableTransducer;

typedef StdILabelLookAheadFst LookaheadFst;

class RuleSystem {
 public:
  RuleSystem() { }
  ~RuleSystem();

  // Loads a protobuf containing the filename of the grammar far
  // and the rule specifications as defined in rule_order.proto.
  bool LoadGrammar(const string& filename, const string& prefix);

  // This one returns the epsilon-free output projection of all
  // paths. use_lookahead constructs a lookahead FST for the composition.
  bool ApplyRules(const Transducer& input,
                  MutableTransducer* output,
                  bool use_lookahead) const;

  // These two return the string of the shortest path.
  bool ApplyRules(const string& input,
                  string* output,
                  bool use_lookahead) const;

  bool ApplyRules(const Transducer& input,
                  string* output,
                  bool use_lookahead) const;

  // Find the named transducer or NULL if nonexistent.
  const Transducer* FindRule(const string& name) const;

  const string& grammar_name() const { return grammar_name_; }

 private:
  Grammar grammar_;
  string grammar_name_;
  std::unique_ptr<GrmManager> grm_;
  // Precomputed lookahead transducers
  mutable std::map<string, LookaheadFst*> lookaheads_;
};

}  // namespace sparrowhawk
}  // namespace speech

#endif  // SPARROWHAWK_RULE_SYSTEM_H_
