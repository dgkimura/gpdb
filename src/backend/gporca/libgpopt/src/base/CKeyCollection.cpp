//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CKeyCollection.cpp
//
//	@doc:
//		Implementation of key collections
//---------------------------------------------------------------------------

#include "gpopt/base/CKeyCollection.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CUtils.h"

using namespace gpopt;

FORCE_GENERATE_DBGSTR(CKeyCollection);

//---------------------------------------------------------------------------
//	@function:
//		CKeyCollection::CKeyCollection
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CKeyCollection::CKeyCollection(CMemoryPool *mp) : m_pdrgpcrs(nullptr)
{
	GPOS_ASSERT(nullptr != mp);

	m_pdrgpcrs = GPOS_NEW(mp) CColRefSetArray(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CKeyCollection::CKeyCollection
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CKeyCollection::CKeyCollection(CMemoryPool *mp, gpos::Ref<CColRefSet> pcrs)
	: m_pdrgpcrs(nullptr)
{
	GPOS_ASSERT(nullptr != pcrs && 0 < pcrs->Size());

	m_pdrgpcrs = GPOS_NEW(mp) CColRefSetArray(mp);

	// we own the set
	Add(std::move(pcrs));
}


//---------------------------------------------------------------------------
//	@function:
//		CKeyCollection::CKeyCollection
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CKeyCollection::CKeyCollection(CMemoryPool *mp,
							   gpos::Ref<CColRefArray> colref_array)
	: m_pdrgpcrs(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != colref_array && 0 < colref_array->Size());

	m_pdrgpcrs = GPOS_NEW(mp) CColRefSetArray(mp);

	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp);
	pcrs->Include(colref_array.get());
	Add(std::move(pcrs));

	// we own the array
	;
}


//---------------------------------------------------------------------------
//	@function:
//		CKeyCollection::~CKeyCollection
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CKeyCollection::~CKeyCollection()
{
	;
}


//---------------------------------------------------------------------------
//	@function:
//		CKeyCollection::Add
//
//	@doc:
//		Add key set to collection; takes ownership
//
//---------------------------------------------------------------------------
void
CKeyCollection::Add(gpos::Ref<CColRefSet> pcrs)
{
	GPOS_ASSERT(!FKey(pcrs.get()) && "no duplicates allowed");

	m_pdrgpcrs->Append(std::move(pcrs));
}


//---------------------------------------------------------------------------
//	@function:
//		CKeyCollection::FKey
//
//	@doc:
//		Check if set constitutes key
//
//---------------------------------------------------------------------------
BOOL
CKeyCollection::FKey(const CColRefSet *pcrs,
					 BOOL fExactMatch  // true: match keys exactly,
									   //  false: match keys by inclusion
) const
{
	const ULONG ulSets = m_pdrgpcrs->Size();
	for (ULONG ul = 0; ul < ulSets; ul++)
	{
		if (fExactMatch)
		{
			// accept only exact matches
			if (pcrs->Equals((*m_pdrgpcrs)[ul].get()))
			{
				return true;
			}
		}
		else
		{
			// if given column set includes a key, then it is also a key
			if (pcrs->ContainsAll((*m_pdrgpcrs)[ul].get()))
			{
				return true;
			}
		}
	}

	return false;
}



//---------------------------------------------------------------------------
//	@function:
//		CKeyCollection::FKey
//
//	@doc:
//		Check if array constitutes key
//
//---------------------------------------------------------------------------
BOOL
CKeyCollection::FKey(CMemoryPool *mp, const CColRefArray *colref_array) const
{
	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp);
	pcrs->Include(colref_array);

	BOOL fKey = FKey(pcrs.get());
	;

	return fKey;
}


//---------------------------------------------------------------------------
//	@function:
//		CKeyCollection::PdrgpcrTrim
//
//	@doc:
//		Return first subsumed key as column array
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefArray>
CKeyCollection::PdrgpcrTrim(CMemoryPool *mp,
							const CColRefArray *colref_array) const
{
	gpos::Ref<CColRefArray> pdrgpcrTrim = nullptr;
	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp);
	pcrs->Include(colref_array);

	const ULONG ulSets = m_pdrgpcrs->Size();
	for (ULONG ul = 0; ul < ulSets; ul++)
	{
		CColRefSet *pcrsKey = (*m_pdrgpcrs)[ul].get();
		if (pcrs->ContainsAll(pcrsKey))
		{
			pdrgpcrTrim = pcrsKey->Pdrgpcr(mp);
			break;
		}
	};

	return pdrgpcrTrim;
}

//---------------------------------------------------------------------------
//	@function:
//		CKeyCollection::PdrgpcrKey
//
//	@doc:
//		Extract a key
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefArray>
CKeyCollection::PdrgpcrKey(CMemoryPool *mp) const
{
	if (0 == m_pdrgpcrs->Size())
	{
		return nullptr;
	}

	GPOS_ASSERT(nullptr != (*m_pdrgpcrs)[0]);

	gpos::Ref<CColRefArray> colref_array = (*m_pdrgpcrs)[0]->Pdrgpcr(mp);
	return colref_array;
}


//---------------------------------------------------------------------------
//	@function:
//		CKeyCollection::PdrgpcrHashableKey
//
//	@doc:
//		Extract a hashable key
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefArray>
CKeyCollection::PdrgpcrHashableKey(CMemoryPool *mp) const
{
	const ULONG ulSets = m_pdrgpcrs->Size();
	for (ULONG ul = 0; ul < ulSets; ul++)
	{
		gpos::Ref<CColRefArray> pdrgpcrKey = (*m_pdrgpcrs)[ul]->Pdrgpcr(mp);
		if (CUtils::IsHashable(pdrgpcrKey.get()))
		{
			return pdrgpcrKey;
		};
	}

	// no hashable key is found
	return nullptr;
}


//---------------------------------------------------------------------------
//	@function:
//		CKeyCollection::PdrgpcrKey
//
//	@doc:
//		Extract the key at a position
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefArray>
CKeyCollection::PdrgpcrKey(CMemoryPool *mp, ULONG ulIndex) const
{
	if (0 == m_pdrgpcrs->Size())
	{
		return nullptr;
	}

	GPOS_ASSERT(nullptr != (*m_pdrgpcrs)[ulIndex]);

	gpos::Ref<CColRefArray> colref_array = (*m_pdrgpcrs)[ulIndex]->Pdrgpcr(mp);
	return colref_array;
}


//---------------------------------------------------------------------------
//	@function:
//		CKeyCollection::PcrsKey
//
//	@doc:
//		Extract key at given position
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CKeyCollection::PcrsKey(CMemoryPool *mp, ULONG ulIndex) const
{
	if (0 == m_pdrgpcrs->Size())
	{
		return nullptr;
	}

	GPOS_ASSERT(nullptr != (*m_pdrgpcrs)[ulIndex]);

	CColRefSet *pcrsKey = (*m_pdrgpcrs)[ulIndex].get();
	return GPOS_NEW(mp) CColRefSet(mp, *pcrsKey);
}


//---------------------------------------------------------------------------
//	@function:
//		CKeyCollection::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CKeyCollection::OsPrint(IOstream &os) const
{
	os << " Keys: (";

	const ULONG ulSets = m_pdrgpcrs->Size();
	for (ULONG ul = 0; ul < ulSets; ul++)
	{
		if (0 < ul)
		{
			os << ", ";
		}

		GPOS_ASSERT(nullptr != (*m_pdrgpcrs)[ul]);
		os << "[" << (*(*m_pdrgpcrs)[ul]) << "]";
	}

	return os << ")";
}


// EOF
