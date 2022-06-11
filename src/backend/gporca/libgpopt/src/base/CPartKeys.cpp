//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CPartKeys.cpp
//
//	@doc:
//		Implementation of partitioning keys
//---------------------------------------------------------------------------

#include "gpopt/base/CPartKeys.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"

using namespace gpopt;

FORCE_GENERATE_DBGSTR(CPartKeys);

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::CPartKeys
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPartKeys::CPartKeys(gpos::owner<CColRef2dArray *> pdrgpdrgpcr)
	: m_pdrgpdrgpcr(std::move(pdrgpdrgpcr))
{
	GPOS_ASSERT(nullptr != m_pdrgpdrgpcr);
	m_num_of_part_levels = m_pdrgpdrgpcr->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::~CPartKeys
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPartKeys::~CPartKeys()
{
	m_pdrgpdrgpcr->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::PcrKey
//
//	@doc:
//		Return key at a given level
//
//---------------------------------------------------------------------------
CColRef *
CPartKeys::PcrKey(ULONG ulLevel) const
{
	GPOS_ASSERT(ulLevel < m_num_of_part_levels);
	gpos::pointer<CColRefArray *> colref_array = (*m_pdrgpdrgpcr)[ulLevel];
	return (*colref_array)[0];
}

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::FOverlap
//
//	@doc:
//		Check whether the key columns overlap the given column
//
//---------------------------------------------------------------------------
BOOL
CPartKeys::FOverlap(gpos::pointer<CColRefSet *> pcrs) const
{
	for (ULONG ul = 0; ul < m_num_of_part_levels; ul++)
	{
		CColRef *colref = PcrKey(ul);
		if (pcrs->FMember(colref))
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::PpartkeysCopy
//
//	@doc:
//		Copy part key into the given memory pool
//
//---------------------------------------------------------------------------
gpos::owner<CPartKeys *>
CPartKeys::PpartkeysCopy(CMemoryPool *mp)
{
	gpos::owner<CColRef2dArray *> pdrgpdrgpcrCopy =
		GPOS_NEW(mp) CColRef2dArray(mp);

	const ULONG length = m_pdrgpdrgpcr->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::pointer<CColRefArray *> colref_array = (*m_pdrgpdrgpcr)[ul];
		gpos::owner<CColRefArray *> pdrgpcrCopy = GPOS_NEW(mp) CColRefArray(mp);
		const ULONG num_cols = colref_array->Size();
		for (ULONG ulCol = 0; ulCol < num_cols; ulCol++)
		{
			pdrgpcrCopy->Append((*colref_array)[ulCol]);
		}
		pdrgpdrgpcrCopy->Append(pdrgpcrCopy);
	}

	return GPOS_NEW(mp) CPartKeys(std::move(pdrgpdrgpcrCopy));
}

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::PdrgppartkeysCopy
//
//	@doc:
//		Copy array of part keys into given memory pool
//
//---------------------------------------------------------------------------
gpos::owner<CPartKeysArray *>
CPartKeys::PdrgppartkeysCopy(
	CMemoryPool *mp, gpos::pointer<const CPartKeysArray *> pdrgppartkeys)
{
	GPOS_ASSERT(nullptr != pdrgppartkeys);

	gpos::owner<CPartKeysArray *> pdrgppartkeysCopy =
		GPOS_NEW(mp) CPartKeysArray(mp);
	const ULONG length = pdrgppartkeys->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		pdrgppartkeysCopy->Append((*pdrgppartkeys)[ul]->PpartkeysCopy(mp));
	}
	return pdrgppartkeysCopy;
}


//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::PpartkeysRemap
//
//	@doc:
//		Create a new PartKeys object from the current one by remapping the
//		keys using the given hashmap
//
//---------------------------------------------------------------------------
gpos::owner<CPartKeys *>
CPartKeys::PpartkeysRemap(CMemoryPool *mp,
						  UlongToColRefMap *colref_mapping) const
{
	GPOS_ASSERT(nullptr != colref_mapping);
	gpos::owner<CColRef2dArray *> pdrgpdrgpcr = GPOS_NEW(mp) CColRef2dArray(mp);

	for (ULONG ul = 0; ul < m_num_of_part_levels; ul++)
	{
		CColRef *colref =
			CUtils::PcrRemap(PcrKey(ul), colref_mapping, false /*must_exist*/);

		gpos::owner<CColRefArray *> colref_array =
			GPOS_NEW(mp) CColRefArray(mp);
		colref_array->Append(colref);

		pdrgpdrgpcr->Append(colref_array);
	}

	return GPOS_NEW(mp) CPartKeys(std::move(pdrgpdrgpcr));
}

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CPartKeys::OsPrint(IOstream &os) const
{
	os << "(";
	for (ULONG ul = 0; ul < m_num_of_part_levels; ul++)
	{
		CColRef *colref = PcrKey(ul);
		os << *colref;

		// separator
		os << (ul == m_num_of_part_levels - 1 ? "" : ", ");
	}

	os << ")";

	return os;
}

// EOF
