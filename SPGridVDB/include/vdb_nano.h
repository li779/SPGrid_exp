/*!
    Copyright Ken Museth (please do not distribute this file)

    \file VDB.h

    \author Ken Museth

    \date January 8, 2017

    \brief Implements a light-weight self-contained VDB data-structure in a
           single file! In other words this is a significantly watered down
           version of the OpenVDB implementation, with no dependencies other
           than a few STL header files (see below) - so a one-stop-shop for
           a minimalistic VDB data structure!

    \note  It is possible (even likely) that for simple applications of VDB
           this implementaiton might outperform the one in OpenVDB, primarily
           due to the lack of support for "delayed-loading" and better utilization
           of the instruction cache. However, only bechmark tests can tell.

    \warning It is primarily meant as an educational illustration of some of the most fundamental
             design principals behind the VDB data strcuture. Obviously the full implementation
             in OpenVDB has far more features, optimizations, and most importantly tools.

    \details Please see the following paper for more details:
             K. Museth, “VDB: High-Resolution Sparse Volumes with Dynamic Topology”,
             ACM Transactions on Graphics 32(3), 2013, which can be found here:
             http://www.museth.org/Ken/Publications_files/Museth_TOG13.pdf

    Overview: This file implements the following fundamental class that when combined
              forms the backbone of the VDB tree data structure:

              Coord - a simple container of three signed integer coordinates
              Mask - a bit mask essential to the non-root tree nodes
              Tree - defined the high-level API of the VDB data structure
              RootNode - the top level node of the VDB data structure
              InternalNode - the internal nodes of the VDB data structure
              LeafNode - the lowest level tree nodes that encode voxel values and state
              ValueAccessor - implements accelerated random access operations

    Semantics: A VDB data structure encodes values and (binary) states associated with
               signed integer coordinates. Values encoded at the leaf node level are
               denoted voxel values, and values associated with other tree nodes are referred
               to as tile values, which by design covers a larger coordinate index domain.
*/

#ifndef VDB_H_HAS_BEEN_INCLUDED
#define VDB_H_HAS_BEEN_INCLUDED

// optionally replace a sorted map in the root node with a hash table.
// OpenVDB employes a sorted map but the VDB paper also discusses the
// potential benefits of using a hash table so we include both.
#define USE_HASH_MAP

#include <array> // for std::array
#include <cstdint>// for uint32_t and uint64_t
#include <limits>// for std::numeric_limits
#ifdef USE_HASH_MAP
#include <unordered_map>// for std::unordered_map
#else
#include <map>//for the sorted std::map
#endif

namespace vdb {

// ------------------------------> Coord <--------------------------------------

/// @brief Signed (x, y, z) 32-bit integer coordinate class
class Coord
{
public:
    /// @brief Initialize all coordinates to zero.
    Coord() : mVec({0, 0, 0}) {}

    /// @brief Initializes all coordinates to the given signed integer.
    explicit Coord(int ijk) : mVec({ijk, ijk, ijk}) {}

    /// @brief Initializes coordinate to the given signed integers.
    Coord(int x, int y, int z) : mVec({x, y, z}) {}

    /// @brief Default copy constructor
    Coord(const Coord&) = default;

    // @brief Default assignment operator.
    Coord& operator=(const Coord&) = default;

    /// @brief Return a const reference to the given Coord component.
    /// @warning The argument is assumed to be 0, 1, or 2.
    const int& operator[](uint32_t i) const { return mVec[i]; }

    /// @brief Return a non-const reference to the given Coord component.
    /// @warning The argument is assumed to be 0, 1, or 2.
    int& operator[](uint32_t i) { return mVec[i]; }

    /// @brief Return a new instance with coordinates masked by the given unsigned integer.
    Coord operator& (uint32_t n) const { return Coord(mVec[0] & n, mVec[1] & n, mVec[2] & n); }

    /// @brief Return a new instance with coordinates masked by the given signed integer.
    Coord operator& (int n) const { return Coord(mVec[0] & n, mVec[1] & n, mVec[2] & n); }

    /// @brief Return true is this Coord is Lexicographiclly less than the given Coord.
    bool operator<(const Coord& rhs) const
    {
        return mVec[0] < rhs[0] ? true : mVec[0] > rhs[0] ? false
             : mVec[1] < rhs[1] ? true : mVec[1] > rhs[1] ? false
             : mVec[2] < rhs[2] ? true : false;
    }

    /// @brief Make this Coord the minimum with respect to another Coord.
    void minComponent(const Coord& other)
    {
        mVec[0] = std::min(mVec[0], other.mVec[0]);
        mVec[1] = std::min(mVec[1], other.mVec[1]);
        mVec[2] = std::min(mVec[2], other.mVec[2]);
    }

    /// @brief Make this Coord the minimum with respect to another Coord.
    void maxComponent(const Coord& other)
    {
        mVec[0] = std::max(mVec[0], other.mVec[0]);
        mVec[1] = std::max(mVec[1], other.mVec[1]);
        mVec[2] = std::max(mVec[2], other.mVec[2]);
    }

    /// @brief Offset the coordinates by the given integers.
    Coord& offset(int dx, int dy, int dz)
    {
      mVec[0] += dx;
      mVec[1] += dy;
      mVec[2] += dz;
      return *this;
    }

