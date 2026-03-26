#ifndef LEFTTREE_LEFTTREE_HPP
#define LEFTTREE_LEFTTREE_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace lefttree {


template <class NodeData, class LeafData>
class LeftTree {
public:
  using Id = std::int64_t;

  struct Element {
    Id id;

    explicit Element(Id id_) : id(id_) {}
    virtual ~Element() = default;

    virtual bool isLeaf() const = 0;
  };

  struct Leaf final : Element {
    std::optional<LeafData> data;

    explicit Leaf(Id id_) : Element(id_) {}
    Leaf(Id id_, LeafData d) : Element(id_), data(std::move(d)) {}

    bool isLeaf() const override { return true; }

    void setData(LeafData d) { data = std::move(d); }
  };

  struct Node final : Element {
    std::unique_ptr<Element> left;
    std::unique_ptr<Element> right;

    std::optional<NodeData> data;

    explicit Node(Id id_) : Element(id_) {}
    Node(Id id_, NodeData d) : Element(id_), data(std::move(d)) {}

    bool isLeaf() const override { return false; }

    void setData(NodeData d) { data = std::move(d); }
  };

  LeftTree() = default;

  LeftTree(const LeftTree&) = delete;
  LeftTree& operator=(const LeftTree&) = delete;
  LeftTree(LeftTree&&) noexcept = default;
  LeftTree& operator=(LeftTree&&) noexcept = default;

  /** Returns the ID of the current insertion node, or -1 if the tree is complete. */
  [[nodiscard]] Id currentNodeID() const noexcept { return current_ ? current_->id : static_cast<Id>(-1); }

  /** Returns a non-owning pointer to the current insertion node; nullptr if complete. */
  [[nodiscard]] const Node* getCurrentNode() const noexcept { return current_; }
  [[nodiscard]] Node* getCurrentNode() noexcept { return current_; }

  /** Backward-compatible alias. */
  [[nodiscard]] const Node* currentNode() const noexcept { return getCurrentNode(); }
  [[nodiscard]] Node* currentNode() noexcept { return getCurrentNode(); }

  /** Returns the current node's left child (may be nullptr if not created yet). */
  [[nodiscard]] const Element* left() const noexcept {
    if (!current_) return nullptr;
    return current_->left.get();
  }
  [[nodiscard]] Element* left() noexcept {
    if (!current_) return nullptr;
    return current_->left.get();
  }

  /** Returns the current node's right child (may be nullptr if not created yet). */
  [[nodiscard]] const Element* right() const noexcept {
    if (!current_) return nullptr;
    return current_->right.get();
  }
  [[nodiscard]] Element* right() noexcept {
    if (!current_) return nullptr;
    return current_->right.get();
  }

  /** The root element (node or leaf), or nullptr if nothing has been created yet. */
  [[nodiscard]] const Element* root() const noexcept { return root_.get(); }
  [[nodiscard]] Element* root() noexcept { return root_.get(); }

  /** Returns true if the tree is complete (no more insertion positions). */
  [[nodiscard]] bool complete() const noexcept { return current_ == nullptr; }

  /** Returns the total number of created elements (nodes + leaves). */
  [[nodiscard]] std::size_t elementCount() const noexcept {
    // IDs are assigned starting at 0 and incremented for each created element.
    // Therefore nextId_ equals the number of created elements.
    return static_cast<std::size_t>(nextId_ < 0 ? 0 : nextId_);
  }

  /** Create a Node at the current insertion position. */
  void createNode() { createNode(std::nullopt); }
  void createNode(NodeData data) { createNode(std::optional<NodeData>(std::move(data))); }

  /** Create a Leaf at the current insertion position. */
  void createLeaf() { createLeaf(std::nullopt); }
  void createLeaf(LeafData data) { createLeaf(std::optional<LeafData>(std::move(data))); }

