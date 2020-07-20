#include "Timer.h"
#include "Laplacian.h"

#include <iomanip>

#include <SPGrid/Core/SPGrid_Allocator.h>
#include <SPGrid/Core/SPGrid_Page_Map.h>


struct MyStruct
{
    float u;
    float Lu;
    uint32_t mask;
};

enum MyFlags : uint32_t {
    uExistsFlag = 0x00000001,
    LuExistsFlag = 0x00000002
};

int main(int argc, char *argv[])
{
    using array_t = float (&) [XDIM][YDIM][ZDIM];
    
    float *uRaw = new float [XDIM*YDIM*ZDIM];
    float *LuRaw = new float [XDIM*YDIM*ZDIM];
    array_t u = reinterpret_cast<array_t>(*uRaw);
    array_t Lu = reinterpret_cast<array_t>(*LuRaw);

    using AllocatorType = SPGrid::SPGrid_Allocator<MyStruct,3>;
    using PageMapType = SPGrid::SPGrid_Page_Map<>;
    using FloatMaskType = AllocatorType::Array_mask<float>;
    AllocatorType allocator(XDIM, YDIM, ZDIM);
    PageMapType pageMap(allocator);

    auto u_array = allocator.Get_Array(&MyStruct::u);
    auto Lu_array = allocator.Get_Array(&MyStruct::Lu);
    auto mask_array = allocator.Get_Array(&MyStruct::mask);

    // Initialization

    for (int i = 0; i < XDIM; i++)
    for (int j = 0; j < YDIM; j++)
    for (int k = 0; k < ZDIM; k++)
    {
        // uint64_t radiusSquared =
        //     uint64_t(i-XDIM/2) * uint64_t(i-XDIM/2) +
        //     uint64_t(j-YDIM/2) * uint64_t(j-YDIM/2) +
        //     uint64_t(k-ZDIM/2) * uint64_t(k-ZDIM/2);
        // if(radiusSquared > ... || radiusSquared < ...) continue;

        auto floatOffset = FloatMaskType::Linear_Offset(i, j, k);
        u[i][j][k]  = u_array(floatOffset)  = (float) (i+j+k);
        Lu[i][j][k] = Lu_array(floatOffset) = (float) 0;
        mask_array(i, j, k) |= MyFlags::uExistsFlag;
        pageMap.Set_Page(floatOffset);
    }

    for (int i = 1; i < XDIM-1; i++)
    for (int j = 1; j < YDIM-1; j++)
    for (int k = 1; k < ZDIM-1; k++)
        mask_array(i, j, k) |= MyFlags::LuExistsFlag;

    pageMap.Update_Block_Offsets();

    Timer timer;

    for(int test = 1; test <= 50; test++)
    {
        std::cout << "Running test iteration " << std::setw(2) << test << " ";
        timer.Start();
        //ComputeLaplacian(u, Lu);

        auto blocks = pageMap.Get_Blocks();
        const auto nElementsPerBlock = allocator.Elements_Per_Block();
        
        std::cout << "blocks.second = " <<blocks.second << std::endl;

        #pragma omp parallel for
        for (int b = 0; b < blocks.second; b++) {

            const auto blockOffset = blocks.first[b];
            const auto maskPtr = &mask_array(blockOffset);
            auto offset = blockOffset;

            for (int e = 0; e < nElementsPerBlock; e++, offset += sizeof(float))
                if (maskPtr[e] & MyFlags::LuExistsFlag)
                    Lu_array(offset) =
                        -6 * u_array.operator()<0,0,0>(offset)
                           + u_array.operator()<+1,0,0>(offset)
                           + u_array.operator()<-1,0,0>(offset)
                           + u_array.operator()<0,+1,0>(offset)
                           + u_array.operator()<0,-1,0>(offset)
                           + u_array.operator()<0,0,+1>(offset)
                           + u_array.operator()<0,0,-1>(offset);


        }
        
        timer.Stop("Elapsed time : ");
    }
    
    return 0;
}