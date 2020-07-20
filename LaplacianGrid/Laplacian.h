#include "Config.h"
#include <iostream>


namespace SPGrid {
void SPGridLaplacian(DataArrayType& u_array, DataArrayType& Lu_array, MaskArrayType& mask_array,const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks);
}