  /**
   * Returns a non-owning pointer to the Node with the given ID.
   * If not found (or the ID belongs to a Leaf), returns nullptr.
   */
  [[nodiscard]] const Node* getNode(Id id) const noexcept {
    const Element* e = getElementById_(id);
    if (!e || e->isLeaf()) return nullptr;
    return static_cast<const Node*>(e);
  }
  [[nodiscard]] Node* getNode(Id id) noexcept {
    return const_cast<Node*>(static_cast<const LeftTree*>(this)->getNode(id));
  }

  enum class ElementType : std::uint8_t { Node = 1, Leaf = 2 };

  using NodeDataResult = std::pair<ElementType, std::optional<NodeData>>;
  using LeafDataResult = std::pair<ElementType, std::optional<LeafData>>;
  using ElementData = std::variant<NodeDataResult, LeafDataResult>;

  /** Returns whether the element with this ID is a Node or Leaf; nullopt if not found. */
  [[nodiscard]] std::optional<ElementType> getElementType(Id id) const noexcept {
    const Element* e = getElementById_(id);
    if (!e) return std::nullopt;
    return e->isLeaf() ? ElementType::Leaf : ElementType::Node;
  }

  /**
   * Returns (ElementType, payload) for the element with this ID.
   * - For nodes: variant holds (Node, optional<NodeData>)
   * - For leaves: variant holds (Leaf, optional<LeafData>)
   * Returns nullopt if ID not found.
   */
  [[nodiscard]] std::optional<ElementData> getElementData(Id id) const noexcept {
    const Element* e = getElementById_(id);
    if (!e) return std::nullopt;

    if (e->isLeaf()) {
      const auto* lf = static_cast<const Leaf*>(e);
      return ElementData{LeafDataResult{ElementType::Leaf, lf->data}};
    }

    const auto* nd = static_cast<const Node*>(e);
    return ElementData{NodeDataResult{ElementType::Node, nd->data}};
  }

private:
  static void attach_(std::unique_ptr<Element>& slot, std::unique_ptr<Element> elem) {
    if (slot) throw std::logic_error("LeftTree insertion slot already filled");
    slot = std::move(elem);
  }

  void createRootNode_(std::optional<NodeData> data) {
    std::unique_ptr<Node> n;
    if (data) {
      n = std::make_unique<Node>(nextId_++, std::move(*data));
    } else {
      n = std::make_unique<Node>(nextId_++);
    }
    current_ = n.get();
    root_ = std::move(n);
    path_.clear();
    path_.push_back(current_);
    insertLeft_ = true;

    // Root node means tree is not complete yet.
    idIndexBuilt_ = false;
    idIndex_.clear();
  }

  void createRootLeaf_(std::optional<LeafData> data) {
    if (data) {
      root_ = std::make_unique<Leaf>(nextId_++, std::move(*data));
    } else {
      root_ = std::make_unique<Leaf>(nextId_++);
    }
    current_ = nullptr;
    path_.clear();
    insertLeft_ = true;

    // A single-leaf tree is complete immediately; build the index now.
    idIndexBuilt_ = false;
    buildIdIndexIfComplete_();
  }

  void createNode(std::optional<NodeData> data) {
    if (!root_) {
      createRootNode_(std::move(data));
      return;
    }
    if (!current_) throw std::logic_error("LeftTree is complete; cannot create more elements");

    std::unique_ptr<Node> child;
    if (data) {
      child = std::make_unique<Node>(nextId_++, std::move(*data));
    } else {
      child = std::make_unique<Node>(nextId_++);
    }
    Node* childPtr = child.get();

    if (insertLeft_) {
      attach_(current_->left, std::move(child));
    } else {
      attach_(current_->right, std::move(child));
    }

    current_ = childPtr;
    path_.push_back(current_);
    insertLeft_ = true;

    // Creating a node never completes the tree.
    idIndexBuilt_ = false;
    idIndex_.clear();
  }

