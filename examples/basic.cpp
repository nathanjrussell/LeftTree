#include <LeftTree/LeftTree.hpp>

#include <iostream>

int main() {
  lefttree::LeftTree t;

  t.createNode();
  std::cout << "current=" << t.currentNodeID() << "\n";

  t.createNode();
  std::cout << "current=" << t.currentNodeID() << "\n";

  t.createLeaf();
  std::cout << "current(after leaf)=" << t.currentNodeID() << "\n";

  t.createLeaf();
  std::cout << "current(after leaf)=" << t.currentNodeID() << "\n";

  t.createLeaf();
  std::cout << "complete current=" << t.currentNodeID() << "\n";

  return 0;
}

