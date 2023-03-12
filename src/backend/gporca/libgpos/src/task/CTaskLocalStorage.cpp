//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CTaskLocalStorage.cpp
//
//	@doc:
//		Task-local storage implementation
//---------------------------------------------------------------------------

#include "gpos/task/CTaskLocalStorage.h"

#include "gpos/common/CSyncHashtableAccessByKey.h"
#include "gpos/task/CTaskLocalStorageObject.h"

using namespace gpos;


// shorthand for HT accessor
using HashTableAccessor = CSyncHashtableAccessByKey<CTaskLocalStorageObject,
													CTaskLocalStorage::Etlsidx>;


// invalid idx
const CTaskLocalStorage::Etlsidx CTaskLocalStorage::m_invalid_idx =
	EtlsidxInvalid;

CTaskLocalStorage::CTaskLocalStorage(CMemoryPool *mp) : m_hash_table(mp)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CTaskLocalStorage::~CTaskLocalStorage
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CTaskLocalStorage::~CTaskLocalStorage() = default;

//---------------------------------------------------------------------------
//	@function:
//		CTaskLocalStorage::Reset
//
//	@doc:
//		(re-)init TLS
//
//---------------------------------------------------------------------------
void
CTaskLocalStorage::Reset(CMemoryPool *)
{
	// destroy old
	m_hash_table.Clear();
}


//---------------------------------------------------------------------------
//	@function:
//		CTaskLocalStorage::Store
//
//	@doc:
//		Store object in TLS
//
//---------------------------------------------------------------------------
void
CTaskLocalStorage::Store(gpos::Ref<CTaskLocalStorageObject> obj)
{
	GPOS_ASSERT(nullptr != obj);

#ifdef GPOS_DEBUG
	{
		GPOS_ASSERT(nullptr == m_hash_table.Find(obj->idx()) &&
					"Duplicate TLS object key");
	}
#endif	// GPOS_DEBUG

	m_hash_table.Insert(obj->idx(), obj);
}


//---------------------------------------------------------------------------
//	@function:
//		CTaskLocalStorage::Get
//
//	@doc:
//		Lookup object in TLS
//
//---------------------------------------------------------------------------
gpos::Ref<CTaskLocalStorageObject>
CTaskLocalStorage::Get(CTaskLocalStorage::Etlsidx idx)
{
	return m_hash_table.Find(idx);
}


//---------------------------------------------------------------------------
//	@function:
//		CTaskLocalStorage::Remove
//
//	@doc:
//		Delete given object from TLS
//
//---------------------------------------------------------------------------
void
CTaskLocalStorage::Remove(gpos::Ref<CTaskLocalStorageObject> obj)
{
	GPOS_ASSERT(nullptr != obj);

	// lookup object
	GPOS_ASSERT(nullptr != m_hash_table.Find(obj->idx()) &&
				"Object not found in TLS");

	m_hash_table.Delete(obj->idx());
}

// EOF