  void createLeaf(std::optional<LeafData> data) {
    if (!root_) {
      createRootLeaf_(std::move(data));
      return;
    }
    if (!current_) throw std::logic_error("LeftTree is complete; cannot create more elements");

    std::unique_ptr<Element> child;
    if (data) {
      child = std::make_unique<Leaf>(nextId_++, std::move(*data));
    } else {
      child = std::make_unique<Leaf>(nextId_++);
    }

    if (insertLeft_) {
      attach_(current_->left, std::move(child));
    } else {
      attach_(current_->right, std::move(child));
    }

    backtrackAfterLeaf_();

    // If this leaf finished the tree, build the index once.
    buildIdIndexIfComplete_();
  }

  void backtrackAfterLeaf_() {
    // If we just filled the left child, next fill should be right child of current node.
    if (insertLeft_) {
      insertLeft_ = false;
      return;
    }

    // We just filled the right child of current_. Pop it and continue upward until we find
    // a parent missing a right child.
    while (!path_.empty()) {
      path_.pop_back();

      if (path_.empty()) {
        current_ = nullptr;
        insertLeft_ = true;
        return;
      }

      Node* parent = path_.back();
      if (!parent->right) {
        current_ = parent;
        insertLeft_ = false;
        return;
      }
    }

    current_ = nullptr;
    insertLeft_ = true;
  }

  [[nodiscard]] const Element* getElementById_(Id id) const noexcept {
    // Use the fast index once the tree is complete and the index has been built.
    if (idIndexBuilt_) {
      if (id < 0) return nullptr;
      const auto idx = static_cast<std::size_t>(id);
      if (idx >= idIndex_.size()) return nullptr;
      return idIndex_[idx];
    }

    // Fallback during construction.
    return findElementById_(root_.get(), id);
  }

  static const Element* findElementById_(const Element* e, Id id) noexcept {
    if (!e) return nullptr;
    if (e->id == id) return e;

    if (e->isLeaf()) return nullptr;

    const auto* n = static_cast<const Node*>(e);
    if (const Element* hit = findElementById_(n->left.get(), id)) return hit;
    return findElementById_(n->right.get(), id);
  }

  void buildIdIndexIfComplete_() {
    if (!complete() || idIndexBuilt_) return;

    if (nextId_ < 0) {
      throw std::logic_error("LeftTree internal error: negative nextId_");
    }

    idIndex_.assign(static_cast<std::size_t>(nextId_), nullptr);
    fillIdIndex_(root_.get());

    // Validate that every ID [0..nextId_) exists exactly once.
    for (std::size_t i = 0; i < idIndex_.size(); ++i) {
      if (idIndex_[i] == nullptr) {
        throw std::logic_error("LeftTree internal error: missing element ID in index");
      }
    }

    idIndexBuilt_ = true;
  }

  void fillIdIndex_(const Element* e) {
    if (!e) return;

    if (e->id < 0) throw std::logic_error("LeftTree internal error: negative element ID");
    const auto idx = static_cast<std::size_t>(e->id);
    if (idx >= idIndex_.size()) throw std::logic_error("LeftTree internal error: element ID out of range");
    if (idIndex_[idx] != nullptr) throw std::logic_error("LeftTree internal error: duplicate element ID");

    idIndex_[idx] = e;

    if (!e->isLeaf()) {
      const auto* n = static_cast<const Node*>(e);
      fillIdIndex_(n->left.get());
      fillIdIndex_(n->right.get());
    }
  }

  std::unique_ptr<Element> root_;
  Id nextId_ = 0;
  std::vector<Node*> path_;
  Node* current_ = nullptr;
  bool insertLeft_ = true;

  // Built only when the tree becomes complete (current_ == nullptr).
  bool idIndexBuilt_ = false;
  std::vector<const Element*> idIndex_;
};

} // namespace lefttree


#endif // LEFTTREE_LEFTTREE_HPP