    /// @brief Offset the coordinates by the given integer.
    Coord& offset(int n) { mVec[0] += n; mVec[1] += n; mVec[2] += n; return *this; }

    /// @brief Return true if the Coord components are identical.
    bool operator==(const Coord& rhs) const
    {
      return mVec[0]==rhs[0] && mVec[1]==rhs[1] && mVec[2]==rhs[2];
    }
    /// @brief Return a hash key derived from the existing coordinates.
    /// @details For details on this hash function please see the VDB paper.
    inline size_t hash() const
    {
      return ( (1<<(3+4+5))-1 ) & (mVec[0]*73856093 ^ mVec[1]*19349663 ^ mVec[2]*83492791);
    }

private:

    // Only private data member of Coord
    std::array<int, 3> mVec;
};// Coord class


// ----------------------------> Mask <--------------------------------------


/// @brief Bit-mask to encode active states and facilitate sequnetial iterators
/// and a fast codec for I/O compression.
template <uint32_t LOG2DIM>
class Mask
{
public:
    static const uint32_t SIZE = 1<<3*LOG2DIM, COUNT = SIZE >> 6;// number of 64 bit words

    /// @brief Iterator that sequentially visits the set bits of this Mask.
    /// @details A public member class of the Mask class.
    /// @warning This is not a STL-compliant iterator!
    class Iterator
    {
    public:
      /// @brief Default constructor pointing to the end
      Iterator() : mPos(Mask::SIZE), mParent(nullptr) {}

      /// @brief Initialize Iterator to point to the given set bit.
      Iterator(uint32_t pos, const Mask* parent) : mPos(pos), mParent(parent) {}

      /// @brief Return true if the iterators are differencent.
      bool operator!=(const Iterator &iter) const {return mPos != iter.mPos;}

      /// @brief Default assignment operator.
      Iterator& operator=(const Iterator& other) = default;

      /// @brief Return the linear offset of the current bit pointed to.
      uint32_t pos() const { return mPos; }

      /// @brief Convert to a boolean
      operator bool() const { return mPos != Mask::SIZE;}

      /// @brief Increment this Iterator to point to the next set bit.
      Iterator& operator++() { mPos = mParent->findNextOn(mPos+1); return *this; }

    private:

      uint32_t     mPos;
      const Mask*  mParent;
    }; // MaskIterator::Iterator class

    /// @brief Initialize all bits to zero.
    Mask() { for (uint32_t i = 0; i < COUNT; ++i) mWords[i]=0; }

    /// @brief Initialize all bits to the given state.
    Mask(bool on)
    {
        const uint64_t v = on ? ~uint64_t(0) : uint64_t(0);
        for (uint32_t i = 0; i < COUNT; ++i) mWords[i] = v;
    }

    /// @brief Copy constructor
    Mask(const Mask& other) { for (uint32_t i = 0; i < COUNT; ++i) mWords[i] = other.mWords[i]; }

    /// @brief Return the number of set bits in this Mask.
    uint32_t countOn() const
    {
        uint32_t sum = 0, n = COUNT;
        for (const uint64_t* w = mWords; n--; ++w) sum += CountOn(*w);
        return sum;
    }

    /// @brief Set the given bit on.
    void setOn(uint32_t n)  { mWords[n >> 6] |=    uint64_t(1) << (n & 63);  }

    /// @brief Set the given bit off.
    void setOff(uint32_t n)  { mWords[n >> 6] &= ~(uint64_t(1) << (n & 63));  }

    /// @brief Return true of the given bit is set.
    bool isOn(uint32_t n) const { return 0 != (mWords[n >> 6] & (uint64_t(1) << (n & 63))); }

    /// @brief Return an instance of an Iterator that point to the first set bit of this Mask.
    Iterator begin() const       { return Iterator(this->findFirstOn(), this); }

    /// @brief Return an instance of an Iterator point pointing to the end.
    /// @warning Does not point to a valid bit.
    Iterator end() const         { return Iterator(SIZE, this); }

    /// @brief Bitwise union with another mask.
    const Mask& operator|=(const Mask& other)
    {
        uint64_t *w1 = mWords;
        const uint64_t *w2 = other.mWords;
        for (uint32_t n = COUNT; n--;  ++w1, ++w2) *w1 |= *w2;
        return *this;
    }

    /// @brief Bitwise difference with another mask.
    const Mask& operator-=(const Mask& other)
    {
        uint64_t *w1 = mWords;
        const uint64_t *w2 = other.mWords;
        for (uint32_t n = COUNT; n--;  ++w1, ++w2) *w1 &= ~*w2;
        return *this;
    }
  private:

    /// @brief Return the number of set bits in the given 64 bit unsigned integer.
    static inline uint32_t CountOn(uint64_t v)
    {
        v = v - ((v >> 1) & uint64_t(0x5555555555555555));
        v = (v & uint64_t(0x3333333333333333)) + ((v >> 2) & uint64_t(0x3333333333333333));
        return (((v + (v >> 4)) & uint64_t(0xF0F0F0F0F0F0F0F)) * uint64_t(0x101010101010101)) >> 56;
    }

