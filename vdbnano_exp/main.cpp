#include "vdb_nano.h"
#include "Timer.h"
#include <math.h>
#include "Laplacian.h"
#include "SPGridOperator.h"

// #define XDIM 512
// #define YDIM 512
// #define ZDIM 512
// #define LIMIT 0.001

using namespace vdb;

enum MyFlags : uint32_t {
    uExistsFlag = 0x00000001,
    LuExistsFlag = 0x00000002
};

using VDBTree = Tree<RootNode<InternalNode<InternalNode<LeafNode<MyStruct, 3>, 4>, 5> > >;
using VDBAccessor = ValueAccessor<VDBTree>;
using array_t = float (&) [XDIM][YDIM][ZDIM];
using SPGridOperatorType = SPGridOperator<VDBTree>;


void initialize(array_t& u, array_t& Lu, VDBAccessor& accessor){
    for (int i = 0; i < XDIM; i++)
    for (int j = 0; j < YDIM; j++)
    for (int k = 0; k < ZDIM; k++)
    {
        
        const Coord index(i,j,k);
        MyStruct data;
        u[i][j][k]  = data.u  = (float) (i+j+k);
        Lu[i][j][k] = data.Lu = (float) 0;
        data.mask |= MyFlags::uExistsFlag;
        if ((i>0 && i<XDIM-1) &&
            (j>0 && j<XDIM-1) &&
            (k>0 && k<XDIM-1))
            data.mask |= MyFlags::LuExistsFlag;
        accessor.setValue(index, data);
    }
}

void vdbLaplacian(VDBAccessor& accessor){
    for (int i = 1; i < XDIM-1; i++)
    for (int j = 1; j < YDIM-1; j++)
    for (int k = 1; k < ZDIM-1; k++)
    {
        
        float result = -6 * accessor.getValue(Coord(i,j,k)).u
            + accessor.getValue(Coord(i+1,j,k)).u
            + accessor.getValue(Coord(i-1,j,k)).u
            + accessor.getValue(Coord(i,j+1,k)).u
            + accessor.getValue(Coord(i,j-1,k)).u
            + accessor.getValue(Coord(i,j,k+1)).u
            + accessor.getValue(Coord(i,j,k-1)).u;
        MyStruct newdata;
        newdata.copy(accessor.getValue(Coord(i,j,k)));
        newdata.Lu = result;
        accessor.setValue(Coord(i,j,k),newdata);
    }
}

bool checkLaplacian(array_t& Lu, VDBAccessor& accessor){
    for (int i = 0; i < XDIM; i++)
    for (int j = 0; j < YDIM; j++)
    for (int k = 0; k < ZDIM; k++)
    {
        if (std::abs(Lu[i][j][k]-accessor.getValue(Coord(i,j,k)).Lu)>LIMIT){
            std::cout << "Wrong!";
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    
    float *uRaw = new float [XDIM*YDIM*ZDIM];
    float *LuRaw = new float [XDIM*YDIM*ZDIM];
    array_t u = reinterpret_cast<array_t>(*uRaw);
    array_t Lu = reinterpret_cast<array_t>(*LuRaw);

    // using AllocatorType = SPGrid::SPGrid_Allocator<MyStruct,3>;
    // using PageMapType = SPGrid::SPGrid_Page_Map<>;
    // using FloatMaskType = AllocatorType::Array_mask<float>;
    MyStruct defaultStruct;
    VDBTree tree(defaultStruct);
    VDBAccessor accessor(tree);

    
    // Initialization
    initialize( u, Lu, accessor);
    SPGridOperatorType SPGridOperator;

    Timer timer;

    for(int test = 1; test <= 50; test++)
    {
        std::cout << "Running test iteration " << test << " ";
        timer.Start();
        ComputeLaplacian(u, Lu);

        vdbLaplacian(accessor);
        
        timer.Stop("Elapsed time : ");
        std::cout << "Calculation is " << checkLaplacian(Lu, accessor) <<std::endl;
    }
    
    return 0;

}


