//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CLogicalProject.cpp
//
//	@doc:
//		Implementation of project operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalProject.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/CDefaultComparator.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarProjectElement.h"

using namespace gpopt;
using namespace gpnaucrates;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::CLogicalProject
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalProject::CLogicalProject(CMemoryPool *mp) : CLogicalUnary(mp)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::DeriveOutputColumns
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogicalProject::DeriveOutputColumns(CMemoryPool *mp,
									 CExpressionHandle &exprhdl)
{
	GPOS_ASSERT(2 == exprhdl.Arity());

	gpos::owner<CColRefSet *> pcrs = GPOS_NEW(mp) CColRefSet(mp);

	// the scalar child defines additional columns
	pcrs->Union(exprhdl.DeriveOutputColumns(0));
	pcrs->Union(exprhdl.DeriveDefinedColumns(1));

	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
gpos::owner<CKeyCollection *>
CLogicalProject::DeriveKeyCollection(CMemoryPool *,	 // mp
									 CExpressionHandle &exprhdl) const
{
	return PkcDeriveKeysPassThru(exprhdl, 0 /* ulChild */);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::PdrgpcrsEquivClassFromScIdent
//
//	@doc:
//		Return equivalence class from scalar ident project element
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSetArray *>
CLogicalProject::PdrgpcrsEquivClassFromScIdent(
	CMemoryPool *mp, gpos::pointer<CExpression *> pexprPrEl,
	gpos::pointer<CColRefSet *> not_null_columns)
{
	GPOS_ASSERT(nullptr != pexprPrEl);

	gpos::pointer<CScalarProjectElement *> popPrEl =
		gpos::dyn_cast<CScalarProjectElement>(pexprPrEl->Pop());
	CColRef *pcrPrEl = popPrEl->Pcr();
	gpos::pointer<CExpression *> pexprScalar = (*pexprPrEl)[0];


	if (EopScalarIdent != pexprScalar->Pop()->Eopid())
	{
		return nullptr;
	}

	gpos::pointer<CScalarIdent *> popScIdent =
		gpos::dyn_cast<CScalarIdent>(pexprScalar->Pop());
	const CColRef *pcrScIdent = popScIdent->Pcr();
	GPOS_ASSERT(pcrPrEl->Id() != pcrScIdent->Id());
	GPOS_ASSERT(pcrPrEl->RetrieveType()->MDId()->Equals(
		pcrScIdent->RetrieveType()->MDId()));

	if (!CUtils::FConstrainableType(pcrPrEl->RetrieveType()->MDId()))
	{
		return nullptr;
	}

	BOOL non_nullable = not_null_columns->FMember(pcrScIdent);

	// only add renamed columns to equivalent class if the column is not null-able
	// this is because equality predicates will be inferred from the equivalent class
	// during preprocessing
	if (CColRef::EcrtTable == pcrScIdent->Ecrt() && non_nullable)
	{
		// equivalence class
		gpos::owner<CColRefSetArray *> pdrgpcrs =
			GPOS_NEW(mp) CColRefSetArray(mp);

		gpos::owner<CColRefSet *> pcrs = GPOS_NEW(mp) CColRefSet(mp);
		pcrs->Include(pcrPrEl);
		pcrs->Include(pcrScIdent);
		pdrgpcrs->Append(std::move(pcrs));

		return pdrgpcrs;
	}

	return nullptr;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::ExtractConstraintFromScConst
//
//	@doc:
//		Extract constraint from scalar constant project element
//
//---------------------------------------------------------------------------
void
CLogicalProject::ExtractConstraintFromScConst(
	CMemoryPool *mp, gpos::pointer<CExpression *> pexprPrEl,
	gpos::pointer<CConstraintArray *> pdrgpcnstr,  // array of range constraints
	gpos::pointer<CColRefSetArray *> pdrgpcrs	   // array of equivalence class
)
{
	GPOS_ASSERT(nullptr != pexprPrEl);
	GPOS_ASSERT(nullptr != pdrgpcnstr);
	GPOS_ASSERT(nullptr != pdrgpcrs);

	gpos::pointer<CScalarProjectElement *> popPrEl =
		gpos::dyn_cast<CScalarProjectElement>(pexprPrEl->Pop());
	CColRef *colref = popPrEl->Pcr();
	gpos::pointer<CExpression *> pexprScalar = (*pexprPrEl)[0];

	gpos::pointer<IMDId *> mdid_type = colref->RetrieveType()->MDId();

	if (EopScalarConst != pexprScalar->Pop()->Eopid() ||
		!CUtils::FConstrainableType(mdid_type))
	{
		return;
	}

	gpos::pointer<CScalarConst *> popConst =
		gpos::dyn_cast<CScalarConst>(pexprScalar->Pop());
	IDatum *datum = popConst->GetDatum();

	gpos::owner<CRangeArray *> pdrgprng = GPOS_NEW(mp) CRangeArray(mp);
	BOOL is_null = datum->IsNull();
	if (!is_null)
	{
		datum->AddRef();
		pdrgprng->Append(GPOS_NEW(mp) CRange(COptCtxt::PoctxtFromTLS()->Pcomp(),
											 IMDType::EcmptEq, datum));
	}

	pdrgpcnstr->Append(GPOS_NEW(mp) CConstraintInterval(
		mp, colref, std::move(pdrgprng), is_null));

	gpos::owner<CColRefSet *> pcrs = GPOS_NEW(mp) CColRefSet(mp);
	pcrs->Include(colref);
	pdrgpcrs->Append(std::move(pcrs));
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::DerivePropertyConstraint
//
//	@doc:
//		Derive constraint property
//
//---------------------------------------------------------------------------
gpos::owner<CPropConstraint *>
CLogicalProject::DerivePropertyConstraint(CMemoryPool *mp,
										  CExpressionHandle &exprhdl) const
{
	gpos::pointer<CExpression *> pexprPrL = exprhdl.PexprScalarExactChild(1);

	if (nullptr == pexprPrL)
	{
		return PpcDeriveConstraintPassThru(exprhdl, 0 /*ulChild*/);
	}

	gpos::owner<CConstraintArray *> pdrgpcnstr =
		GPOS_NEW(mp) CConstraintArray(mp);
	gpos::owner<CColRefSetArray *> pdrgpcrs = GPOS_NEW(mp) CColRefSetArray(mp);

	const ULONG ulProjElems = pexprPrL->Arity();
	for (ULONG ul = 0; ul < ulProjElems; ul++)
	{
		gpos::pointer<CExpression *> pexprPrEl = (*pexprPrL)[ul];
		gpos::pointer<CExpression *> pexprProjected = (*pexprPrEl)[0];

		if (EopScalarConst == pexprProjected->Pop()->Eopid())
		{
			ExtractConstraintFromScConst(mp, pexprPrEl, pdrgpcnstr, pdrgpcrs);
		}
		else
		{
			gpos::pointer<CColRefSet *> not_null_columns =
				exprhdl.DeriveNotNullColumns(0 /*ulChild*/);
			gpos::owner<CColRefSetArray *> pdrgpcrsChild =
				PdrgpcrsEquivClassFromScIdent(mp, pexprPrEl, not_null_columns);

			if (nullptr != pdrgpcrsChild)
			{
				// merge with the equivalence classes we have so far
				gpos::owner<CColRefSetArray *> pdrgpcrsMerged =
					CUtils::PdrgpcrsMergeEquivClasses(mp, pdrgpcrs,
													  pdrgpcrsChild);

				// clean up
				pdrgpcrs->Release();
				pdrgpcrsChild->Release();

				pdrgpcrs = pdrgpcrsMerged;
			}
		}
	}

	if (0 == pdrgpcnstr->Size() && 0 == pdrgpcrs->Size())
	{
		// no constants or equivalence classes found, so just return the same constraint property of the child
		pdrgpcnstr->Release();
		pdrgpcrs->Release();
		return PpcDeriveConstraintPassThru(exprhdl, 0 /*ulChild*/);
	}

	gpos::pointer<CPropConstraint *> ppcChild =
		exprhdl.DerivePropertyConstraint(0 /* ulChild */);

	// equivalence classes coming from child
	gpos::pointer<CColRefSetArray *> pdrgpcrsChild =
		ppcChild->PdrgpcrsEquivClasses();
	if (nullptr != pdrgpcrsChild)
	{
		// merge with the equivalence classes we have so far
		gpos::owner<CColRefSetArray *> pdrgpcrsMerged =
			CUtils::PdrgpcrsMergeEquivClasses(mp, pdrgpcrs, pdrgpcrsChild);

		// clean up
		pdrgpcrs->Release();
		pdrgpcrs = pdrgpcrsMerged;
	}

	// constraint coming from child
	CConstraint *pcnstr = ppcChild->Pcnstr();
	if (nullptr != pcnstr)
	{
		pcnstr->AddRef();
		pdrgpcnstr->Append(pcnstr);
	}

	gpos::owner<CConstraint *> pcnstrNew =
		CConstraint::PcnstrConjunction(mp, std::move(pdrgpcnstr));

	return GPOS_NEW(mp)
		CPropConstraint(mp, std::move(pdrgpcrs), std::move(pcnstrNew));
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalProject::DeriveMaxCard(CMemoryPool *,  // mp
							   CExpressionHandle &exprhdl) const
{
	if (exprhdl.DeriveHasNonScalarFunction(1))
	{
		// unbounded by default
		return CMaxCard();
	}

	// pass on max card of first child
	return exprhdl.DeriveMaxCard(0);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::owner<CXformSet *>
CLogicalProject::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);

	(void) xform_set->ExchangeSet(CXform::ExfSimplifyProjectWithSubquery);
	(void) xform_set->ExchangeSet(CXform::ExfProject2Apply);
	(void) xform_set->ExchangeSet(CXform::ExfProject2ComputeScalar);
	(void) xform_set->ExchangeSet(CXform::ExfCollapseProject);

	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::owner<IStatistics *>
CLogicalProject::PstatsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl,
							  gpos::pointer<IStatisticsArray *>	 // stats_ctxt
) const
{
	gpos::owner<UlongToIDatumMap *> phmuldatum =
		GPOS_NEW(mp) UlongToIDatumMap(mp);

	// extract scalar constant expression that can be used for
	// statistics calculation
	gpos::pointer<CExpression *> pexprPrList =
		exprhdl.PexprScalarRepChild(1 /*child_index*/);
	const ULONG arity = pexprPrList->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::pointer<CExpression *> pexprPrElem = (*pexprPrList)[ul];
		GPOS_ASSERT(1 == pexprPrElem->Arity());
		CColRef *colref =
			gpos::dyn_cast<CScalarProjectElement>(pexprPrElem->Pop())->Pcr();

		gpos::pointer<CExpression *> pexprScalar = (*pexprPrElem)[0];
		gpos::pointer<COperator *> pop = pexprScalar->Pop();
		if (COperator::EopScalarConst == pop->Eopid())
		{
			IDatum *datum = gpos::dyn_cast<CScalarConst>(pop)->GetDatum();
			if (datum->StatsMappable())
			{
				datum->AddRef();
				BOOL fInserted GPOS_ASSERTS_ONLY =
					phmuldatum->Insert(GPOS_NEW(mp) ULONG(colref->Id()), datum);
				GPOS_ASSERT(fInserted);
			}
		}
	}

	gpos::owner<IStatistics *> stats =
		PstatsDeriveProject(mp, exprhdl, phmuldatum);

	// clean up
	phmuldatum->Release();

	return stats;
}


// EOF
