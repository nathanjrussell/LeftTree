#include <gtest/gtest.h>

#include <LeftTree/LeftTree.hpp>
#include <LeftTree/LeftTreeSerialization.hpp>

#include <sstream>
#include <string>

namespace {

struct SerializableInt {
  int value = 0;

  void serialize(std::ostream& out) const {
    // Simple, stable binary: big-endian int32
    std::uint32_t u = static_cast<std::uint32_t>(static_cast<std::int32_t>(value));
    for (int i = 3; i >= 0; --i) {
      out.put(static_cast<char>((u >> (i * 8)) & 0xFFu));
    }
  }

  static SerializableInt deserialize(std::istream& in) {
    std::uint32_t u = 0;
    for (int i = 0; i < 4; ++i) {
      int c = in.get();
      if (c == std::char_traits<char>::eof()) throw std::runtime_error("EOF");
      u = (u << 8) | static_cast<std::uint32_t>(static_cast<unsigned char>(c));
    }
    SerializableInt v;
    v.value = static_cast<int>(static_cast<std::int32_t>(u));
    return v;
  }

  bool operator==(const SerializableInt& other) const noexcept { return value == other.value; }
};

struct NodePayload {
  std::string name;

  void serialize(std::ostream& out) const {
    // u32_be length + bytes
    const std::uint32_t len = static_cast<std::uint32_t>(name.size());
    for (int i = 3; i >= 0; --i) out.put(static_cast<char>((len >> (i * 8)) & 0xFFu));
    out.write(name.data(), static_cast<std::streamsize>(name.size()));
  }

  static NodePayload deserialize(std::istream& in) {
    std::uint32_t len = 0;
    for (int i = 0; i < 4; ++i) {
      int c = in.get();
      if (c == std::char_traits<char>::eof()) throw std::runtime_error("EOF");
      len = (len << 8) | static_cast<std::uint32_t>(static_cast<unsigned char>(c));
    }

    NodePayload p;
    p.name.resize(len);
    in.read(p.name.data(), static_cast<std::streamsize>(len));
    if (!in) throw std::runtime_error("EOF");
    return p;
  }
};

} // namespace

using Tree = lefttree::LeftTree<NodePayload, SerializableInt>;
using Ser = lefttree::LeftTreeSerialization<NodePayload, SerializableInt>;

TEST(LeftTree, EmptyTreeCurrentIsMinusOne) {
  Tree t;
  EXPECT_EQ(t.currentNodeID(), -1);
  EXPECT_EQ(t.root(), nullptr);
  EXPECT_TRUE(t.complete());
  EXPECT_EQ(t.getCurrentNode(), nullptr);
}

TEST(LeftTree, RootNodeHasIdZeroAndSetDataWorks) {
  Tree t;
  t.createNode();

  ASSERT_NE(t.root(), nullptr);
  EXPECT_EQ(t.currentNodeID(), 0);
  EXPECT_FALSE(t.complete());

  auto* n = t.getCurrentNode();
  ASSERT_NE(n, nullptr);
  n->setData(NodePayload{"root"});
  ASSERT_TRUE(n->data.has_value());
  EXPECT_EQ(n->data->name, "root");
}

TEST(LeftTree, RootLeafCompletesTreeImmediately) {
  Tree t;
  t.createLeaf(SerializableInt{123});
  ASSERT_NE(t.root(), nullptr);
  EXPECT_EQ(t.currentNodeID(), -1);
  EXPECT_TRUE(t.complete());
  EXPECT_EQ(t.getCurrentNode(), nullptr);
}

TEST(LeftTree, BacktracksToFirstNodeWithoutRightAndThenCompletes) {
  Tree t;

  // Build a minimal full tree of depth 1:
  //   node(0)
  //    /   \
  // leaf(1) leaf(2)
  t.createNode(NodePayload{"n0"});
  EXPECT_EQ(t.currentNodeID(), 0);
  ASSERT_NE(t.getCurrentNode(), nullptr);

  t.createLeaf(SerializableInt{1});
  // After left leaf, current should still be node 0, now filling right.
  EXPECT_EQ(t.currentNodeID(), 0);
  ASSERT_NE(t.getCurrentNode(), nullptr);
  EXPECT_NE(t.left(), nullptr);
  EXPECT_EQ(t.right(), nullptr);

  t.createLeaf(SerializableInt{2});
  // After right leaf, tree is complete.
  EXPECT_EQ(t.currentNodeID(), -1);
  EXPECT_TRUE(t.complete());
  EXPECT_EQ(t.getCurrentNode(), nullptr);
}

