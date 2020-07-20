#ifndef __Init__
#define __Init__


#include "Config.h"

namespace SPGrid{


void initialize(array_t& x, array_t& f, array_t& p, array_t& r, array_t& z, MaskArrayType& mask_array, PageMapType& pageMap, DataArrayType& x_array,
 DataArrayType& f_array, DataArrayType& p_array, DataArrayType& r_array, DataArrayType& z_array){
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
        x[i][j][k]  = x_array(floatOffset)  = (float) (i+j+k);
        f[i][j][k]  = f_array(floatOffset)  = (float) (i+j+k);
        p[i][j][k] = p_array(floatOffset) = (float) 0;
        r[i][j][k] = r_array(floatOffset) = (float) 0;
        z[i][j][k] = z_array(floatOffset) = (float) 0;
        mask_array(i, j, k) |= MyFlags::uExistsFlag;
        pageMap.Set_Page(floatOffset);
    }

    for (int i = 1; i < XDIM-1; i++)
    for (int j = 1; j < YDIM-1; j++)
    for (int k = 1; k < ZDIM-1; k++)
        mask_array(i, j, k) |= MyFlags::zExistsFlag;

    pageMap.Update_Block_Offsets();
};


}

#endif