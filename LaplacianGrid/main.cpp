#include "Timer.h"
#include "Laplacian.h"
#include "Init.h"

#include <iomanip>




int main(int argc, char *argv[])
{
    
    
    float *uRaw = new float [XDIM*YDIM*ZDIM];
    float *LuRaw = new float [XDIM*YDIM*ZDIM];
    array_t u = reinterpret_cast<array_t>(*uRaw);
    array_t Lu = reinterpret_cast<array_t>(*LuRaw);

    // using AllocatorType = SPGrid::SPGrid_Allocator<MyStruct,3>;
    // using PageMapType = SPGrid::SPGrid_Page_Map<>;
    // using FloatMaskType = AllocatorType::Array_mask<float>;
    AllocatorType allocator(XDIM, YDIM, ZDIM);
    PageMapType pageMap(allocator);

    DataArrayType u_array = allocator.Get_Array(&MyStruct::u);
    DataArrayType Lu_array = allocator.Get_Array(&MyStruct::Lu);
    MaskArrayType mask_array = allocator.Get_Array(&MyStruct::mask);

    // Initialization
    initialize( u, Lu, mask_array, pageMap, u_array,Lu_array);

    Timer timer;

    for(int test = 1; test <= 50; test++)
    {
        std::cout << "Running test iteration " << std::setw(2) << test << " ";
        timer.Start();
        //ComputeLaplacian(u, Lu);

        SPGridLaplacian(u_array, Lu_array, mask_array, allocator.Elements_Per_Block(), pageMap.Get_Blocks().second, pageMap.Get_Blocks().first);
        
        timer.Stop("Elapsed time : ");
    }
    
    return 0;
}