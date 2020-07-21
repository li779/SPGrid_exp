#include <SPGrid/Core/SPGrid_Allocator.h>
#include <SPGrid/Core/SPGrid_Page_Map.h>
#pragma once

// #define XDIM 512
// #define YDIM 512
// #define ZDIM 512


using namespace SPGrid;

struct ConjugateGradientStruct
{
    float x,f,p,r,z;
    uint32_t mask;
};

enum MyFlags : uint32_t {
    uExistsFlag = 0x00000001,
    zExistsFlag = 0x00000002
};

using AllocatorType = SPGrid::SPGrid_Allocator<ConjugateGradientStruct,3>;
using PageMapType = SPGrid::SPGrid_Page_Map<>;
using FloatMaskType = AllocatorType::Array_mask<float>;
using DataArrayType = AllocatorType::Array_type<float>;
using MaskArrayType = AllocatorType::Array_type<uint32_t>;
using array_t = float (&) [XDIM][YDIM][ZDIM];