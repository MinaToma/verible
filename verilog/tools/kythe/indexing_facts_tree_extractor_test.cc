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

#include "indexing_facts_tree_extractor.h"
#include "common/text/concrete_syntax_tree.h"
#include "gtest/gtest.h"
#include "verilog/analysis/verilog_analyzer.h"

#undef EXPECT_OK
#define EXPECT_OK(value) EXPECT_TRUE((value).ok())

namespace verilog {
namespace kythe {
namespace {

typedef IndexingFactNode T;

bool FactsTreeEqual(const IndexingNodeData& lhs, const IndexingNodeData& rhs) {
  auto left = lhs, right = rhs;
  bool equal = left.GetIndexingFactType() == right.GetIndexingFactType();

  const auto &lAnchors = left.Anchors(), rAnchors = right.Anchors();
  equal &= lAnchors.size() == rAnchors.size();
  for (size_t i = 0; i < lAnchors.size(); i++) {
    equal &= lAnchors[i].StartLocation() == rAnchors[i].StartLocation() &&
             lAnchors[i].EndLocation() == rAnchors[i].EndLocation() &&
             lAnchors[i].Value() == rAnchors[i].Value();
  }

  return equal;
}

TEST(EmptyCSTTest, FactsTreeExtractor) {
  std::string code_text = "";
  VerilogAnalyzer analyzer(code_text, "");
  EXPECT_OK(analyzer.Analyze());
  const auto& root = analyzer.Data().SyntaxTree();
  const absl::string_view file_name = "verilog.v";

  const auto expected =
      T({{Anchor(file_name, 0, code_text.size())}, IndexingFactType ::kFile});

  const auto& facts_tree = BuildIndexingFactsTree(
      ABSL_DIE_IF_NULL(root), analyzer.Data().Contents(), file_name);

  const auto result_pair = DeepEqual(facts_tree, expected, FactsTreeEqual);
  EXPECT_EQ(result_pair.left, nullptr) << *result_pair.left;
  EXPECT_EQ(result_pair.right, nullptr) << *result_pair.right;
}

TEST(EmptyModuleTest, FactsTreeExtractor) {
  std::string code_text = "module foo; endmodule: foo";
  VerilogAnalyzer analyzer(code_text, "");
  EXPECT_OK(analyzer.Analyze());
  const auto& root = analyzer.Data().SyntaxTree();
  const absl::string_view file_name = "verilog.v";

  const auto expected =
      T({{Anchor(file_name, 0, code_text.size())}, IndexingFactType ::kFile},
        T({{Anchor(absl::string_view("foo"), 7, 10),
            Anchor(absl::string_view("foo"), 23, 26)},
           IndexingFactType::kModule}));

  const auto& facts_tree = BuildIndexingFactsTree(
      ABSL_DIE_IF_NULL(root), analyzer.Data().Contents(), file_name);

  const auto result_pair = DeepEqual(facts_tree, expected, FactsTreeEqual);
  EXPECT_EQ(result_pair.left, nullptr) << *result_pair.left;
  EXPECT_EQ(result_pair.right, nullptr) << *result_pair.right;
}

TEST(OneModuleInstanceTest, FactsTreeExtractor) {
  std::string code_text =
      "module bar; endmodule: bar module foo; bar b1(); endmodule: foo";
  VerilogAnalyzer analyzer(code_text, "");
  EXPECT_OK(analyzer.Analyze());
  const auto& root = analyzer.Data().SyntaxTree();
  const absl::string_view file_name = "verilog.v";

  const auto expected =
      T({{Anchor(file_name, 0, code_text.size())}, IndexingFactType ::kFile},
        T({{Anchor(absl::string_view("bar"), 7, 10),
            Anchor(absl::string_view("bar"), 23, 26)},
           IndexingFactType::kModule}),
        T({{Anchor(absl::string_view("foo"), 34, 37),
            Anchor(absl::string_view("foo"), 60, 63)},
           IndexingFactType::kModule},
          T({{Anchor(absl::string_view("bar"), 39, 42),
              Anchor(absl::string_view("b1"), 43, 45)},
             IndexingFactType ::kModuleInstance})));

  const auto& facts_tree = BuildIndexingFactsTree(
      ABSL_DIE_IF_NULL(root), analyzer.Data().Contents(), file_name);

  const auto result_pair = DeepEqual(facts_tree, expected, FactsTreeEqual);
  EXPECT_EQ(result_pair.left, nullptr) << *result_pair.left;
  EXPECT_EQ(result_pair.right, nullptr) << *result_pair.right;
}  // namespace

TEST(TwoModuleInstanceTest, FactsTreeExtractor) {
  std::string code_text =
      "module bar; endmodule: bar module foo; bar b1(); bar b2(); endmodule: "
      "foo";
  VerilogAnalyzer analyzer(code_text, "");
  EXPECT_OK(analyzer.Analyze());
  const auto& root = analyzer.Data().SyntaxTree();
  const absl::string_view file_name = "verilog.v";

  const auto expected =
      T({{Anchor(file_name, 0, code_text.size())}, IndexingFactType ::kFile},
        T({{Anchor(absl::string_view("bar"), 7, 10),
            Anchor(absl::string_view("bar"), 23, 26)},
           IndexingFactType::kModule}),
        T({{Anchor(absl::string_view("foo"), 34, 37),
            Anchor(absl::string_view("foo"), 70, 73)},
           IndexingFactType::kModule},
          T({{Anchor(absl::string_view("bar"), 39, 42),
              Anchor(absl::string_view("b1"), 43, 45)},
             IndexingFactType ::kModuleInstance}),
          T({{Anchor(absl::string_view("bar"), 49, 52),
              Anchor(absl::string_view("b2"), 53, 55)},
             IndexingFactType ::kModuleInstance})));

  const auto& facts_tree = BuildIndexingFactsTree(
      ABSL_DIE_IF_NULL(root), analyzer.Data().Contents(), file_name);

  const auto result_pair = DeepEqual(facts_tree, expected, FactsTreeEqual);
  EXPECT_EQ(result_pair.left, nullptr) << *result_pair.left;
  EXPECT_EQ(result_pair.right, nullptr) << *result_pair.right;
}  // namespace

}  // namespace
}  // namespace kythe
}  // namespace verilog
