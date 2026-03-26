#ifndef LEFTTREE_LEFTTREE_SERIALIZATION_HPP
#define LEFTTREE_LEFTTREE_SERIALIZATION_HPP


#include <LeftTree/LeftTree.hpp>

#include <cpp_type_concepts/Serializable.hpp>

#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace lefttree {

namespace detail {

inline void writeU8(std::ostream& out, std::uint8_t v) {
  out.put(static_cast<char>(v));
  if (!out) throw std::runtime_error("writeU8 failed");
}

inline std::uint8_t readU8(std::istream& in) {
  const int c = in.get();
  if (c == std::char_traits<char>::eof()) throw std::runtime_error("readU8 failed");
  return static_cast<std::uint8_t>(c);
}

inline void writeU64BE(std::ostream& out, std::uint64_t v) {
  for (int i = 7; i >= 0; --i) {
    writeU8(out, static_cast<std::uint8_t>((v >> (i * 8)) & 0xFFu));
  }
}

inline std::uint64_t readU64BE(std::istream& in) {
  std::uint64_t v = 0;
  for (int i = 0; i < 8; ++i) {
    v = (v << 8) | readU8(in);
  }
  return v;
}

inline std::string readBytes(std::istream& in, std::uint64_t n) {
  if (n > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
    throw std::runtime_error("payload too large");
  }
  std::string s;
  s.resize(static_cast<std::size_t>(n));
  in.read(s.data(), static_cast<std::streamsize>(n));
  if (!in) throw std::runtime_error("readBytes failed");
  return s;
}

inline constexpr std::uint8_t kMarkerNode = static_cast<std::uint8_t>('N');
inline constexpr std::uint8_t kMarkerLeaf = static_cast<std::uint8_t>('L');
inline constexpr std::uint8_t kMarkerNull = static_cast<std::uint8_t>('0');

} // namespace detail

/**
 * Serialization helper for LeftTree.
 *
 * Wire format (preorder):
 *   - Null: '0'
 *   - Leaf: 'L' + u64_be(len) + payload_bytes
 *   - Node: 'N' + u64_be(len) + payload_bytes + left_subtree + right_subtree
 *
 * Outer framing:
 *   - u64_be(total_bytes) + (tree bytes)
 *
 * Notes:
 * - Payload bytes are produced by NodeData::serialize(std::ostream&) / LeafData::serialize(...)
 * - Payload reconstruction is done via T::deserialize(std::istream&)
 */
template <class NodeData, class LeafData>
  requires(cpp_type_concepts::Serializable<NodeData> && cpp_type_concepts::Serializable<LeafData>)
struct LeftTreeSerialization {
  using Tree = LeftTree<NodeData, LeafData>;
  using Element = typename Tree::Element;
  using Node = typename Tree::Node;
  using Leaf = typename Tree::Leaf;

  static void serializeToStream(const Tree& t, std::ostream& out) {
    // Build payload into memory to compute leading u64 length.
    std::ostringstream payload(std::ios::binary);
    writeElement_(t.root(), payload);

    const std::string bytes = std::move(payload).str();
    detail::writeU64BE(out, static_cast<std::uint64_t>(bytes.size()));
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!out) throw std::runtime_error("serializeToStream failed");
  }

  static Tree deserializeFromStream(std::istream& in) {
    const std::uint64_t totalLen = detail::readU64BE(in);
    const std::string bytes = detail::readBytes(in, totalLen);

    std::istringstream payload(bytes, std::ios::binary);

    // Parse the root element.
    const std::uint8_t marker = detail::readU8(payload);
    if (marker == detail::kMarkerLeaf) {
      const std::uint64_t len = detail::readU64BE(payload);
      Tree t;
      if (len == 0) {
        t.createLeaf();
      } else {
        const std::string leafBytes = detail::readBytes(payload, len);
        std::istringstream leafPayload(leafBytes, std::ios::binary);
        t.createLeaf(LeafData::deserialize(leafPayload));
      }

      if (payload.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("deserializeFromStream: trailing bytes");
      }
      if (!t.complete()) {
        throw std::runtime_error("deserializeFromStream: reconstructed tree incomplete");
      }
      return t;
    }

    if (marker != detail::kMarkerNode) {
      throw std::runtime_error("deserializeFromStream: bad root marker");
    }

    // Root is a node => we can use the standard recursive builder.
    std::uint64_t len = detail::readU64BE(payload);
    Tree t;
    if (len == 0) {
      t.createNode();
    } else {
      const std::string nodeBytes = detail::readBytes(payload, len);
      std::istringstream nodePayload(nodeBytes, std::ios::binary);
      t.createNode(NodeData::deserialize(nodePayload));
    }

    buildFromStream_(t, payload);
    buildFromStream_(t, payload);

    if (payload.peek() != std::char_traits<char>::eof()) {
      throw std::runtime_error("deserializeFromStream: trailing bytes");
    }
    if (!t.complete()) {
      throw std::runtime_error("deserializeFromStream: reconstructed tree incomplete");
    }
    return t;
  }

private:
  template <class T>
  static void writeFramedPayload_(std::ostream& out, std::uint8_t marker, const std::optional<T>& opt) {
    detail::writeU8(out, marker);
    if (!opt) {
      detail::writeU64BE(out, 0);
      return;
    }

    std::ostringstream tmp(std::ios::binary);
    opt->serialize(tmp);
    const std::string bytes = std::move(tmp).str();

    detail::writeU64BE(out, static_cast<std::uint64_t>(bytes.size()));
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!out) throw std::runtime_error("writeFramedPayload failed");
  }

  static void writeElement_(const Element* e, std::ostream& out) {
    if (!e) {
      detail::writeU8(out, detail::kMarkerNull);
      return;
    }

    if (e->isLeaf()) {
      const auto* lf = static_cast<const Leaf*>(e);
      writeFramedPayload_<LeafData>(out, detail::kMarkerLeaf, lf->data);
      return;
    }

    const auto* nd = static_cast<const Node*>(e);
    writeFramedPayload_<NodeData>(out, detail::kMarkerNode, nd->data);
    writeElement_(nd->left.get(), out);
    writeElement_(nd->right.get(), out);
  }

  static void buildFromStream_(Tree& t, std::istream& in) {
    const std::uint8_t marker = detail::readU8(in);

    if (marker == detail::kMarkerNull) {
      // For now we disallow constructing null children via the builder API.
      // If this appears, the format doesn't match the builder assumptions.
      throw std::runtime_error("deserialize: null element not supported");
    }

    if (marker == detail::kMarkerLeaf) {
      // We already consumed marker; read len+bytes and delegate.
      const std::uint64_t len = detail::readU64BE(in);
      if (len == 0) {
        t.createLeaf();
      } else {
        const std::string bytes = detail::readBytes(in, len);
        std::istringstream payload(bytes, std::ios::binary);
        t.createLeaf(LeafData::deserialize(payload));
      }
      return;
    }

    if (marker == detail::kMarkerNode) {
      const std::uint64_t len = detail::readU64BE(in);
      if (len == 0) {
        t.createNode();
      } else {
        const std::string bytes = detail::readBytes(in, len);
        std::istringstream payload(bytes, std::ios::binary);
        t.createNode(NodeData::deserialize(payload));
      }

      buildFromStream_(t, in);
      buildFromStream_(t, in);
      return;
    }

    throw std::runtime_error("deserialize: bad marker");
  }
};

} // namespace lefttree

#endif // LEFTTREE_LEFTTREE_SERIALIZATION_HPP