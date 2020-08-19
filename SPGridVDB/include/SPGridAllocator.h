#include <SPGrid/Core/SPGrid_Allocator.h>
#include <SPGrid/Core/SPGrid_Page_Map.h>
#include <openvdb/tree/LeafManager.h>

int DIM = 512;

template<typename TreeT>
inline void allocateWithSPGrid(TreeT& tree);

template<typename TreeT>
class SPGridHelper{
    public:
    typedef typename TreeT::ValueType    ValueT;
    typedef typename TreeT::RootNodeType RootT;
    typedef typename TreeT::LeafNodeType LeafT;
    typedef typename LeafT::Buffer       BufferT;
    typedef typename openvdb::tree::LeafManager<TreeT>  LeafManagerT;
    typedef typename SPGrid::SPGrid_Allocator<ValueT,3> AllocatorType;
    typedef typename SPGrid::SPGrid_Page_Map<> PageMapType;
    

    SPGridHelper(TreeT tree)
        :mLeafManager(new openvdb::tree::LeafManager<TreeT>(tree,1)),
        mSPGridAllocator(DIM,DIM,DIM)
        {
    }

    void swapWithGrid(){
        
    }

    private:
    LeafManagerT* mLeafManager;
    AllocatorType mSPGridAllocator;

};

template<typename TreeT>
inline void allocateWithSPGrid(TreeT& tree){
    SPGridHelper<TreeT> help(tree);
}

