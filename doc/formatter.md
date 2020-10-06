# Formatter

## Design

This section describes the various subsystems of the Verilog formatter.

### Inter-Token Annotation

Between every pair of adjacent tokens in the to-be-formatted text, there are a
set of attributes that influence spacing and line wrapping decisions,
[InterTokenInfo](https://cs.opensource.google/verible/verible/+/master:common/formatting/format_token.h;l=59?q=InterTokenInfo),
such as minimum spacing between tokens.

The role of the
[token annotator](https://cs.opensource.google/verible/verible/+/master:verilog/formatting/token_annotator.h)
is to populate those attributes based on the attributes, types, and syntactic
context of adjacent pairs of tokens. Formatting rules around punctuation tokens
like parenthesis and brackets often depend on syntactic context.

### Token Partition Tree

[TokenPartitionTree](https://cs.opensource.google/verible/verible/+/master:common/formatting/token_partition_tree.h)
is a language-agnostic hierarchical representation of (formatting) token ranges.
The data represented by each node is an
[UnwrappedLine](https://cs.opensource.google/verible/verible/+/master:common/formatting/unwrapped_line.h),
which contains a range of formatting tokens and indentation information.
TokenPartitionTree has the following properties:

*   The range spanned by a node in the tree is equal to the range spanned by its
    children, if there are any.
*   The end points of the tokens ranges of adjacent siblings are equal, or put
    another way, there are no gaps in token ranges between adjacent sibling
    nodes.

The role of the
[TreeUnwrapper](https://cs.opensource.google/verible/verible/+/master:verilog/formatting/tree_unwrapper.h)
is to convert a SystemVerilog concrete syntax tree into a TokenPartitionTree.

Leaf partitions of the TokenPartitionTree represents a unit of work that is
handled by the line-wrapping optimization phase.

### Wrapping

Line wrapping optimization is currently done as a Dijsktra shortest-path search
over the space of append/wrap decisions spanning a range of formatting tokens.
When a wrap (line break) is used, the optimization state incurs a penalty value
determined in the token annotation phase. This algorithm is not very scalable,
but there are plans to replace it with something like dynamic programming.

### Disable Control and Preservation

## Troubleshooting

This section provides tips about how to triage formatter issues.

`verible-verilog-syntax --printtree` shows the concrete syntax tree
representation of source code. Many formatting decisions are based on syntax
tree node types, which are shown as constants like `kPackageDeclaration`.

`verible-verilog-format --show_token_partition_tree` shows the
TokenPartitionTree subdivision of formatting token ranges and relative
indentation. Add `--show_inter_token_info` to show even more information about
InterTokenInfo before each token.

A lot of other details are traced by running with `-v=4 --alsologtostderr`.
