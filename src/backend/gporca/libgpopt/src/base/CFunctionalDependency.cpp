//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 Greenplum, Inc.
//
//	@filename:
//		CFunctionalDependency.cpp
//
//	@doc:
//		Implementation of functional dependency
//---------------------------------------------------------------------------

#include "gpopt/base/CFunctionalDependency.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CUtils.h"


using namespace gpopt;

FORCE_GENERATE_DBGSTR(CFunctionalDependency);

//---------------------------------------------------------------------------
//	@function:
//		CFunctionalDependency::CFunctionalDependency
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CFunctionalDependency::CFunctionalDependency(
	gpos::Ref<CColRefSet> pcrsKey, gpos::Ref<CColRefSet> pcrsDetermined)
	: m_pcrsKey(std::move(pcrsKey)), m_pcrsDetermined(std::move(pcrsDetermined))
{
	GPOS_ASSERT(0 < m_pcrsKey->Size());
	GPOS_ASSERT(0 < m_pcrsDetermined->Size());
}


//---------------------------------------------------------------------------
//	@function:
//		CFunctionalDependency::~CFunctionalDependency
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CFunctionalDependency::~CFunctionalDependency()
{
	;
	;
}


//---------------------------------------------------------------------------
//	@function:
//		CFunctionalDependency::FIncluded
//
//	@doc:
//		Determine if all FD columns are included in the given column set
//
//---------------------------------------------------------------------------
BOOL
CFunctionalDependency::FIncluded(CColRefSet *pcrs) const
{
	return pcrs->ContainsAll(m_pcrsKey.get()) &&
		   pcrs->ContainsAll(m_pcrsDetermined.get());
}


//---------------------------------------------------------------------------
//	@function:
//		CFunctionalDependency::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CFunctionalDependency::HashValue() const
{
	return gpos::CombineHashes(m_pcrsKey->HashValue(),
							   m_pcrsDetermined->HashValue());
}


//---------------------------------------------------------------------------
//	@function:
//		CFunctionalDependency::Equals
//
//	@doc:
//		Equality function
//
//---------------------------------------------------------------------------
BOOL
CFunctionalDependency::Equals(const CFunctionalDependency *pfd) const
{
	if (nullptr == pfd)
	{
		return false;
	}

	return m_pcrsKey->Equals(pfd->PcrsKey()) &&
		   m_pcrsDetermined->Equals(pfd->PcrsDetermined());
}


//---------------------------------------------------------------------------
//	@function:
//		CFunctionalDependency::OsPrint
//
//	@doc:
//		Print function
//
//---------------------------------------------------------------------------
IOstream &
CFunctionalDependency::OsPrint(IOstream &os) const
{
	os << "(" << *m_pcrsKey << ")";
	os << " --> (" << *m_pcrsDetermined << ")";
	return os;
}


//---------------------------------------------------------------------------
//	@function:
//		CFunctionalDependency::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CFunctionalDependency::HashValue(const CFunctionalDependencyArray *pdrgpfd)
{
	ULONG ulHash = 0;
	if (nullptr != pdrgpfd)
	{
		const ULONG size = pdrgpfd->Size();
		for (ULONG ul = 0; ul < size; ul++)
		{
			ulHash = gpos::CombineHashes(ulHash, (*pdrgpfd)[ul]->HashValue());
		}
	}

	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		CFunctionalDependency::Equals
//
//	@doc:
//		Equality function
//
//---------------------------------------------------------------------------
BOOL
CFunctionalDependency::Equals(const CFunctionalDependencyArray *pdrgpfdFst,
							  const CFunctionalDependencyArray *pdrgpfdSnd)
{
	if (nullptr == pdrgpfdFst && nullptr == pdrgpfdSnd)
		return true; /* both empty */

	if (nullptr == pdrgpfdFst || nullptr == pdrgpfdSnd)
		return false; /* one is empty, the other is not */

	const ULONG ulLenFst = pdrgpfdFst->Size();
	const ULONG ulLenSnd = pdrgpfdSnd->Size();

	if (ulLenFst != ulLenSnd)
		return false;

	BOOL fEqual = true;
	for (ULONG ulFst = 0; fEqual && ulFst < ulLenFst; ulFst++)
	{
		const CFunctionalDependency *pfdFst = (*pdrgpfdFst)[ulFst].get();
		BOOL fMatch = false;
		for (ULONG ulSnd = 0; !fMatch && ulSnd < ulLenSnd; ulSnd++)
		{
			const CFunctionalDependency *pfdSnd = (*pdrgpfdSnd)[ulSnd].get();
			fMatch = pfdFst->Equals(pfdSnd);
		}
		fEqual = fMatch;
	}

	return fEqual;
}


//---------------------------------------------------------------------------
//	@function:
//		CFunctionalDependency::PcrsKeys
//
//	@doc:
//		Create a set of all keys
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CFunctionalDependency::PcrsKeys(CMemoryPool *mp,
								const CFunctionalDependencyArray *pdrgpfd)
{
	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp);

	if (pdrgpfd != nullptr)
	{
		const ULONG size = pdrgpfd->Size();
		for (ULONG ul = 0; ul < size; ul++)
		{
			pcrs->Include((*pdrgpfd)[ul]->PcrsKey());
		}
	}

	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		CFunctionalDependency::PdrgpcrKeys
//
//	@doc:
//		Create an array of all keys
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefArray>
CFunctionalDependency::PdrgpcrKeys(CMemoryPool *mp,
								   const CFunctionalDependencyArray *pdrgpfd)
{
	gpos::Ref<CColRefSet> pcrs = PcrsKeys(mp, pdrgpfd);
	gpos::Ref<CColRefArray> colref_array = pcrs->Pdrgpcr(mp);
	;

	return colref_array;
}


// EOF