TEST(LeftTree, GrowsLeftThenFillsMissingRights) {
  Tree t;
  // Root
  t.createNode(NodePayload{"n0"});
  ASSERT_NE(t.getCurrentNode(), nullptr);

  t.createNode(NodePayload{"n1"});
  ASSERT_NE(t.getCurrentNode(), nullptr);
  EXPECT_EQ(t.currentNodeID(), 1);

  t.createLeaf(SerializableInt{10});
  EXPECT_EQ(t.currentNodeID(), 1);

  t.createLeaf(SerializableInt{20});
  EXPECT_EQ(t.currentNodeID(), 0);
  ASSERT_NE(t.getCurrentNode(), nullptr);
  ASSERT_TRUE(t.getCurrentNode()->data.has_value());
  EXPECT_EQ(t.getCurrentNode()->data->name, "n0");

  t.createLeaf(SerializableInt{30});
  EXPECT_EQ(t.currentNodeID(), -1);
  EXPECT_TRUE(t.complete());
  EXPECT_EQ(t.getCurrentNode(), nullptr);
}

TEST(LeftTree, LeafSetDataWorks) {
  Tree t;
  t.createNode(NodePayload{"n0"});
  t.createLeaf();

  auto* leaf = t.left();
  ASSERT_NE(leaf, nullptr);
  ASSERT_TRUE(leaf->isLeaf());

  auto* leafTyped = dynamic_cast<Tree::Leaf*>(leaf);
  ASSERT_NE(leafTyped, nullptr);

  leafTyped->setData(SerializableInt{42});
  ASSERT_TRUE(leafTyped->data.has_value());
  EXPECT_EQ(leafTyped->data->value, 42);
}

TEST(LeftTree, SingleLeafTreeIsSupported) {
  Tree t;
  t.createLeaf(SerializableInt{99});

  ASSERT_NE(t.root(), nullptr);
  EXPECT_TRUE(t.root()->isLeaf());

  // A single-leaf tree is complete immediately.
  EXPECT_TRUE(t.complete());
  EXPECT_EQ(t.currentNodeID(), -1);
  EXPECT_EQ(t.getCurrentNode(), nullptr);

  auto* leaf = dynamic_cast<Tree::Leaf*>(t.root());
  ASSERT_NE(leaf, nullptr);
  ASSERT_TRUE(leaf->data.has_value());
  EXPECT_EQ(leaf->data->value, 99);
}

static void expectHelpersMatchById(const Tree& a, const Tree& b, Tree::Id id) {
  const auto ta = a.getElementType(id);
  const auto tb = b.getElementType(id);
  ASSERT_TRUE(ta.has_value());
  ASSERT_TRUE(tb.has_value());
  EXPECT_EQ(*ta, *tb);

  const auto da = a.getElementData(id);
  const auto db = b.getElementData(id);
  ASSERT_TRUE(da.has_value());
  ASSERT_TRUE(db.has_value());

  // Same variant alternative (node vs leaf), then compare stored optionals.
  EXPECT_EQ(da->index(), db->index());

  if (std::holds_alternative<Tree::NodeDataResult>(*da)) {
    const auto& pa = std::get<Tree::NodeDataResult>(*da);
    const auto& pb = std::get<Tree::NodeDataResult>(*db);
    EXPECT_EQ(pa.first, Tree::ElementType::Node);
    EXPECT_EQ(pb.first, Tree::ElementType::Node);
    EXPECT_EQ(pa.second.has_value(), pb.second.has_value());
    if (pa.second && pb.second) {
      EXPECT_EQ(pa.second->name, pb.second->name);
    }
  } else {
    const auto& pa = std::get<Tree::LeafDataResult>(*da);
    const auto& pb = std::get<Tree::LeafDataResult>(*db);
    EXPECT_EQ(pa.first, Tree::ElementType::Leaf);
    EXPECT_EQ(pb.first, Tree::ElementType::Leaf);
    EXPECT_EQ(pa.second.has_value(), pb.second.has_value());
    if (pa.second && pb.second) {
      EXPECT_EQ(pa.second->value, pb.second->value);
    }
  }
}

