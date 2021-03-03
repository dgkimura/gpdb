//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOS_unordered_map_H
#define GPOS_unordered_map_H
#include <unordered_map>

#include "gpos/memory/MemoryPoolAllocator.h"
namespace gpos
{
template <class Key, class T, class Hash = std::hash<Key>,
		  class KeyEqual = std::equal_to<>>
using unordered_map =
	std::unordered_map<Key, T, Hash, KeyEqual,
					   gpos::MemoryPoolAllocator<std::pair<const Key, T>>>;
}
#endif	//GPOS_unordered_map_H
