// Copyright 2017-2020 The Verible Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VERIBLE_VERILOG_TOOLS_KYTHE_KYTHE_FACTS_EXTRACTOR_H_
#define VERIBLE_VERILOG_TOOLS_KYTHE_KYTHE_FACTS_EXTRACTOR_H_

#include <string>
#include <utility>

#include "verilog/tools/kythe/indexing_facts_tree.h"
#include "verilog/tools/kythe/indexing_facts_tree_context.h"
#include "verilog/tools/kythe/indexing_facts_tree_extractor.h"
#include "verilog/tools/kythe/kythe_facts.h"

namespace verilog {
namespace kythe {

// Streamable printing class for kythe facts.
// Usage: stream << KytheFactsPrinter(*tree_root);
class KytheFactsPrinter {
 public:
  explicit KytheFactsPrinter(const IndexingFactNode& root) : root_(root) {}

  std::ostream& Print(std::ostream&) const;

 private:
  // The root of the indexing facts tree to extract kythe facts from.
  const IndexingFactNode& root_;
};

// Responsible for traversing IndexingFactsTree and processing its different
// nodes to produce kythe indexing facts.
class KytheFactsExtractor {
 public:
  explicit KytheFactsExtractor(absl::string_view file_path,
                               std::ostream* stream)
      : file_path_(file_path), stream_(stream) {}

  void Visit(const IndexingFactNode&);

 private:
  // Container with a stack of VNames to hold context of VNames during traversal
  // of an IndexingFactsTree.
  // This is used to generate to VNames inside the current scope.
  // e.g.
  // module foo();
  //  wire x; ==> x#variable#foo#module
  // endmodule: foo
  //
  // module bar();
  //  wire x; ==> x#variable#bar#module
  // endmodule: bar
  class VNameContext : public verible::AutoPopStack<const VName*> {
   public:
    typedef verible::AutoPopStack<const VName*> base_type;

    // member class to handle push and pop of stack safely
    using AutoPop = base_type::AutoPop;

    // returns the top VName of the stack
    const VName& top() const { return *ABSL_DIE_IF_NULL(base_type::top()); }
  };

  // Container with a stack of Scopes to hold the accessible scopes during
  // traversing an Indexing Facts Tree.
  // This is used to get the definitions of some reference.
  //
  // This is modified during tree traversal because in case of entering new
  // scope the new scope is resolved first and after that it's added to the
  // containing scope and the next scope is being analyzed.
  class ScopeContext : public verible::AutoPopStack<std::vector<VName>*> {
   public:
    typedef verible::AutoPopStack<std::vector<VName>*> base_type;

    // member class to handle push and pop of stack safely
    using AutoPop = base_type::AutoPop;

    // returns the top VName of the stack
    std::vector<VName>& top() { return *ABSL_DIE_IF_NULL(base_type::top()); }

    // Search function to get the VName of a definitions of some reference.
    // It loops over the scopes in reverse order and loops over every scope in
    // reverse order to find a definition for the variable with given prefix
    // signature.
    // e.g
    // {
    //    bar#module,
    //    foo#module,
    // }
    // {
    //    other scope,
    // }
    // Given bar#module it return the whole VName of that definition.
    // And if more than one match is found the first would be returned.
    const VName* SearchForDefinition(std::string prefix) {
      for (const auto& scope : verible::make_range(rbegin(), rend())) {
        for (const VName& vname :
             verible::make_range(scope->rbegin(), scope->rend())) {
          if (vname.signature.find(prefix) != std::string::npos) {
            return &vname;
          }
        }
      }
      return nullptr;
    }
  };

  // Extracts kythe facts from file node and returns it VName.
  VName ExtractFileFact(const IndexingFactNode&);

  // Extracts kythe facts from module instance node and returns it VName.
  VName ExtractModuleInstanceFact(const IndexingFactNode&);

  // Extracts kythe facts from module node and returns it VName.
  VName ExtractModuleFact(const IndexingFactNode&);

  // Extracts kythe facts from module port node and returns its VName.
  VName ExtractVariableDefinition(const IndexingFactNode& node);

  // Extracts kythe facts from a module port reference node and returns its
  // VName.
  VName ExtractVariableReference(const IndexingFactNode& node);

  // Generates an anchor VName for kythe.
  VName PrintAnchorVName(const Anchor&, absl::string_view);

  // Appends the signatures of previous containing scope vnames to make
  // signatures unique relative to scopes.
  std::string CreateScopeRelativeSignature(absl::string_view);

  // The verilog file name which the facts are extracted from.
  std::string file_path_;

  // Keeps track of VNames of ancestors as the visitor traverses the facts
  // tree.
  VNameContext vnames_context_;

  // Keeps track of scopes and definitions inside the scopes of ancestors as the
  // visitor traverses the facts tree.
  ScopeContext scope_context_;

  // Output stream for capturing, redirecting, testing and verifying the
  // output.
  std::ostream* stream_;
};

// Creates the signature for module names.
std::string CreateModuleSignature(absl::string_view);

// Creates the signature for module instantiations.
std::string CreateVariableSignature(absl::string_view);

std::ostream& operator<<(std::ostream&, const KytheFactsPrinter&);

}  // namespace kythe
}  // namespace verilog

#endif  // VERIBLE_VERILOG_TOOLS_KYTHE_KYTHE_FACTS_EXTRACTOR_H_
