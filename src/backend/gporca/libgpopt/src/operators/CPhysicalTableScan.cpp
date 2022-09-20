//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CPhysicalTableScan.cpp
//
//	@doc:
//		Implementation of basic table scan operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalTableScan.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpec.h"
#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/base/CDistributionSpecRandom.h"
#include "gpopt/base/CDistributionSpecSingleton.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/metadata/CTableDescriptor.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalTableScan::CPhysicalTableScan
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CPhysicalTableScan::CPhysicalTableScan(CMemoryPool *mp, const CName *pnameAlias,
									   gpos::Ref<CTableDescriptor> ptabdesc,
									   gpos::Ref<CColRefArray> pdrgpcrOutput)
	: CPhysicalScan(mp, pnameAlias, std::move(ptabdesc),
					std::move(pdrgpcrOutput))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalTableScan::HashValue
//
//	@doc:
//		Combine pointer for table descriptor and Eop
//
//---------------------------------------------------------------------------
ULONG
CPhysicalTableScan::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(),
									   m_ptabdesc->MDId()->HashValue());
	ulHash = gpos::CombineHashes(ulHash,
								 CUtils::UlHashColArray(m_pdrgpcrOutput.get()));

	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalTableScan::Matches
//
//	@doc:
//		match operator
//
//---------------------------------------------------------------------------
BOOL
CPhysicalTableScan::Matches(COperator *pop) const
{
	if (Eopid() != pop->Eopid())
	{
		return false;
	}

	CPhysicalTableScan *popTableScan = gpos::dyn_cast<CPhysicalTableScan>(pop);
	return m_ptabdesc->MDId()->Equals(popTableScan->Ptabdesc()->MDId()) &&
		   m_pdrgpcrOutput->Equals(popTableScan->PdrgpcrOutput());
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalTableScan::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CPhysicalTableScan::OsPrint(IOstream &os) const
{
	os << SzId() << " ";

	// alias of table as referenced in the query
	m_pnameAlias->OsPrint(os);

	// actual name of table in catalog and columns
	os << " (";
	m_ptabdesc->Name().OsPrint(os);
	os << ")";

	return os;
}



// EOF
