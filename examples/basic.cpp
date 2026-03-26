#include <LeftTree/LeftTree.hpp>

#include <iostream>
#include <string>

struct NodePayload {
  std::string name;
};

int main() {
  using Tree = lefttree::LeftTree<NodePayload, int>;
  Tree t;

  // Root node (id=0)
  t.createNode();
  if (auto* n = t.getCurrentNode()) {
    n->setData(NodePayload{"root"});
    std::cout << "current=" << t.currentNodeID() << " node.name=" << n->data->name << "\n";
  }

  // Create a left child node (id=1)
  t.createNode(NodePayload{"left-child"});
  if (auto* n = t.getCurrentNode()) {
    std::cout << "current=" << t.currentNodeID() << " node.name=" << n->data->name << "\n";
  }

  // Fill left leaf under node(1); current remains node(1) but will now fill right next.
  t.createLeaf(10);
  if (auto* n = t.getCurrentNode()) {
    std::cout << "current(after leaf)=" << t.currentNodeID() << " (will fill right next)\n";
  }

  // Fill right leaf under node(1); current backtracks to node(0).
  t.createLeaf(20);
  if (auto* n = t.getCurrentNode()) {
    std::cout << "current(after leaf)=" << t.currentNodeID() << " (backtracked)\n";
  }

  // Fill right leaf under node(0); tree completes.
  t.createLeaf(30);
  std::cout << "complete current=" << t.currentNodeID() << " getCurrentNode=" << (t.getCurrentNode() ? "non-null" : "null")
            << "\n";

  return 0;
}
