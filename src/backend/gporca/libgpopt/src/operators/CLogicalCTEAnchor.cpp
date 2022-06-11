//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalCTEAnchor.cpp
//
//	@doc:
//		Implementation of CTE anchor operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalCTEAnchor.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::CLogicalCTEAnchor
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalCTEAnchor::CLogicalCTEAnchor(CMemoryPool *mp) : CLogical(mp), m_id(0)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::CLogicalCTEAnchor
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalCTEAnchor::CLogicalCTEAnchor(CMemoryPool *mp, ULONG id)
	: CLogical(mp), m_id(id)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::DeriveOutputColumns
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogicalCTEAnchor::DeriveOutputColumns(CMemoryPool *,  // mp
									   CExpressionHandle &exprhdl)
{
	return PcrsDeriveOutputPassThru(exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
gpos::owner<CKeyCollection *>
CLogicalCTEAnchor::DeriveKeyCollection(CMemoryPool *,  // mp
									   CExpressionHandle &exprhdl) const
{
	return PkcDeriveKeysPassThru(exprhdl, 0 /* ulChild */);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::DerivePartitionInfo
//
//	@doc:
//		Derive part consumer
//
//---------------------------------------------------------------------------
gpos::owner<CPartInfo *>
CLogicalCTEAnchor::DerivePartitionInfo(CMemoryPool *mp,
									   CExpressionHandle &exprhdl) const
{
	gpos::pointer<CPartInfo *> ppartinfoChild = exprhdl.DerivePartitionInfo(0);
	GPOS_ASSERT(nullptr != ppartinfoChild);

	gpos::pointer<CExpression *> pexprProducer =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(nullptr != pexprProducer);
	gpos::pointer<CPartInfo *> ppartinfoCTEProducer =
		pexprProducer->DerivePartitionInfo();

	return CPartInfo::PpartinfoCombine(mp, ppartinfoChild,
									   ppartinfoCTEProducer);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalCTEAnchor::DeriveMaxCard(CMemoryPool *,	 // mp
								 CExpressionHandle &exprhdl) const
{
	// pass on max card of first child
	return exprhdl.DeriveMaxCard(0);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::Matches
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CLogicalCTEAnchor::Matches(gpos::pointer<COperator *> pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	gpos::pointer<CLogicalCTEAnchor *> popCTEAnchor =
		gpos::dyn_cast<CLogicalCTEAnchor>(pop);

	return m_id == popCTEAnchor->Id();
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalCTEAnchor::HashValue() const
{
	return gpos::CombineHashes(COperator::HashValue(), m_id);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::owner<CXformSet *>
CLogicalCTEAnchor::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfCTEAnchor2Sequence);
	(void) xform_set->ExchangeSet(CXform::ExfCTEAnchor2TrivialSelect);
	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalCTEAnchor::OsPrint(IOstream &os) const
{
	os << SzId() << " (";
	os << m_id;
	os << ")";

	return os;
}

// EOF
