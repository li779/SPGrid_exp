#include <SPGrid/Core/SPGrid_Allocator.h>
#include <SPGrid/Core/SPGrid_Page_Map.h>
#include <openvdb/tree/LeafManager.h>
#include <openvdb/tree/LeafBuffer.h>

int DIM = 512;
int BufferCount = 1;

template<typename TreeT>
inline void allocateWithSPGrid(TreeT& tree);

template<typename T>
struct MyStruct
{
    T field;
};


template<typename TreeT>
class SPGridHelper{
    public:
    typedef typename TreeT::ValueType    ValueT;
    typedef typename TreeT::RootNodeType RootT;
    typedef typename TreeT::LeafNodeType LeafT;
    typedef typename LeafT::Buffer       BufferT;
    typedef typename openvdb::tree::LeafManager<TreeT>  LeafManagerT;
    typedef typename SPGrid::SPGrid_Allocator<MyStruct<ValueT>,3> AllocatorType;
    typedef typename SPGrid::SPGrid_Page_Map<> PageMapType;
    typedef typename AllocatorType::template Array_type<ValueT> DataArrayType;

    struct InitBuffers
    {
        InitBuffers(SPGridHelper& sPGridHelper): mSPGridHelper(sPGridHelper){}
        template <typename LeafNodeType>
        void operator()(LeafNodeType &lhsLeaf, size_t) const
         {
            openvdb::Int x,y,z;
            lhsLeaf.getOrigin(x,y,z);
            SPGridHelper::BufferT newBuffer(mSPGridHelper.mDataArrays(x,y,z));
            lhsLeaf.swap(newBuffer);
         }
        SPGridHelper& mSPGridHelper;
    };
    
    SPGridHelper(TreeT tree)
        :mLeafManager(new openvdb::tree::LeafManager<TreeT>(tree,BufferCount)),
        mSPGridAllocator(DIM,DIM,DIM),
        mDataArrays(mSPGridAllocator.Get_Array(&MyStruct<ValueT>::field)),
        {
            //tree.evalLeafBoundingBox(TreeSize)
    }

    void inline Init(){
        mLeafManager->foreach(InitBuffers(*this), false);
    }

    private:
    LeafManagerT* mLeafManager;
    AllocatorType mSPGridAllocator;
    DataArrayType mDataArrays;
    //CoordBBox TreeSize;
};

template<typename TreeT>
inline void allocateWithSPGrid(TreeT& tree){
    SPGridHelper<TreeT> help(tree);
    help.Init();
}

