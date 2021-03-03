//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2020 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOS_vector_H
#define GPOS_vector_H

#include <vector>

#include "gpos/memory/MemoryPoolAllocator.h"

namespace gpos
{
template <class T>
using vector = std::vector<T, MemoryPoolAllocator<T>>;
}
#endif	//GPOS_vector_H
