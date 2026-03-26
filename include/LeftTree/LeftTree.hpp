#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stack>
#include <type_traits>
#include <utility>
#include <vector>

namespace lefttree {

/**
 * LeftTree is a small binary-tree builder that grows down the left side.
 *
 * Key behavior:
 * - Elements have sequential IDs starting at 0.
 * - After you create a Node, the next insertion position is its left child.
 * - After you create a Leaf, the builder backtracks to the first ancestor node
 *   with a missing right child, and continues from there.
 * - When the tree is complete, currentNodeID() returns -1.
 *
 * Payload:
 * - Each element stores an arbitrary pointer-like payload (DataPtr). The tree
 *   does not interpret it.
 *
 * Notes:
 * - The builder maintains a "current" insertion node (always a Node), plus a
 *   flag indicating whether the next insertion is its left or right child.
 */
class LeftTree {
public:
  using Id = std::int64_t;

  // Type-erased payload pointer. You can store e.g. shared_ptr<MyData>,
  // unique_ptr<MyData>, raw pointers, or nullptr.
  using DataPtr = std::shared_ptr<void>;

  struct Element {
    Id id;
    DataPtr data;

    explicit Element(Id id_, DataPtr data_ = {}) : id(id_), data(std::move(data_)) {}
    virtual ~Element() = default;

    virtual bool isLeaf() const = 0;
  };

  struct Leaf final : Element {
    using Element::Element;
    bool isLeaf() const override { return true; }
  };

  struct Node final : Element {
    std::unique_ptr<Element> left;
    std::unique_ptr<Element> right;

    using Element::Element;
    bool isLeaf() const override { return false; }
  };

  LeftTree();

  LeftTree(const LeftTree&) = delete;
  LeftTree& operator=(const LeftTree&) = delete;
  LeftTree(LeftTree&&) noexcept = default;
  LeftTree& operator=(LeftTree&&) noexcept = default;

  /** Returns the ID of the current insertion node, or -1 if the tree is complete. */
  [[nodiscard]] Id currentNodeID() const noexcept;

  /** Returns a non-owning pointer to the current insertion node; nullptr if complete. */
  [[nodiscard]] const Node* currentNode() const noexcept;
  [[nodiscard]] Node* currentNode() noexcept;

  /**
   * Returns the current node's left child (may be nullptr if not created yet).
   * Returns nullptr if the tree is complete.
   */
  [[nodiscard]] const Element* left() const noexcept;
  [[nodiscard]] Element* left() noexcept;

  /**
   * Returns the current node's right child (may be nullptr if not created yet).
   * Returns nullptr if the tree is complete.
   */
  [[nodiscard]] const Element* right() const noexcept;
  [[nodiscard]] Element* right() noexcept;

  /** The root element (node or leaf), or nullptr if nothing has been created yet. */
  [[nodiscard]] const Element* root() const noexcept { return root_.get(); }
  [[nodiscard]] Element* root() noexcept { return root_.get(); }

  /** Create a Node at the current insertion position. */
  void createNode(DataPtr data = {});

  /** Create a Leaf at the current insertion position. */
  void createLeaf(DataPtr data = {});

  /** Returns true if the tree is complete (no more insertion positions). */
  [[nodiscard]] bool complete() const noexcept { return current_ == nullptr; }

private:
  struct InsertionPos {
    Node* parent = nullptr;
    bool toLeft = true;
  };

  // Advances current_ after inserting a leaf.
  void backtrackAfterLeaf_();

  // Creates the first element (root).
  void createRootNode_(DataPtr data);
  void createRootLeaf_(DataPtr data);

  // Attach helpers.
  static void attach_(std::unique_ptr<Element>& slot, std::unique_ptr<Element> elem);

  std::unique_ptr<Element> root_;
  Id nextId_ = 0;

  // Path stack of nodes from root to the current insertion node.
  // Invariant: top() == current_ whenever current_ != nullptr.
  std::vector<Node*> path_;

  // Current insertion node. If nullptr => tree is complete.
  Node* current_ = nullptr;

  // If true, next insertion goes to current_->left, else to current_->right.
  bool insertLeft_ = true;
};

} // namespace lefttree

