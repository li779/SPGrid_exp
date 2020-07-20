#include "Laplacian.h"
#include "CSRMatrixHelper.h"
#include "MatVecMultiply.h"

namespace SPGrid{
void SPGridLaplacian(DataArrayType& u_array, DataArrayType& Lu_array, MaskArrayType& mask_array,const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks)
{
    
    std::cout << "blocks.second = " <<block_size << std::endl;

    #pragma omp parallel for
    for (int b = 0; b < block_size; b++) {

        const auto blockOffset = blocks[b];
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
            
}
}