    /// @brief Return the first set bit in the given 64 bit unsigned integer.
    static inline uint32_t FindLowestOn(uint64_t v)
    {
      static const unsigned char DeBruijn[64] = {
          0,   1,  2, 53,  3,  7, 54, 27, 4,  38, 41,  8, 34, 55, 48, 28,
          62,  5, 39, 46, 44, 42, 22,  9, 24, 35, 59, 56, 49, 18, 29, 11,
          63, 52,  6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
          51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12,
      };
      return DeBruijn[uint64_t((v & -static_cast<int64_t>(v)) * uint64_t(0x022FDD63CC95386D)) >> 58];
    }

    /// @brief Return the linear offset to the first set bit in this mask.
    uint32_t findFirstOn() const
    {
        uint32_t n = 0;
        const uint64_t* w = mWords;
        for (; n<COUNT && !*w; ++w, ++n) ;
        return n==COUNT ? SIZE : (n << 6) + FindLowestOn(*w);
    }

    /// @brief Return the linear offset to the next set bit in this mask,
    /// starting from the given linear offset.
    uint32_t findNextOn(uint32_t start) const
    {
        uint32_t n = start >> 6;// find the word
        if (n >= COUNT) return SIZE;// check for out of bounds
        uint32_t m = start & 63;// local bit offset
        uint64_t b = mWords[n];// relevant word
        if (b & (uint64_t(1) << m)) return start;//simpel case: start is on
        b &= ~uint64_t(0) << m;// mask out lower bits
        while(!b && ++n<COUNT) b = mWords[n];// find next none-zero word
        return (!b ? SIZE : (n << 6) + FindLowestOn(b));//catch last word=0
    }

    // Only private member data of Mask
    uint64_t mWords[COUNT];
};// Mask class


// ----------------------------> Tree <--------------------------------------


/// @brief VDB Tree, which is a thin wrapper around a RootNode.
template <typename RootT>
class Tree
{
public:
    using RootType = RootT;
    using LeafNodeType = typename RootT::LeafNodeType;
    using ValueType = typename RootT::ValueType;

    /// @brief Initialize an empty tree with the given background value.
    Tree(const ValueType& background) : mRoot(background) {}

    /// @brief Default deep copy constructor
    Tree(const Tree& other) = default;

    /// @brief Return a const reference to the root node.
    const RootType& root() const { return mRoot; }

    /// @brief Return a non-const reference to the root node.
    RootType& root()  { return mRoot; }

    /// @brief Set the given coordinate to an active voxel with the given value.
    void setValue(const Coord& xyz, const ValueType& value) { mRoot.setValue(xyz, value); }

    /// @brief Return the value of the given voxel (regardless of state or location in the tree.)
    const ValueType& getValue(const Coord& xyz) const { return mRoot.getValue(xyz); }

    /// @brief Inset all leaf nodes in this Tree into the given list (with a push_back method).
    template <typename ListT>
    void getLeafNodes(ListT& list) const { mRoot.getLeafNodes(list); }

    /// @brief Return the total number of active values in this Tree, accounting
    /// for both active tiles and voxels.
    size_t activeValueCount() const { return mRoot.activeValueCount(); }

    /// @brief If the return value is true the given min/max coordinates were updated
    /// to define the bounding box of the active values in this Tree (accounting
    /// for both active tiles and voxels). Else this Tree contains no active values.
    bool getActiveBBox(Coord &min, Coord &max) const { return mRoot.getActiveBBox(min, max); }

    /// @breif Merges active values from the other Tree into this Tree.
    ///
    /// @details Only transfers active values from the other Tree that are inactive in
    /// this Tree. The state of values in the resulting Tree is the union the two
    /// original states.
    void merge(Tree& other) { mRoot.merge( other.mRoot ); }

    /// @brief Return a const reference to the background value.
    const ValueType& background() const { return mRoot.background(); }

    /// @brief Remove all values and nodes from this Tree, resulting in an empty Tree.
    void clear() { mRoot.clear(); }

private:

    // Only private data member of the Tree
    RootT mRoot;
};// Tree class


// --------------------------> RootNode <------------------------------------

/// @brief Forward decleration of accelerated random access class
template<typename TreeT> class ValueAccessor;

/// @brief Top-most node of the VDB tree structure.
template <typename ChildT>
class RootNode
{
public:
    using LeafNodeType = typename ChildT::LeafNodeType;
    using ChildNodeType = ChildT;
    using ValueType = typename ChildT::ValueType;

    /// @brief Construct an empty root node with the given background value.
    RootNode(const ValueType& background) : mBackground(background) {}

    /// @brief Deep copy constructor.
    RootNode(const RootNode& other) : mBackground(other.mBackground)
    {
        for (auto iter = other.mTable.begin(); iter != other.mTable.end(); ++iter) {
            if (iter->second.child == nullptr) {
                mTable[iter->first] = iter->second;
            } else {
                mTable[iter->first] = Tile(new ChildNodeType(*(iter->second.child)));
            }
        }
    }

    /// @brief Destructor
    ~RootNode() { this->clear(); }