TEST(LeftTree, SerializeDeserializeRoundTripPreservesStructureAndPayloads) {
  Tree t;
  // Build a small full tree:
  //        node0("root")
  //        /          \
  //   node1("left")    leaf(30)
  //    /     \
  // leaf(10) leaf(20)
  t.createNode(NodePayload{"root"});      // id 0
  t.createNode(NodePayload{"left"});      // id 1
  t.createLeaf(SerializableInt{10});       // id 2
  t.createLeaf(SerializableInt{20});       // id 3
  t.createLeaf(SerializableInt{30});       // id 4
  ASSERT_TRUE(t.complete());

  std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
  Ser::serializeToStream(t, ss);

  ss.seekg(0);
  Tree t2 = Ser::deserializeFromStream(ss);
  ASSERT_TRUE(t2.complete());

  // Helper-method equivalence checks (pre vs post) for every element by ID.
  // This assumes the serialization/deserialization reproduces the same creation order,
  // thus preserving IDs.
  const auto* rootPre = dynamic_cast<const Tree::Node*>(t.root());
  ASSERT_NE(rootPre, nullptr);
  const auto maxId = static_cast<Tree::Id>(4);
  for (Tree::Id id = 0; id <= maxId; ++id) {
    expectHelpersMatchById(t, t2, id);
  }

  // Root must be a node with id 0 and payload
  auto* root = dynamic_cast<const Tree::Node*>(t2.root());
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(root->id, 0);
  ASSERT_TRUE(root->data.has_value());
  EXPECT_EQ(root->data->name, "root");

  // getNode(id) should find nodes only
  auto* n0 = t2.getNode(0);
  ASSERT_NE(n0, nullptr);
  EXPECT_EQ(n0->id, 0);

  auto* n1 = t2.getNode(1);
  ASSERT_NE(n1, nullptr);
  EXPECT_EQ(n1->id, 1);
  ASSERT_TRUE(n1->data.has_value());
  EXPECT_EQ(n1->data->name, "left");

  EXPECT_EQ(t2.getNode(2), nullptr); // leaf id
  EXPECT_EQ(t2.getNode(3), nullptr); // leaf id
  EXPECT_EQ(t2.getNode(4), nullptr); // leaf id

  // Structure checks
  ASSERT_NE(root->left.get(), nullptr);
  ASSERT_NE(root->right.get(), nullptr);

  auto* leftNode = dynamic_cast<const Tree::Node*>(root->left.get());
  ASSERT_NE(leftNode, nullptr);
  EXPECT_EQ(leftNode->id, 1);

  auto* rightLeaf = dynamic_cast<const Tree::Leaf*>(root->right.get());
  ASSERT_NE(rightLeaf, nullptr);
  EXPECT_EQ(rightLeaf->id, 4);
  ASSERT_TRUE(rightLeaf->data.has_value());
  EXPECT_EQ(rightLeaf->data->value, 30);

  auto* ll = dynamic_cast<const Tree::Leaf*>(leftNode->left.get());
  auto* lr = dynamic_cast<const Tree::Leaf*>(leftNode->right.get());
  ASSERT_NE(ll, nullptr);
  ASSERT_NE(lr, nullptr);
  EXPECT_EQ(ll->id, 2);
  EXPECT_EQ(lr->id, 3);
  ASSERT_TRUE(ll->data.has_value());
  ASSERT_TRUE(lr->data.has_value());
  EXPECT_EQ(ll->data->value, 10);
  EXPECT_EQ(lr->data->value, 20);
}
