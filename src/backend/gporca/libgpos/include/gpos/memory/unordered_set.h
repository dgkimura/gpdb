//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOS_unordered_set_H
#define GPOS_unordered_set_H

#include <unordered_set>

#include "gpos/memory/MemoryPoolAllocator.h"

namespace gpos
{
template <class Key, class Hash = std::hash<Key>,
		  class KeyEqual = std::equal_to<>>
using unordered_set =
	std::unordered_set<Key, Hash, KeyEqual, gpos::MemoryPoolAllocator<Key>>;
}

#endif	// GPOS_unordered_set_H
