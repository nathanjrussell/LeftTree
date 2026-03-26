#include <gtest/gtest.h>

#include <LeftTree/LeftTree.hpp>

using lefttree::LeftTree;

TEST(LeftTree, EmptyTreeCurrentIsMinusOne) {
  LeftTree t;
  EXPECT_EQ(t.currentNodeID(), -1);
  EXPECT_EQ(t.root(), nullptr);
  EXPECT_TRUE(t.complete());
}

TEST(LeftTree, RootNodeHasIdZero) {
  LeftTree t;
  t.createNode();
  ASSERT_NE(t.root(), nullptr);
  EXPECT_EQ(t.currentNodeID(), 0);
  EXPECT_FALSE(t.complete());
}

TEST(LeftTree, RootLeafCompletesTreeImmediately) {
  LeftTree t;
  t.createLeaf();
  ASSERT_NE(t.root(), nullptr);
  EXPECT_EQ(t.currentNodeID(), -1);
  EXPECT_TRUE(t.complete());
}

TEST(LeftTree, BacktracksToFirstNodeWithoutRightAndThenCompletes) {
  LeftTree t;

  // Build a minimal full tree of depth 1:
  //   node(0)
  //    /   \
  // leaf(1) leaf(2)
  t.createNode();
  EXPECT_EQ(t.currentNodeID(), 0);

  t.createLeaf();
  // After left leaf, current should still be node 0, now filling right.
  EXPECT_EQ(t.currentNodeID(), 0);
  ASSERT_NE(t.currentNode(), nullptr);
  EXPECT_NE(t.left(), nullptr);
  EXPECT_EQ(t.right(), nullptr);

  t.createLeaf();
  // After right leaf, tree is complete.
  EXPECT_EQ(t.currentNodeID(), -1);
  EXPECT_TRUE(t.complete());
}

TEST(LeftTree, GrowsLeftThenFillsMissingRights) {
  LeftTree t;
  // Root
  t.createNode();              // id 0, current=0
  t.createNode();              // id 1, current=1 (left of 0)
  t.createLeaf();              // id 2, fills left of 1; current should be 1 (fill right next)
  EXPECT_EQ(t.currentNodeID(), 1);

  t.createLeaf();              // id 3, fills right of 1; backtracks to node 0 right
  EXPECT_EQ(t.currentNodeID(), 0);
  ASSERT_NE(t.currentNode(), nullptr);
  EXPECT_EQ(t.left()->id, 1);
  EXPECT_EQ(t.right(), nullptr);

  t.createLeaf();              // id 4, fills right of 0 (since left already exists)
  EXPECT_EQ(t.currentNodeID(), -1);
  EXPECT_TRUE(t.complete());
}