    /// @brief Remove all values and nodes from this root node, resulting in an empty node.
    void clear()
    {
        for (auto iter = mTable.begin(); iter != mTable.end(); ++iter) {
          delete iter->second.child;
        }
        mTable.clear();
    }

    /// @brief Return a const reference to the background value.
    const ValueType& background() const { return mBackground; }

    /// @brief Set the voxel at the given coordinate to the given value. Its state
    /// will be active.
    void setValue(const Coord& xyz, const ValueType& value)
    {
        ChildT* child = nullptr;
        const Coord key = CoordToKey(xyz);
        auto iter = mTable.find(key);
        if (iter == mTable.end()) {
            child = new ChildT(xyz, mBackground, false);
            mTable[key] = Tile(child);
        } else if (iter->second.child != nullptr) {
            child = iter->second.child;
        } else if (iter->second.value != value) {
            child = new ChildT(xyz, iter->second.value, iter->second.state);
            iter->second.child = child;
        }
        if (child) child->setValue(xyz, value);
    }

    /// @brief Return the value of the given voxel, regardless of its state or
    /// location in the tree.
    const ValueType& getValue(const Coord& xyz) const
    {
        auto iter = mTable.find(CoordToKey(xyz));
        return iter == mTable.end() ? mBackground : iter->second.child ?
            iter->second.child->getValue(xyz) : iter->second.value;
    }

    /// @brief Inset all leaf nodes below this RootNode into the given list.
    template <typename ListT>
    void getLeafNodes(ListT& list) const
    {
        for (auto iter = mTable.begin(); iter != mTable.end(); ++iter) {
            if (iter->second.child) iter->second.child->getLeafNodes(list);
        }
    }

    /// @brief Return the total number of active values in an below this RootNode.
    /// @Note An active tiles counts for ChildNodeType::SIZE active values.
    size_t activeValueCount() const
    {
        size_t sum = 0;
        for (auto iter = mTable.begin(); iter != mTable.end(); ++iter) {
            if (iter->second.child) {
              sum += iter->second.child->activeValueCount();
            } else if (iter->second.state) {//active tile
              sum += ChildNodeType::SIZE;
            }
        }
        return sum;
    }

    /// @brief If the return value is true the given min/max coordinates were updated
    /// to define the bounding box of the active values in this Tree (accounting
    /// for both active tiles and voxels). Else this tree contains no active values.
    bool getActiveBBox(Coord &min, Coord &max) const
    {
        auto iter = mTable.begin();
        if (iter == mTable.end()) return false;
        min = Coord( std::numeric_limits<int>::max());
        max = Coord(-std::numeric_limits<int>::max());
        for (; iter != mTable.end(); ++iter) {
            if (iter->second.child) {
                iter->second.child->getActiveBBox(min, max);
            } else if (iter->second.state) {// encountered an active tile
                Coord c = iter->first;
                min.minComponent(c);
                max.maxComponent(c.offset(1<<ChildT::TOTAL));
            }
        }
        return true;
    }

    /// @breif Merges active values from the other root node into this root node.
    ///
    /// @details Only transfers active values from the other root node that are inactive in
    /// this root node. The state of values in the resulting node is the union the two
    /// original states.
    void merge(RootNode& other)
    {
        for (auto i = other.mTable.begin(), e = other.mTable.end(); i != e; ++i) {
            auto j = mTable.find(i->first);
            if (i->second.child) {//other node has a child
                if (j == mTable.end()) { // transfer other node's child
                    mTable[i->first] = Tile(i->second.child);
                    i->second.child = nullptr;
                } else if (j->second.child == nullptr) {
                    if (j->second.state == false) { // replace inactive tile with other node's child
                        j->second.child = i->second.child;
                        i->second.child = nullptr;
                    }
                } else { // merge both child nodes
                    j->second.child->merge(*(i->second.child));
                }
            } else if (i->second.state == true) {//other node has an active tile
                if (j == mTable.end()) { // insert other node's active tile
                    mTable[i->first] = Tile(i->second.value, i->second.state);
                } else if (j->second.state == false) {
                    // Replace anything except an active tile with the other node's active tile.
                    j->second.state = true;
                    j->second.value = i->second.value;
                }
            }
        }
        other.clear();
    }

private:

    template<typename> friend class ValueAccessor;
    template<typename> friend class Tree;

    /// @brief Private method to retun a voxel value and update a ValueAccessor
    template <typename AccessorT>
    const ValueType& getValueAndCache(const Coord& xyz, AccessorT& acc) const
    {
        auto iter = mTable.find(CoordToKey(xyz));
        if (iter == mTable.end()) return mBackground;
        if (iter->second.child) {
            acc.insert(xyz, iter->second.child);
            return iter->second.child->getValueAndCache(xyz, acc);
        }
        return iter->second.value;
    }

    /// @brief Private method to set a voxel value and update a ValueAccessor
    template <typename AccessorT>
    void setValueAndCache(const Coord& xyz, const ValueType& value, AccessorT& acc)
    {
        ChildT* child = nullptr;
        const Coord key = CoordToKey(xyz);
        auto iter = mTable.find(key);
        if (iter == mTable.end()) {
            child = new ChildT(xyz, mBackground, false);
            mTable[key] = Tile(child);
        } else if (iter->second.child != nullptr) {
            child = iter->second.child;
        } else if (iter->second.value != value) {
            child = new ChildT(xyz, iter->second.value, iter->second.state);
            iter->second.child = child;
        }
        if (child) {
            acc.insert(xyz, child);
            child->setValueAndCache(xyz, value, acc);
        }
    }

