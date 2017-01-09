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
#include <sparrowhawk/rule_system.h>

#include <google/protobuf/text_format.h>
#include <sparrowhawk/io_utils.h>
#include <sparrowhawk/logger.h>

namespace speech {
namespace sparrowhawk {

using fst::LabelLookAheadRelabeler;
using fst::StdArc;

RuleSystem::~RuleSystem() {
  std::map<string, LookaheadFst*>::iterator iter;
  for (iter = lookaheads_.begin(); iter != lookaheads_.end(); iter++) {
    delete iter->second;
  }
}

bool RuleSystem::LoadGrammar(const string& filename, const string& prefix) {
  // This is the contents of filename.
  string proto_string = IOStream::LoadFileToString(prefix + filename);
  if (!google::protobuf::TextFormat::ParseFromString(proto_string, &grammar_))
    return false;
  string grm_file = prefix + grammar_.grammar_file();
  grammar_name_ = grammar_.grammar_name();
  grm_.reset(new GrmManager);
  if (!grm_->LoadArchive(grm_file)) {
    LoggerError("Error loading archive \"%s\" from \"%s\"",
                grammar_name_.c_str(), grm_file.c_str());
    return false;
  }
  // Verifies that the rules named in the rule ordering all exist in the
  // grammar.
  for (int i = 0; i < grammar_.rules_size(); ++i) {
    Rule rule = grammar_.rules(i);
    if (grm_->GetFst(rule.main()) == NULL) {
      LoggerError("Rule \"%s\" not found in \"%s\"",
                  rule.main().c_str(), grammar_name_.c_str());
      return false;
    }
    if (rule.has_parens() && grm_->GetFst(rule.parens()) == NULL) {
      LoggerError("Rule \"%s\" not found in \"%s\"",
                  rule.parens().c_str(), grammar_name_.c_str());
      return false;
    }
    if (rule.has_redup() && grm_->GetFst(rule.redup()) == NULL) {
      LoggerError("Rule \"%s\" not found in \"%s\"",
                  rule.redup().c_str(), grammar_name_.c_str());
      return false;
    }
  }
  return true;
}

bool RuleSystem::ApplyRules(const Transducer& input,
                            MutableTransducer* output,
                            bool use_lookahead) const {
  MutableTransducer mutable_input(input);
  for (int i = 0; i < grammar_.rules_size(); ++i) {
    Rule rule = grammar_.rules(i);
    if (rule.has_redup()) {
      const string& redup_rule = rule.redup();
      MutableTransducer redup1;
      // Not an error if it fails.
      if (grm_->Rewrite(redup_rule, mutable_input, &redup1, "")) {
        MutableTransducer redup2(redup1);
        fst::Concat(redup1, &redup2);
        fst::Union(&mutable_input, redup2);
        fst::RmEpsilon(&mutable_input);
      }
    }
    const string& rule_name = rule.main();
    string parens_rule = rule.has_parens() ? rule.parens() : "";
    // Only use lookahead on non (M)PDT's
    bool success = true;
    if (parens_rule.empty()
        && use_lookahead) {
      std::map<string, LookaheadFst*>::iterator iter =
          lookaheads_.find(rule_name);
      LookaheadFst *lookahead_rule_fst;
      if (iter == lookaheads_.end()) {
        const Transducer *rule_fst = grm_->GetFst(rule_name);
        lookahead_rule_fst = new LookaheadFst(*rule_fst);
        lookaheads_[rule_name] = lookahead_rule_fst;
      } else {
        lookahead_rule_fst = iter->second;
      }
      LabelLookAheadRelabeler<StdArc>::Relabel(&mutable_input,
                                               *lookahead_rule_fst,
                                               false);
      fst::ComposeFst<StdArc> tmp_output(mutable_input,
                                             *lookahead_rule_fst);
      *output = tmp_output;
      if (output->NumStates() == 0) {
        success = false;
      }
      // Otherwise we just use the regular rewrite mechanism
    } else if (!grm_->Rewrite(rule_name,
                              mutable_input,
                              output,
                              parens_rule
                              )
        || output->NumStates() == 0) {
      success = false;
    }
    if (!success) {
      LoggerError("Application of rule \"%s\" failed", rule_name.c_str());
      return false;
    }
    mutable_input = *output;
  }
  // NB: We do NOT want to Project in this case because this will be the input
  // to the ProtobufParser, which needs the input-side epsilons in order to keep
  // track of positions in the input.
  fst::RmEpsilon(output);
  return true;
}

typedef fst::StringCompiler<StdArc> Compiler;
typedef fst::StringPrinter<StdArc> Printer;

bool RuleSystem::ApplyRules(const string& input,
                            string* output,
                            bool use_lookahead) const {
  Compiler compiler(fst::StringTokenType::BYTE);
  MutableTransducer input_fst, output_fst;
  if (!compiler.operator()(input, &input_fst)) {
    LoggerError("Failed to compile input string \"%s\"", input.c_str());
    return false;
  }
  if (!ApplyRules(input_fst, &output_fst, use_lookahead)) return false;
  MutableTransducer shortest_path;
  fst::ShortestPath(output_fst, &shortest_path);
  fst::Project(&shortest_path, fst::PROJECT_OUTPUT);
  fst::RmEpsilon(&shortest_path);
  Printer printer(fst::StringTokenType::BYTE);
  if (!printer.operator()(shortest_path, output)) {
    LoggerError("Failed to print output string");
    return false;
  }
  return true;
}

bool RuleSystem::ApplyRules(const Transducer& input,
                            string* output,
                            bool use_lookahead) const {
  MutableTransducer output_fst;
  if (!ApplyRules(input, &output_fst, use_lookahead)) return false;
  MutableTransducer shortest_path;
  fst::ShortestPath(output_fst, &shortest_path);
  fst::Project(&shortest_path, fst::PROJECT_OUTPUT);
  fst::RmEpsilon(&shortest_path);
  Printer printer(fst::StringTokenType::BYTE);
  if (!printer.operator()(shortest_path, output)) {
    LoggerError("Failed to print to output string");
    return false;
  }
  return true;
}

const Transducer* RuleSystem::FindRule(const string& name) const {
  return grm_->GetFst(name);
}


}  // namespace sparrowhawk
}  // namespace speech
