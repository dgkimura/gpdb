//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 - 2012 EMC Corp.
//
//	@filename:
//		CLogicalSetOp.cpp
//
//	@doc:
//		Implementation of setops
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalSetOp.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/CConstraintNegation.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::CLogicalSetOp
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalSetOp::CLogicalSetOp(CMemoryPool *mp)
	: CLogical(mp),
	  m_pdrgpcrOutput(nullptr),
	  m_pdrgpdrgpcrInput(nullptr),
	  m_pcrsOutput(nullptr),
	  m_pdrgpcrsInput(nullptr)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::CLogicalSetOp
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalSetOp::CLogicalSetOp(CMemoryPool *mp,
							 gpos::Ref<CColRefArray> pdrgpcrOutput,
							 gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput)
	: CLogical(mp),
	  m_pdrgpcrOutput(std::move(pdrgpcrOutput)),
	  m_pdrgpdrgpcrInput(std::move(pdrgpdrgpcrInput)),
	  m_pcrsOutput(nullptr),
	  m_pdrgpcrsInput(nullptr)
{
	GPOS_ASSERT(nullptr != m_pdrgpcrOutput);
	GPOS_ASSERT(nullptr != m_pdrgpdrgpcrInput);

	BuildColumnSets(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::CLogicalSetOp
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalSetOp::CLogicalSetOp(CMemoryPool *mp,
							 gpos::Ref<CColRefArray> pdrgpcrOutput,
							 gpos::Ref<CColRefArray> pdrgpcrLeft,
							 gpos::Ref<CColRefArray> pdrgpcrRight)
	: CLogical(mp),
	  m_pdrgpcrOutput(std::move(pdrgpcrOutput)),
	  m_pdrgpdrgpcrInput(nullptr)
{
	GPOS_ASSERT(nullptr != m_pdrgpcrOutput);
	GPOS_ASSERT(nullptr != pdrgpcrLeft);
	GPOS_ASSERT(nullptr != pdrgpcrRight);

	m_pdrgpdrgpcrInput = GPOS_NEW(mp) CColRef2dArray(mp, 2);

	m_pdrgpdrgpcrInput->Append(std::move(pdrgpcrLeft));
	m_pdrgpdrgpcrInput->Append(std::move(pdrgpcrRight));

	BuildColumnSets(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::~CLogicalSetOp
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalSetOp::~CLogicalSetOp()
{
	;
	;
	;
	;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::BuildColumnSets
//
//	@doc:
//		Build set representation of input/output columns for faster
//		set operations
//
//---------------------------------------------------------------------------
void
CLogicalSetOp::BuildColumnSets(CMemoryPool *mp)
{
	GPOS_ASSERT(nullptr != m_pdrgpcrOutput);
	GPOS_ASSERT(nullptr != m_pdrgpdrgpcrInput);
	GPOS_ASSERT(nullptr == m_pcrsOutput);
	GPOS_ASSERT(nullptr == m_pdrgpcrsInput);

	m_pcrsOutput = GPOS_NEW(mp) CColRefSet(mp, m_pdrgpcrOutput.get());
	m_pdrgpcrsInput = GPOS_NEW(mp) CColRefSetArray(mp);
	const ULONG ulChildren = m_pdrgpdrgpcrInput->Size();
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		gpos::Ref<CColRefSet> pcrsInput =
			GPOS_NEW(mp) CColRefSet(mp, (*m_pdrgpdrgpcrInput)[ul].get());
		m_pdrgpcrsInput->Append(pcrsInput);

		m_pcrsLocalUsed->Include(pcrsInput.get());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::DeriveOutputColumns
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CLogicalSetOp::DeriveOutputColumns(CMemoryPool *,  // mp
								   CExpressionHandle &
#ifdef GPOS_DEBUG
									   exprhdl
#endif	// GPOS_DEBUG
)
{
#ifdef GPOS_DEBUG
	const ULONG arity = exprhdl.Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		CColRefSet *pcrsChildOutput = exprhdl.DeriveOutputColumns(ul);
		CColRefSet *pcrsInput = (*m_pdrgpcrsInput)[ul].get();
		GPOS_ASSERT(pcrsChildOutput->ContainsAll(pcrsInput) &&
					"Unexpected outer references in SetOp input");
	}
#endif	// GPOS_DEBUG

	;

	return m_pcrsOutput;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
gpos::Ref<CKeyCollection>
CLogicalSetOp::DeriveKeyCollection(CMemoryPool *mp,
								   CExpressionHandle &	// exprhdl
) const
{
	// TODO: 3/3/2012 - ; we can do better by remapping the keys between
	// all children and check if they align

	// True set ops return sets, hence, all output columns are keys
	;
	return GPOS_NEW(mp) CKeyCollection(mp, m_pdrgpcrOutput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::DerivePartitionInfo
//
//	@doc:
//		Derive partition consumer info
//
//---------------------------------------------------------------------------
gpos::Ref<CPartInfo>
CLogicalSetOp::DerivePartitionInfo(CMemoryPool *mp,
								   CExpressionHandle &exprhdl) const
{
	const ULONG arity = exprhdl.Arity();
	GPOS_ASSERT(0 < arity);

	// start with the part info of the first child
	gpos::Ref<CPartInfo> ppartinfo = exprhdl.DerivePartitionInfo(0);
	;

	for (ULONG ul = 1; ul < arity; ul++)
	{
		CPartInfo *ppartinfoChild = exprhdl.DerivePartitionInfo(ul);
		GPOS_ASSERT(nullptr != ppartinfoChild);

		CColRefArray *pdrgpcrInput = (*m_pdrgpdrgpcrInput)[ul].get();
		GPOS_ASSERT(pdrgpcrInput->Size() == m_pdrgpcrOutput->Size());

		gpos::Ref<CPartInfo> ppartinfoRemapped =
			ppartinfoChild->PpartinfoWithRemappedKeys(mp, pdrgpcrInput,
													  m_pdrgpcrOutput.get());
		gpos::Ref<CPartInfo> ppartinfoCombined = CPartInfo::PpartinfoCombine(
			mp, ppartinfo.get(), ppartinfoRemapped.get());
		;

		;
		ppartinfo = ppartinfoCombined;
	}

	return ppartinfo;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalSetOp::Matches(COperator *pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	CLogicalSetOp *popSetOp = gpos::dyn_cast<CLogicalSetOp>(pop);
	CColRef2dArray *pdrgpdrgpcrInput = popSetOp->PdrgpdrgpcrInput();
	const ULONG arity = pdrgpdrgpcrInput->Size();

	if (arity != m_pdrgpdrgpcrInput->Size() ||
		!m_pdrgpcrOutput->Equals(popSetOp->PdrgpcrOutput()))
	{
		return false;
	}

	for (ULONG ul = 0; ul < arity; ul++)
	{
		if (!(*m_pdrgpdrgpcrInput)[ul]->Equals((*pdrgpdrgpcrInput)[ul].get()))
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::PdrgpcrsOutputEquivClasses
//
//	@doc:
//		Get output equivalence classes
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSetArray>
CLogicalSetOp::PdrgpcrsOutputEquivClasses(CMemoryPool *mp,
										  CExpressionHandle &exprhdl,
										  BOOL fIntersect) const
{
	const ULONG ulChildren = exprhdl.Arity();
	gpos::Ref<CColRefSetArray> pdrgpcrs =
		PdrgpcrsInputMapped(mp, exprhdl, 0 /*ulChild*/);

	for (ULONG ul = 1; ul < ulChildren; ul++)
	{
		gpos::Ref<CColRefSetArray> pdrgpcrsChild =
			PdrgpcrsInputMapped(mp, exprhdl, ul);
		gpos::Ref<CColRefSetArray> pdrgpcrsMerged = nullptr;

		if (fIntersect)
		{
			// merge with the equivalence classes we have so far
			pdrgpcrsMerged = CUtils::PdrgpcrsMergeEquivClasses(
				mp, pdrgpcrs, pdrgpcrsChild.get());
		}
		else
		{
			// in case of a union, an equivalence class must be coming from all
			// children to be part of the output
			pdrgpcrsMerged = CUtils::PdrgpcrsIntersectEquivClasses(
				mp, pdrgpcrs.get(), pdrgpcrsChild.get());
		};
		;
		pdrgpcrs = pdrgpcrsMerged;
	}

	return pdrgpcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::PdrgpcrsInputMapped
//
//	@doc:
//		Get equivalence classes from one input child, mapped to output columns
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSetArray>
CLogicalSetOp::PdrgpcrsInputMapped(CMemoryPool *mp, CExpressionHandle &exprhdl,
								   ULONG ulChild) const
{
	CColRefSetArray *pdrgpcrsInput =
		exprhdl.DerivePropertyConstraint(ulChild)->PdrgpcrsEquivClasses();
	const ULONG length = pdrgpcrsInput->Size();

	CColRefSet *pcrsChildInput = (*m_pdrgpcrsInput)[ulChild].get();
	gpos::Ref<CColRefSetArray> pdrgpcrs = GPOS_NEW(mp) CColRefSetArray(mp);
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp);
		pcrs->Include((*pdrgpcrsInput)[ul].get());
		pcrs->Intersection(pcrsChildInput);

		if (0 == pcrs->Size())
		{
			;
			continue;
		}

		// replace each input column with its corresponding output column
		pcrs->Replace((*m_pdrgpdrgpcrInput)[ulChild].get(),
					  m_pdrgpcrOutput.get());

		pdrgpcrs->Append(pcrs);
	}

	return pdrgpcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::PdrgpcnstrColumn
//
//	@doc:
//		Get constraints for a given output column from all children
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintArray>
CLogicalSetOp::PdrgpcnstrColumn(CMemoryPool *mp, CExpressionHandle &exprhdl,
								ULONG ulColIndex, ULONG ulStart) const
{
	gpos::Ref<CConstraintArray> pdrgpcnstr = GPOS_NEW(mp) CConstraintArray(mp);

	CColRef *colref = (*m_pdrgpcrOutput)[ulColIndex];
	if (!CUtils::FConstrainableType(colref->RetrieveType()->MDId()))
	{
		return pdrgpcnstr;
	}

	const ULONG ulChildren = exprhdl.Arity();
	for (ULONG ul = ulStart; ul < ulChildren; ul++)
	{
		gpos::Ref<CConstraint> pcnstr =
			PcnstrColumn(mp, exprhdl, ulColIndex, ul);
		if (nullptr == pcnstr)
		{
			pcnstr =
				CConstraintInterval::PciUnbounded(mp, colref, true /*is_null*/);
		}
		GPOS_ASSERT(nullptr != pcnstr);
		pdrgpcnstr->Append(pcnstr);
	}

	return pdrgpcnstr;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::PcnstrColumn
//
//	@doc:
//		Get constraint for a given output column from a given children
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CLogicalSetOp::PcnstrColumn(CMemoryPool *mp, CExpressionHandle &exprhdl,
							ULONG ulColIndex, ULONG ulChild) const
{
	GPOS_ASSERT(ulChild < exprhdl.Arity());

	// constraint from child
	CConstraint *pcnstrChild =
		exprhdl.DerivePropertyConstraint(ulChild)->Pcnstr();
	if (nullptr == pcnstrChild)
	{
		return nullptr;
	}

	// part of constraint on the current input column
	gpos::Ref<CConstraint> pcnstrCol =
		pcnstrChild->Pcnstr(mp, (*(*m_pdrgpdrgpcrInput)[ulChild])[ulColIndex]);
	if (nullptr == pcnstrCol)
	{
		return nullptr;
	}

	// make a copy of this constraint but for the output column instead
	gpos::Ref<CConstraint> pcnstrOutput =
		pcnstrCol->PcnstrRemapForColumn(mp, (*m_pdrgpcrOutput)[ulColIndex]);
	;
	return pcnstrOutput;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::PpcDeriveConstraintSetop
//
//	@doc:
//		Derive constraint property for difference, intersect, and union
//		operators
//
//---------------------------------------------------------------------------
gpos::Ref<CPropConstraint>
CLogicalSetOp::PpcDeriveConstraintSetop(CMemoryPool *mp,
										CExpressionHandle &exprhdl,
										BOOL fIntersect) const
{
	const ULONG num_cols = m_pdrgpcrOutput->Size();

	gpos::Ref<CConstraintArray> pdrgpcnstr = GPOS_NEW(mp) CConstraintArray(mp);
	for (ULONG ul = 0; ul < num_cols; ul++)
	{
		// get constraints for this column from all children
		gpos::Ref<CConstraintArray> pdrgpcnstrCol =
			PdrgpcnstrColumn(mp, exprhdl, ul, 0 /*ulStart*/);

		gpos::Ref<CConstraint> pcnstrCol = nullptr;
		if (fIntersect)
		{
			pcnstrCol = CConstraint::PcnstrConjunction(mp, pdrgpcnstrCol);
		}
		else
		{
			pcnstrCol = CConstraint::PcnstrDisjunction(mp, pdrgpcnstrCol);
		}

		if (nullptr != pcnstrCol)
		{
			pdrgpcnstr->Append(pcnstrCol);
		}
	}

	gpos::Ref<CConstraint> pcnstrAll =
		CConstraint::PcnstrConjunction(mp, std::move(pdrgpcnstr));

	gpos::Ref<CColRefSetArray> pdrgpcrs =
		PdrgpcrsOutputEquivClasses(mp, exprhdl, fIntersect);

	return GPOS_NEW(mp)
		CPropConstraint(mp, std::move(pdrgpcrs), std::move(pcnstrAll));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::PcrsStat
//
//	@doc:
//		Compute required stats columns
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CLogicalSetOp::PcrsStat(CMemoryPool *,		  // mp
						CExpressionHandle &,  //exprhdl,
						CColRefSet *,		  //pcrsInput
						ULONG child_index) const
{
	gpos::Ref<CColRefSet> pcrs = (*m_pdrgpcrsInput)[child_index];
	;

	return pcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSetOp::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalSetOp::OsPrint(IOstream &os) const
{
	os << SzId() << " Output: (";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrOutput.get());
	os << ")";

	os << ", Input: [";
	const ULONG ulChildren = m_pdrgpdrgpcrInput->Size();
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		os << "(";
		CUtils::OsPrintDrgPcr(os, (*m_pdrgpdrgpcrInput)[ul].get());
		os << ")";

		if (ul < ulChildren - 1)
		{
			os << ", ";
		}
	}
	os << "]";

	return os;
}

// EOF