    /// @brief Private struct of a child pointer and a tile value and state
    struct Tile
    {
        Tile(ChildT* c = nullptr) : child(c) {}
        Tile(const ValueType& v, bool s) : child(nullptr), value(v), state(s) {}
        ChildT*   child;// a value of nullptr indicates its a tile with a value and state
        ValueType value;// tile value
        bool      state;// tile state
    };

    /// @brief Return a Cood key based on the coordinates of a voxel
    inline static Coord CoordToKey(const Coord& xyz) { return xyz & ~((1 << ChildT::TOTAL) - 1u); }
#ifdef USE_HASH_MAP
    struct Hash { inline size_t operator()(const Coord& ijk) const { return ijk.hash(); } };
    using MapT = std::unordered_map<Coord, Tile, Hash>;// unordered hash table
#else
    using MapT = std::map<Coord, Tile>;// sorted map
#endif

    // Private member data of the RootNode
    MapT      mTable;
    ValueType mBackground;// background value, i.e. value of any unset voxel
};// RootNode class


// --------------------------> InternalNode <------------------------------------


/// @brief Interal nodes of a VDB tree
template<typename ChildT, uint32_t LOG2DIM>
class InternalNode
{
public:
    using LeafNodeType = typename ChildT::LeafNodeType;
    using ChildNodeType = ChildT;
    using ValueType = typename ChildT::ValueType;

    static const uint32_t TOTAL = LOG2DIM + ChildT::TOTAL;//dimension
    static const uint32_t SIZE  = 1 << (3 * LOG2DIM);//number of
    static const uint32_t MASK  = (1 << TOTAL) - 1u;

    /// @brief Iterator that sequentially visits the child nodes of an internal node.
    /// @details A public member class of the InternalNode class.
    /// @warning This is not a STL-compliant iterator!
    class ChildIterator
    {
    public:
        /// @brief Constructor from an internal node.
        ChildIterator(const InternalNode& p) : mMaskIter(p.mChildMask.begin()), mParent(&p) {}
        ChildIterator& operator=(const ChildIterator&) = default;
        /// @brief Return the linear offset to the current child node.
        inline uint32_t pos() const { return mMaskIter.pos(); }
        /// @brief Return a const reference to the current child node.
        const ChildT& operator*() const { return *mParent->mTable[mMaskIter.pos()].child; }
        /// @brief Return a const pointer to the current child node.
        const ChildT* operator->() const { return mParent->mTable[mMaskIter.pos()].child; }
        /// @brief Conversion to a boolean
        operator bool() const { return mMaskIter;}
        /// @brief Increments this iterator to point to the next child node.
        ChildIterator& operator++() { ++mMaskIter; return *this; }
    private:
        typename Mask<LOG2DIM>::Iterator mMaskIter;
        const InternalNode* mParent;
    };// InternalNode::ChildIterator

    /// @brief Empty default constructor
    InternalNode() = default;

    /// @brief Constructs an InternalNode at the given
    InternalNode(const Coord& origin, const ValueType& value, bool state)
        : mOrigin(origin[0] & ~MASK, origin[1] & ~MASK, origin[2] & ~MASK), mValueMask(state), mChildMask(false)
    {
        for (uint32_t i = 0; i < SIZE; ++i) mTable[i].value = value;
    }

    /// @brief Deep copy constructor
    InternalNode(const InternalNode& other)
      : mOrigin(other.mOrigin), mValueMask(other.mValueMask), mChildMask(other.mChildMask)
    {
      for (uint32_t i=0; i < SIZE; ++i) {
        if (mChildMask.isOn(i)) {
          mTable[i].child = new ChildNodeType(*other.mTable[i].child);
        } else {
          mTable[i].value = other.mTable[i].value;
        }
      }
    }

    /// @brief Destructor
    ~InternalNode() { this->clear(); }

    /// @brief Set the voxel at the given coordinate to the given value. Its state
    /// will be active.
    void setValue(const Coord& xyz, const ValueType& value)
    {
        const uint32_t n = CoordToOffset(xyz);
        ChildT* child = mChildMask.isOn(n) ? mTable[n].child : nullptr;
        if (child == nullptr && value != mTable[n].value) {
            child = new ChildT(xyz, mTable[n].value, mValueMask.isOn(n));
            mTable[n].child = child;
            mChildMask.setOn(n);
        }
        if (child) child->setValue(xyz, value);
    }

    /// @brief Return the value of the given voxel, regardless of its state or
    /// location in the tree.
    inline const ValueType& getValue(const Coord& xyz) const
    {
        const uint32_t n = CoordToOffset(xyz);
        return mChildMask.isOn(n) ? mTable[n].child->getValue(xyz) : mTable[n].value;
    }

    /// @brief Inset all leaf nodes below this InternalNode into the given list.
    template <typename ListT>
    void getLeafNodes(ListT& list) const
    {
        for (ChildIterator iter(*this); iter; ++iter) iter->getLeafNodes(list);
    }

