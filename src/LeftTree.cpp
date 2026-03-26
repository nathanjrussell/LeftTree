#include <LeftTree/LeftTree.hpp>

#include <stdexcept>

namespace lefttree {

LeftTree::LeftTree() = default;

LeftTree::Id LeftTree::currentNodeID() const noexcept {
  return current_ ? current_->id : static_cast<Id>(-1);
}

const LeftTree::Node* LeftTree::currentNode() const noexcept { return current_; }
LeftTree::Node* LeftTree::currentNode() noexcept { return current_; }

const LeftTree::Element* LeftTree::left() const noexcept {
  if (!current_) return nullptr;
  return current_->left.get();
}

LeftTree::Element* LeftTree::left() noexcept {
  if (!current_) return nullptr;
  return current_->left.get();
}

const LeftTree::Element* LeftTree::right() const noexcept {
  if (!current_) return nullptr;
  return current_->right.get();
}

LeftTree::Element* LeftTree::right() noexcept {
  if (!current_) return nullptr;
  return current_->right.get();
}

void LeftTree::attach_(std::unique_ptr<Element>& slot, std::unique_ptr<Element> elem) {
  if (slot) {
    throw std::logic_error("LeftTree insertion slot already filled");
  }
  slot = std::move(elem);
}

void LeftTree::createRootNode_(DataPtr data) {
  auto n = std::make_unique<Node>(nextId_++, std::move(data));
  current_ = n.get();
  root_ = std::move(n);
  path_.clear();
  path_.push_back(current_);
  insertLeft_ = true;
}

void LeftTree::createRootLeaf_(DataPtr data) {
  root_ = std::make_unique<Leaf>(nextId_++, std::move(data));
  // A single leaf tree is complete immediately: no insertion node exists.
  current_ = nullptr;
  path_.clear();
  insertLeft_ = true;
}

void LeftTree::createNode(DataPtr data) {
  if (!root_) {
    createRootNode_(std::move(data));
    return;
  }

  if (!current_) {
    throw std::logic_error("LeftTree is complete; cannot create more elements");
  }

  auto child = std::make_unique<Node>(nextId_++, std::move(data));
  Node* childPtr = child.get();

  if (insertLeft_) {
    attach_(current_->left, std::move(child));
  } else {
    attach_(current_->right, std::move(child));
  }

  // After creating a Node, we grow into it, pushing to the path stack.
  current_ = childPtr;
  path_.push_back(current_);
  insertLeft_ = true;
}

void LeftTree::createLeaf(DataPtr data) {
  if (!root_) {
    createRootLeaf_(std::move(data));
    return;
  }

  if (!current_) {
    throw std::logic_error("LeftTree is complete; cannot create more elements");
  }

  std::unique_ptr<Element> child = std::make_unique<Leaf>(nextId_++, std::move(data));

  if (insertLeft_) {
    attach_(current_->left, std::move(child));
  } else {
    attach_(current_->right, std::move(child));
  }

  backtrackAfterLeaf_();
}

void LeftTree::backtrackAfterLeaf_() {
  // If we just filled the left child, we should now fill the right child of the current node.
  if (insertLeft_) {
    insertLeft_ = false;
    return;
  }

  // We just filled the right child of current_. That node is now complete.
  // Pop it, and keep popping while parent right is already filled.
  while (!path_.empty()) {
    // Current node is complete, so pop it.
    path_.pop_back();

    if (path_.empty()) {
      // We completed the root.
      current_ = nullptr;
      insertLeft_ = true;
      return;
    }

    Node* parent = path_.back();

    // If parent's right isn't filled, we should fill it next.
    if (!parent->right) {
      current_ = parent;
      insertLeft_ = false;
      return;
    }

    // Otherwise, parent is also complete (it must have left + right), continue upwards.
  }

  // Defensive: if path_ got emptied without returning, tree is complete.
  current_ = nullptr;
  insertLeft_ = true;
}

} // namespace lefttree