    /// @brief Return the total number of active values in an below this InternalNode.
    /// @Note An active tiles counts for ChildNodeType::SIZE active values.
    size_t activeValueCount() const
    {
        size_t sum = 0;
        for (uint32_t i = 0; i < SIZE; ++i) {
            if (mChildMask.isOn(i)) {
                sum += mTable[i].child->activeValueCount();
            } else if (mValueMask.isOn(i)) {//encountered an active tile
                sum += ChildNodeType::SIZE;
            }
        }
        return sum;
    }

    /// @brief Update the min/max coordinates to define the bounding box of the
    /// active values in this Tree (accounting for both active tiles and voxels).
    void getActiveBBox(Coord &min, Coord &max) const
    {
        for (uint32_t i = 0; i < SIZE; ++i) {
            if (mChildMask.isOn(i)) {
                mTable[i].child->getActiveBBox(min, max);
            } else if (mValueMask.isOn(i)) {// encountered an active tile
                Coord c = mOrigin;
                min.minComponent(c);
                max.maxComponent(c.offset(1<<ChildT::TOTAL));
            }
        }
    }

    /// @breif Merges active values from the other node into this node.
    ///
    /// @details Only transfers active values from the other node that are inactive in
    /// this node. The state of values in the resulting node is the union the two
    /// original states.
    void merge(InternalNode& other)
    {
        for (ChildIterator iter(other); iter; ++iter) {
            ChildNodeType* child = const_cast<ChildNodeType*>(&*iter);
            const uint32_t n = iter.pos();
            if (mChildMask.isOn(n)) {
                // Merge this node's child with the other node's child.
                mTable[n].child->merge(*child);
            } else if (!mValueMask.isOn(n)) {
                // Replace this node's inactive tile with the other node's child
                // and replace the other node's child with a tile of undefined value
                // (which is okay since the other tree is assumed to be cannibalized
                // in the process of merging).
                other.mChildMask.setOff(n);
                mChildMask.setOn(n);
                mTable[n].child=child;//transfer child
            }
        }
        // Copy active tile values.
        for (auto iter = other.mValueMask.begin(); iter; ++iter) {
            const uint32_t n = iter.pos();
            if (mChildMask.isOn(n) || mValueMask.isOn(n)) continue;//ignore child nodes and active tiles
            mTable[n].value = other.mTable[n].value;
            mValueMask.setOn(n);
        }
    }

private:

    template<typename> friend class ValueAccessor;
    template<typename> friend class RootNode;
    template<typename, uint32_t> friend class InternalNode;

    /// @brief Private method to retun a voxel value and update a ValueAccessor
    template<typename AccessorT>
    inline const ValueType& getValueAndCache(const Coord& xyz, AccessorT& acc) const
    {
        const uint32_t n = CoordToOffset(xyz);
        if (mChildMask.isOn(n)) {
            acc.insert(xyz, const_cast<ChildT*>(mTable[n].child));
            return mTable[n].child->getValueAndCache(xyz, acc);
        }
        return mTable[n].value;
    }

    /// @brief Private method to set a voxel value and update a ValueAccessor
    template<typename AccessorT>
    inline void setValueAndCache(const Coord& xyz, const ValueType& value, AccessorT& acc)
    {
      const uint32_t n = CoordToOffset(xyz);
      ChildT* child = mChildMask.isOn(n) ? mTable[n].child : nullptr;
      if (child == nullptr && value != mTable[n].value) {
          child = new ChildT(xyz, mTable[n].value, mValueMask.isOn(n));
          mTable[n].child = child;
          mChildMask.setOn(n);
      }
      if (child) {
           acc.insert(xyz, child);
          child->setValueAndCache(xyz, value, acc);
      }
    }

    /// @brief Remove all child nodes. Private since this results in an invalid node.
    void clear()
    {
      for (ChildIterator iter(*this); iter; ++iter) delete const_cast<ChildT*>(&(*iter));
    }

    /// @brief Return the linear offset corresponding to the given coordinate
    static uint32_t CoordToOffset(const Coord& xyz)
    {
        return (((xyz[0] & ((1 << TOTAL)-1u)) >> ChildT::TOTAL) << 2*LOG2DIM)
            +  (((xyz[1] & ((1 << TOTAL)-1u)) >> ChildT::TOTAL) <<   LOG2DIM)
            +   ((xyz[2] & ((1 << TOTAL)-1u)) >> ChildT::TOTAL);
    }

    // Private struct that unions a child node pointer and a tile value
    struct Tile
    {
        Tile(ChildT* c = nullptr) : child(c) {}
        union { ChildT* child; ValueType value; };
    };

    // Private data memebers of the InternalNode
    Coord         mOrigin;
    Mask<LOG2DIM> mValueMask;
    Mask<LOG2DIM> mChildMask;
    Tile          mTable[SIZE];
};// InternalNode class


// --------------------------> LeafNode <------------------------------------


/// @brief Leaf nodes of the VDB tree.
template <typename ValueT, uint32_t LOG2DIM>
class LeafNode
{
public:
    using LeafNodeType = LeafNode<ValueT, LOG2DIM>;
    using ValueType = ValueT;

    static const uint32_t TOTAL = LOG2DIM;            // needed by parent nodes
    static const uint32_t SIZE  = 1 << 3 * LOG2DIM;   // total number of voxels represented by this node
    static const uint32_t MASK  = (1 << LOG2DIM) - 1u;// mask for bit operations

    /// @brief Iterator that sequentially visits the active voxels of a leaf node.
    /// @details A public member class of the LeafNode class.
    /// @warning This is not a STL-compliant iterator!
    class Iterator
    {
    public:
        /// @brief Constructor from a leaf node.
        Iterator(const LeafNode& p) : mMaskIter(p.mValueMask.begin()), mParent(&p) {}
        Iterator& operator=(const Iterator&) = default;
        /// @brief Return the global index coordinate of the current active voxel.
        Coord getCoord() const
        {
            const Coord& origin =  mParent->origin();
            uint32_t n = mMaskIter.pos();
            Coord xyz( (n >> 2*LOG2DIM) + origin[0] );
            n &= ((1<<2*LOG2DIM)-1u);
            xyz[1] = (n >> LOG2DIM) + origin[1];
            xyz[2] = (n & LeafNode::MASK) + origin[2];
            return xyz;
        }
        /// @brief Conversion to a boolean.
        operator bool() const { return mMaskIter;}
        /// @brief Increments this operator to the next active voxel.
        Iterator& operator++() { ++mMaskIter; return *this; }
    private:
        typename Mask<LOG2DIM>::Iterator mMaskIter;
        const LeafNode* mParent;
    };// LeafNode::Iterator class

    /// @brief Empty default constructor.
    LeafNode() = default;

    /// @brief Constructs a leaf node at the given origin with constant values and states.
    LeafNode(const Coord& xyz, const ValueType& value, bool state)
        : mOrigin(xyz & (~((1 << LOG2DIM) - 1u))), mValueMask(state)
    {
      ValueType* target = mTable;
      uint32_t n = SIZE;
      while (n--) *target++ = value;
    }

    /// @brief Copy constructor from another leaf node, including an OpenVDB node.
    template <typename OtherLeafNodeT>
    LeafNode(const OtherLeafNodeT& other)
      : mOrigin(other.origin()[0], other.origin()[1], other.origin()[2])
      , mValueMask(other.getValueMask())
    {
      for (uint32_t i=0; i<SIZE; ++i) mTable[i] = other.getValue(i);
    }

    /// @brief Default destructor;
    ~LeafNode() = default;

    /// @brief Return a const reference to the origin of this leaf node
    const Coord& origin() const { return mOrigin; }

    /// @brief Set the voxel value at the given coordinate
    inline void setValue(const Coord& xyz, const ValueType& value)
    {
      const uint32_t n = CoordToOffset(xyz);
      mValueMask.setOn(n);
      mTable[n] = value;
    }

    /// @brief Returm the voxel value at the given linear offset.
    inline const ValueType& getValue(uint32_t n) const { return mTable[n]; }

    /// @brief Return the voxel value at the given coordinate.
    inline const ValueType& getValue(const Coord& xyz) const { return mTable[CoordToOffset(xyz)]; }

    /// @brief Insert this leaf node into the given list.
    template <typename ListT>
    void getLeafNodes(ListT& list) const { list.push_back(this); }

    /// @brief Return a const reference to the value mask.
    const Mask<LOG2DIM>& valueMask() const { return mValueMask; }

    /// @brief Return the nuumber of active voxels in this leaf node.
    size_t activeValueCount() const { return mValueMask.countOn(); }

    /// @brief Update the min/max coordinates to include the active values in this
    /// leaf node.
    void getActiveBBox(Coord &min, Coord &max) const
    {
        for (Iterator iter(*this); iter; ++iter) {
            const Coord c = iter.getCoord();
            min.minComponent(c);
            max.maxComponent(c);
        }
    }
    /// @breif Merges active values from the other leaf node into this leafnode.
    ///
    /// @details Transfer active values from the other node that are inactive in
    /// this node, and union the two bit masks.
    void merge(const LeafNode& other)
    {
        Mask<LOG2DIM> mask = other.mValueMask;// deep copy
        mask -= mValueMask;// bitwise difference
        for (auto iter = mask.begin(); iter; ++iter) {
            mTable[iter.pos()] = other.mTable[iter.pos()];
        }
        mValueMask |= other.mValueMask;// bitwise union
    }

private:

    template<typename> friend class ValueAccessor;
    template<typename> friend class RootNode;
    template<typename, uint32_t> friend class InternalNode;

    /// @brief Private method to retun a voxel value and update a (dummy) ValueAccessor
    template<typename AccessorT>
    inline const ValueType& getValueAndCache(const Coord& xyz, const AccessorT&) const
    {
      return mTable[CoordToOffset(xyz)];
    }

    /// @brief Private method to set a voxel value and update a (dummy) ValueAccessor
    template<typename AccessorT>
    inline void setValueAndCache(const Coord& xyz, const ValueType& value, const AccessorT&)
    {
      this->setValue(xyz, value);
    }

    /// @brief Return the linear offset corresponding to the given coordinate
    static uint32_t CoordToOffset(const Coord& xyz)
    {
      return ((xyz[0] & MASK) << 2*LOG2DIM) + ((xyz[1] & MASK) << LOG2DIM) + (xyz[2] & MASK);
    }

    // Privatre member data
    Coord         mOrigin;
    Mask<LOG2DIM> mValueMask;
    ValueType     mTable[SIZE];
};// LeafNode class

// --------------------------> ValueAccessor <------------------------------------

/// @brief ValueAcessor with three levels of node caching.
///
/// @details Used to accelerated random access into a VDB tree. Provides on
/// average O(1) random access operations by means of inverse tree traversal,
/// which amortizes the non-const time complexity of the root node.
template<typename TreeT>
class ValueAccessor
{
public:
    using RootT = typename TreeT::RootType;
    using ValueType = typename RootT::ValueType;
    using NodeT2 = typename RootT::ChildNodeType;
    using NodeT1 = typename NodeT2::ChildNodeType;
    using NodeT0 = typename NodeT1::ChildNodeType;

    /// Constructor from a tree
    ValueAccessor(TreeT& tree) : mKey0(std::numeric_limits<int>::max())
                               , mKey1(std::numeric_limits<int>::max())
                               , mKey2(std::numeric_limits<int>::max())
                               , mNode0(nullptr), mNode1(nullptr), mNode2(nullptr), mTree(&tree)
    {
    }

    /// @brief Return true if a leaf node is cashed with the given coordinate key
    inline bool isCached0(const Coord& xyz) const
    {
        return (xyz[0] & int(~NodeT0::MASK)) == mKey0[0]
            && (xyz[1] & int(~NodeT0::MASK)) == mKey0[1]
            && (xyz[2] & int(~NodeT0::MASK)) == mKey0[2];
    }

    /// @brief Return true if an upper internal node is cashed with the given coordinate key
    inline bool isCached1(const Coord& xyz) const
    {
        return (xyz[0] & int(~NodeT1::MASK)) == mKey1[0]
            && (xyz[1] & int(~NodeT1::MASK)) == mKey1[1]
            && (xyz[2] & int(~NodeT1::MASK)) == mKey1[2];
    }

    /// @brief Return true if a lower internal node is cashed with the given coordinate key
    inline bool isCached2(const Coord& xyz) const
    {
        return (xyz[0] & int(~NodeT2::MASK)) == mKey2[0]
            && (xyz[1] & int(~NodeT2::MASK)) == mKey2[1]
            && (xyz[2] & int(~NodeT2::MASK)) == mKey2[2];
    }

    /// Return the value of the voxel at the given coordinates.
    inline const ValueType& getValue(const Coord& xyz) const
    {
      if (this->isCached0(xyz)) {
          return mNode0->getValue(xyz);
      } else if (this->isCached1(xyz)) {
          return mNode1->getValueAndCache(xyz, const_cast<ValueAccessor&>(*this));
      } else if (this->isCached2(xyz)) {
          return mNode2->getValueAndCache(xyz, const_cast<ValueAccessor&>(*this));
      }
      return mTree->root().getValueAndCache(xyz, const_cast<ValueAccessor&>(*this));
    }

    /// @brief Set the value of the voxel at the given coordinates.
    inline void setValue(const Coord& xyz, const ValueType& value)
    {
        if (this->isCached0(xyz)) {
           mNode0->setValue(xyz, value);
        } else if (this->isCached1(xyz)) {
           mNode1->setValueAndCache(xyz, value, *this);
        } else if (this->isCached2(xyz)) {
           mNode2->setValueAndCache(xyz, value, *this);
        } else {
           mTree->root().setValueAndCache(xyz, value, *this);
        }
    }

private:

    // Allow nodes to insert themselves into the cache.
    template<typename> friend class RootNode;
    template<typename, uint32_t> friend class InternalNode;
    template<typename, uint32_t> friend class LeafNode;

    /// @brief Inserts a leaf node and key pair into this ValueAccessor
    inline void insert(const Coord& xyz, const NodeT0* node)
    {
        mKey0  = xyz & int(~NodeT0::MASK);
        mNode0 = const_cast<NodeT0*>(node);
    }

    /// @brief Inserts a a lower internal node and key pair into this ValueAccessor
    inline void insert(const Coord& xyz, const NodeT1* node)
    {
        mKey1  = xyz & int(~NodeT1::MASK);
        mNode1 = const_cast<NodeT1*>(node);
    }

    /// @brief Inserts an upper internal node and key pair into this ValueAccessor
    inline void insert(const Coord& xyz, const NodeT2* node)
    {
        mKey2  = xyz & int(~NodeT2::MASK);
        mNode2 = const_cast<NodeT2*>(node);
    }

    // Private member data of this ValueAccessor
    Coord mKey0, mKey1, mKey2;
    NodeT0* mNode0;// leaf node
    NodeT1* mNode1;// lower internal node
    NodeT2* mNode2;// upper internal node
    TreeT*  mTree;// tree
}; // ValueAccessor class

/// @brief Default configuration used in OpenVDB
using FloatTree = Tree<RootNode<InternalNode<InternalNode<LeafNode<float, 3>, 4>, 5> > >;

}//end of vdb namespace

#endif