//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CLogical.cpp
//
//	@doc:
//		Implementation of base class of logical operators
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogical.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CConstraintConjunction.h"
#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/CDrvdPropRelational.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CReqdPropRelational.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalBitmapTableGet.h"
#include "gpopt/operators/CLogicalDynamicBitmapTableGet.h"
#include "gpopt/operators/CLogicalDynamicGet.h"
#include "gpopt/operators/CLogicalGet.h"
#include "gpopt/operators/CLogicalNAryJoin.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "naucrates/md/IMDCheckConstraint.h"
#include "naucrates/md/IMDColumn.h"
#include "naucrates/md/IMDIndex.h"
#include "naucrates/statistics/CStatistics.h"
#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpnaucrates;
using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CLogical::CLogical
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogical::CLogical(CMemoryPool *mp) : COperator(mp)
{
	GPOS_ASSERT(nullptr != mp);
	m_pcrsLocalUsed = GPOS_NEW(mp) CColRefSet(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::~CLogical
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogical::~CLogical()
{
	m_pcrsLocalUsed->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PdrgpcrCreateMapping
//
//	@doc:
//		Create output column mapping given a list of column descriptors and
//		a pointer to the operator creating that column
//
//---------------------------------------------------------------------------
gpos::owner<CColRefArray *>
CLogical::PdrgpcrCreateMapping(
	CMemoryPool *mp, gpos::pointer<const CColumnDescriptorArray *> pdrgpcoldesc,
	ULONG ulOpSourceId, gpos::pointer<IMDId *> mdid_table)
{
	// get column factory from optimizer context object
	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();

	ULONG num_cols = pdrgpcoldesc->Size();

	gpos::owner<CColRefArray *> colref_array =
		GPOS_NEW(mp) CColRefArray(mp, num_cols);
	for (ULONG ul = 0; ul < num_cols; ul++)
	{
		gpos::pointer<CColumnDescriptor *> pcoldesc = (*pdrgpcoldesc)[ul];
		CName name(mp, pcoldesc->Name());
		CColRef *colref = col_factory->PcrCreate(
			pcoldesc, name, ulOpSourceId, false /* mark_as_used */, mdid_table);
		colref_array->Append(colref);
	}

	return colref_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PdrgpdrgpcrCreatePartCols
//
//	@doc:
//		Initialize array of partition columns from the array with their indexes
//
//---------------------------------------------------------------------------
gpos::owner<CColRef2dArray *>
CLogical::PdrgpdrgpcrCreatePartCols(
	CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<const ULongPtrArray *> pdrgpulPart)
{
	GPOS_ASSERT(nullptr != colref_array && "Output columns cannot be NULL");
	GPOS_ASSERT(nullptr != pdrgpulPart);

	gpos::owner<CColRef2dArray *> pdrgpdrgpcrPart =
		GPOS_NEW(mp) CColRef2dArray(mp);

	const ULONG ulPartCols = pdrgpulPart->Size();
	GPOS_ASSERT(0 < ulPartCols);

	for (ULONG ul = 0; ul < ulPartCols; ul++)
	{
		ULONG ulCol = *((*pdrgpulPart)[ul]);

		CColRef *colref = (*colref_array)[ulCol];
		gpos::owner<CColRefArray *> pdrgpcrCurr = GPOS_NEW(mp) CColRefArray(mp);
		// The partition columns are not explicity referenced in the query but we
		// still need to mark them as used since they are required during partition
		// elimination
		colref->MarkAsUsed();
		pdrgpcrCurr->Append(colref);
		pdrgpdrgpcrPart->Append(pdrgpcrCurr);
	}

	return pdrgpdrgpcrPart;
}


//		Compute an order spec based on an index

gpos::owner<COrderSpec *>
CLogical::PosFromIndex(CMemoryPool *mp,
					   gpos::pointer<const IMDIndex *> pmdindex,
					   gpos::pointer<CColRefArray *> colref_array,
					   gpos::pointer<const CTableDescriptor *> ptabdesc)
{
	// compute the order spec after getting the current position of the index key
	// from the table descriptor. Index keys are relative to the
	// relation. So consider a case where we had 20 columns in a table. We
	// create an index that covers col # 20 as one of its keys. Then we drop
	// columns 10 through 15. Now the index key still points to col #20 but the
	// column ref list in colref_array will only have 15 elements in it.
	//

	gpos::owner<COrderSpec *> pos = GPOS_NEW(mp) COrderSpec(mp);

	// GiST, GIN and BRIN indexes have no order, so return an empty order spec
	if (pmdindex->IndexType() == IMDIndex::EmdindGist ||
		pmdindex->IndexType() == IMDIndex::EmdindGin ||
		pmdindex->IndexType() == IMDIndex::EmdindBrin)
	{
		return pos;
	}

	const ULONG ulLenKeys = pmdindex->Keys();

	// get relation from the metadata accessor using metadata id
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	gpos::pointer<const IMDRelation *> pmdrel =
		md_accessor->RetrieveRel(ptabdesc->MDId());

	for (ULONG ul = 0; ul < ulLenKeys; ul++)
	{
		// This is the postion of the index key column relative to the relation
		const ULONG ulPosRel = pmdindex->KeyAt(ul);

		// get the column and it's attno from the relation
		gpos::pointer<const IMDColumn *> pmdcol = pmdrel->GetMdCol(ulPosRel);
		INT attno = pmdcol->AttrNum();

		// get the position of the index key column relative to the table descriptor
		const ULONG ulPosTabDesc = ptabdesc->GetAttributePosition(attno);
		CColRef *colref = (*colref_array)[ulPosTabDesc];

		gpos::owner<IMDId *> mdid =
			colref->RetrieveType()->GetMdidForCmpType(IMDType::EcmptL);
		mdid->AddRef();

		// TODO:  March 27th 2012; we hard-code NULL treatment
		// need to revisit
		pos->Append(mdid, colref, COrderSpec::EntLast);
	}

	return pos;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PcrsDeriveOutputPassThru
//
//	@doc:
//		Common case of output derivation for unary operators or operators
//		that pass through the schema of only the outer child
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogical::PcrsDeriveOutputPassThru(CExpressionHandle &exprhdl)
{
	// may have additional children that are ignored, e.g., scalar children
	GPOS_ASSERT(1 <= exprhdl.Arity());

	gpos::owner<CColRefSet *> pcrs = exprhdl.DeriveOutputColumns(0);
	pcrs->AddRef();

	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogical::PcrsDeriveNotNullPassThruOuter
//
//	@doc:
//		Common case of deriving not null columns by passing through
//		not null columns from the outer child
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogical::PcrsDeriveNotNullPassThruOuter(CExpressionHandle &exprhdl)
{
	// may have additional children that are ignored, e.g., scalar children
	GPOS_ASSERT(1 <= exprhdl.Arity());

	gpos::owner<CColRefSet *> pcrs = exprhdl.DeriveNotNullColumns(0);
	pcrs->AddRef();

	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogical::PcrsDeriveOutputCombineLogical
//
//	@doc:
//		Common case of output derivation by combining the schemas of all
//		children
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogical::PcrsDeriveOutputCombineLogical(CMemoryPool *mp,
										 CExpressionHandle &exprhdl)
{
	gpos::owner<CColRefSet *> pcrs = GPOS_NEW(mp) CColRefSet(mp);

	// union columns from the first N-1 children
	ULONG arity = exprhdl.Arity();
	for (ULONG ul = 0; ul < arity - 1; ul++)
	{
		gpos::pointer<CColRefSet *> pcrsChild = exprhdl.DeriveOutputColumns(ul);
		GPOS_ASSERT(pcrs->IsDisjoint(pcrsChild) &&
					"Input columns are not disjoint");

		pcrs->Union(pcrsChild);
	}

	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogical::PcrsDeriveNotNullCombineLogical
//
//	@doc:
//		Common case of combining not null columns from all logical
//		children
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogical::PcrsDeriveNotNullCombineLogical(CMemoryPool *mp,
										  CExpressionHandle &exprhdl)
{
	gpos::owner<CColRefSet *> pcrs = GPOS_NEW(mp) CColRefSet(mp);

	// union not nullable columns from the first N-1 children
	ULONG arity = exprhdl.Arity();
	for (ULONG ul = 0; ul < arity - 1; ul++)
	{
		gpos::pointer<CColRefSet *> pcrsChild =
			exprhdl.DeriveNotNullColumns(ul);
		GPOS_ASSERT(pcrs->IsDisjoint(pcrsChild) &&
					"Input columns are not disjoint");

		pcrs->Union(pcrsChild);
	}

	return pcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PpartinfoPassThruOuter
//
//	@doc:
//		Common case of common case of passing through partition consumer array
//
//---------------------------------------------------------------------------
gpos::owner<CPartInfo *>
CLogical::PpartinfoPassThruOuter(CExpressionHandle &exprhdl)
{
	gpos::pointer<CPartInfo *> ppartinfo = exprhdl.DerivePartitionInfo(0);
	GPOS_ASSERT(nullptr != ppartinfo);
	ppartinfo->AddRef();
	return ppartinfo;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogical::PkcCombineKeys
//
//	@doc:
//		Common case of combining keys from first n - 1 children
//
//---------------------------------------------------------------------------
gpos::owner<CKeyCollection *>
CLogical::PkcCombineKeys(CMemoryPool *mp, CExpressionHandle &exprhdl)
{
	gpos::owner<CColRefSet *> pcrs = GPOS_NEW(mp) CColRefSet(mp);
	const ULONG arity = exprhdl.Arity();
	for (ULONG ul = 0; ul < arity - 1; ul++)
	{
		gpos::pointer<CKeyCollection *> pkc = exprhdl.DeriveKeyCollection(ul);
		if (nullptr == pkc)
		{
			// if a child has no key, the operator has no key
			pcrs->Release();
			return nullptr;
		}

		gpos::owner<CColRefArray *> colref_array = pkc->PdrgpcrKey(mp);
		pcrs->Include(colref_array);
		colref_array->Release();
	}

	return GPOS_NEW(mp) CKeyCollection(mp, std::move(pcrs));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PkcKeysBaseTable
//
//	@doc:
//		Helper function for computing the keys in a base table
//
//---------------------------------------------------------------------------
gpos::owner<CKeyCollection *>
CLogical::PkcKeysBaseTable(CMemoryPool *mp,
						   gpos::pointer<const CBitSetArray *> pdrgpbsKeys,
						   gpos::pointer<const CColRefArray *> pdrgpcrOutput)
{
	const ULONG ulKeys = pdrgpbsKeys->Size();

	if (0 == ulKeys)
	{
		return nullptr;
	}

	gpos::owner<CKeyCollection *> pkc = GPOS_NEW(mp) CKeyCollection(mp);

	for (ULONG ul = 0; ul < ulKeys; ul++)
	{
		gpos::owner<CColRefSet *> pcrs = GPOS_NEW(mp) CColRefSet(mp);

		gpos::pointer<CBitSet *> pbs = (*pdrgpbsKeys)[ul];
		CBitSetIter bsiter(*pbs);

		while (bsiter.Advance())
		{
			pcrs->Include((*pdrgpcrOutput)[bsiter.Bit()]);
		}

		pkc->Add(pcrs);
	}

	return pkc;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PpartinfoDeriveCombine
//
//	@doc:
//		Common case of combining partition info of all logical children
//
//---------------------------------------------------------------------------
gpos::owner<CPartInfo *>
CLogical::PpartinfoDeriveCombine(CMemoryPool *mp, CExpressionHandle &exprhdl)
{
	const ULONG arity = exprhdl.Arity();
	GPOS_ASSERT(0 < arity);

	gpos::owner<CPartInfo *> ppartinfo = GPOS_NEW(mp) CPartInfo(mp);

	for (ULONG ul = 0; ul < arity; ul++)
	{
		CPartInfo *ppartinfoChild = nullptr;
		if (exprhdl.FScalarChild(ul))
		{
			ppartinfoChild = exprhdl.DeriveScalarPartitionInfo(ul);
		}
		else
		{
			ppartinfoChild = exprhdl.DerivePartitionInfo(ul);
		}
		GPOS_ASSERT(nullptr != ppartinfoChild);
		gpos::owner<CPartInfo *> ppartinfoCombined =
			CPartInfo::PpartinfoCombine(mp, ppartinfo, ppartinfoChild);
		ppartinfo->Release();
		ppartinfo = ppartinfoCombined;
	}

	return ppartinfo;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::DeriveOuterReferences
//
//	@doc:
//		Derive outer references
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogical::DeriveOuterReferences(CMemoryPool *mp, CExpressionHandle &exprhdl,
								gpos::pointer<CColRefSet *> pcrsUsedAdditional)
{
	ULONG arity = exprhdl.Arity();
	gpos::owner<CColRefSet *> outer_refs = GPOS_NEW(mp) CColRefSet(mp);

	// collect output columns from relational children
	// and used columns from scalar children
	gpos::owner<CColRefSet *> pcrsOutput = GPOS_NEW(mp) CColRefSet(mp);

	gpos::owner<CColRefSet *> pcrsUsed = GPOS_NEW(mp) CColRefSet(mp);
	for (ULONG i = 0; i < arity; i++)
	{
		if (exprhdl.FScalarChild(i))
		{
			pcrsUsed->Union(exprhdl.DeriveUsedColumns(i));
		}
		else
		{
			// add outer references from relational children
			outer_refs->Union(exprhdl.DeriveOuterReferences(i));
			pcrsOutput->Union(exprhdl.DeriveOutputColumns(i));
		}
	}

	if (nullptr != pcrsUsedAdditional)
	{
		pcrsUsed->Include(pcrsUsedAdditional);
	}

	// outer references are columns used by scalar child
	// but are not included in the output columns of relational children
	outer_refs->Union(pcrsUsed);
	outer_refs->Exclude(pcrsOutput);

	pcrsOutput->Release();
	pcrsUsed->Release();
	return outer_refs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PcrsDeriveOuterIndexGet
//
//	@doc:
//		Derive outer references for index get and dynamic index get operators
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogical::PcrsDeriveOuterIndexGet(CMemoryPool *mp, CExpressionHandle &exprhdl)
{
	ULONG arity = exprhdl.Arity();
	gpos::owner<CColRefSet *> outer_refs = GPOS_NEW(mp) CColRefSet(mp);

	gpos::owner<CColRefSet *> pcrsOutput = DeriveOutputColumns(mp, exprhdl);

	gpos::owner<CColRefSet *> pcrsUsed = GPOS_NEW(mp) CColRefSet(mp);
	for (ULONG i = 0; i < arity; i++)
	{
		GPOS_ASSERT(exprhdl.FScalarChild(i));
		pcrsUsed->Union(exprhdl.DeriveUsedColumns(i));
	}

	// outer references are columns used by scalar children
	// but are not included in the output columns of relational children
	outer_refs->Union(pcrsUsed);
	outer_refs->Exclude(pcrsOutput);

	pcrsOutput->Release();
	pcrsUsed->Release();
	return outer_refs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PcrsDeriveCorrelatedApply
//
//	@doc:
//		Derive columns from the inner child of a correlated-apply expression
//		that can be used above the apply expression
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogical::DeriveCorrelatedApplyColumns(CMemoryPool *mp,
									   CExpressionHandle &exprhdl) const
{
	GPOS_ASSERT(this == exprhdl.Pop());

	ULONG arity = exprhdl.Arity();
	gpos::owner<CColRefSet *> pcrs = GPOS_NEW(mp) CColRefSet(mp);

	if (CUtils::FCorrelatedApply(exprhdl.Pop()))
	{
		// add inner columns of correlated-apply expression
		pcrs->Include(
			gpos::dyn_cast<CLogicalApply>(exprhdl.Pop())->PdrgPcrInner());
	}

	// combine correlated-apply columns from logical children
	for (ULONG ul = 0; ul < arity; ul++)
	{
		if (!exprhdl.FScalarChild(ul))
		{
			gpos::pointer<CDrvdPropRelational *> pdprel =
				exprhdl.GetRelationalProperties(ul);
			pcrs->Union(pdprel->GetCorrelatedApplyColumns());
		}
	}

	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogical::PkcDeriveKeysPassThru
//
//	@doc:
//		Addref and return keys of n-th child
//
//---------------------------------------------------------------------------
CKeyCollection *
CLogical::PkcDeriveKeysPassThru(CExpressionHandle &exprhdl, ULONG ulChild)
{
	CKeyCollection *pkcLeft =
		exprhdl.GetRelationalProperties(ulChild)->GetKeyCollection();

	// key collection may be NULL
	if (nullptr != pkcLeft)
	{
		pkcLeft->AddRef();
	}

	return pkcLeft;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogical::PkcDeriveKeys
//
//	@doc:
//		Derive key collections
//
//---------------------------------------------------------------------------
gpos::owner<CKeyCollection *>
CLogical::DeriveKeyCollection(CMemoryPool *,	   // mp
							  CExpressionHandle &  // exprhdl
) const
{
	// no keys found by default
	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PpcDeriveConstraintFromPredicates
//
//	@doc:
//		Derive constraint property when expression has relational children and
//		scalar children (predicates)
//
//---------------------------------------------------------------------------
gpos::owner<CPropConstraint *>
CLogical::PpcDeriveConstraintFromPredicates(CMemoryPool *mp,
											CExpressionHandle &exprhdl)
{
	gpos::owner<CColRefSetArray *> pdrgpcrs = GPOS_NEW(mp) CColRefSetArray(mp);

	gpos::owner<CConstraintArray *> pdrgpcnstr =
		GPOS_NEW(mp) CConstraintArray(mp);

	// collect constraint properties from relational children
	// and predicates from scalar children
	const ULONG arity = exprhdl.Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		if (exprhdl.FScalarChild(ul))
		{
			CExpression *pexprScalar = exprhdl.PexprScalarExactChild(ul);

			// make sure it is a predicate... boolop, cmp, nulltest,
			// or a list of join predicates for an NAry join
			if (nullptr == pexprScalar || !CUtils::FPredicate(pexprScalar))
			{
				continue;
			}
			GPOS_ASSERT(COperator::EopScalarNAryJoinPredList !=
						pexprScalar->Pop()->Eopid());
			gpos::owner<CColRefSetArray *> pdrgpcrsChild = nullptr;
			gpos::owner<CConstraint *> pcnstr =
				CConstraint::PcnstrFromScalarExpr(mp, pexprScalar,
												  &pdrgpcrsChild);

			if (nullptr != pcnstr)
			{
				pdrgpcnstr->Append(pcnstr);

				// merge with the equivalence classes we have so far
				gpos::owner<CColRefSetArray *> pdrgpcrsMerged =
					CUtils::PdrgpcrsMergeEquivClasses(mp, pdrgpcrs,
													  pdrgpcrsChild);
				pdrgpcrs->Release();
				pdrgpcrs = pdrgpcrsMerged;
			}
			CRefCount::SafeRelease(pdrgpcrsChild);
		}
		else
		{
			gpos::pointer<CPropConstraint *> ppc =
				exprhdl.DerivePropertyConstraint(ul);

			// equivalence classes coming from child
			gpos::pointer<CColRefSetArray *> pdrgpcrsChild =
				ppc->PdrgpcrsEquivClasses();

			// merge with the equivalence classes we have so far
			gpos::owner<CColRefSetArray *> pdrgpcrsMerged =
				CUtils::PdrgpcrsMergeEquivClasses(mp, pdrgpcrs, pdrgpcrsChild);
			pdrgpcrs->Release();
			pdrgpcrs = pdrgpcrsMerged;

			// constraint coming from child
			CConstraint *pcnstr = ppc->Pcnstr();
			if (nullptr != pcnstr)
			{
				pcnstr->AddRef();
				pdrgpcnstr->Append(pcnstr);
			}
		}
	}

	gpos::owner<CConstraint *> pcnstrNew =
		CConstraint::PcnstrConjunction(mp, std::move(pdrgpcnstr));

	return GPOS_NEW(mp)
		CPropConstraint(mp, std::move(pdrgpcrs), std::move(pcnstrNew));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PpcDeriveConstraintFromTable
//
//	@doc:
//		Derive constraint property from a table/index get
//
//---------------------------------------------------------------------------
gpos::owner<CPropConstraint *>
CLogical::PpcDeriveConstraintFromTable(
	CMemoryPool *mp, gpos::pointer<const CTableDescriptor *> ptabdesc,
	gpos::pointer<const CColRefArray *> pdrgpcrOutput)
{
	gpos::owner<CColRefSetArray *> pdrgpcrs = GPOS_NEW(mp) CColRefSetArray(mp);

	gpos::owner<CConstraintArray *> pdrgpcnstr =
		GPOS_NEW(mp) CConstraintArray(mp);

	gpos::pointer<const CColumnDescriptorArray *> pdrgpcoldesc =
		ptabdesc->Pdrgpcoldesc();
	const ULONG num_cols = pdrgpcoldesc->Size();

	gpos::owner<CColRefArray *> pdrgpcrNonSystem =
		GPOS_NEW(mp) CColRefArray(mp);

	for (ULONG ul = 0; ul < num_cols; ul++)
	{
		gpos::pointer<CColumnDescriptor *> pcoldesc = (*pdrgpcoldesc)[ul];
		CColRef *colref = (*pdrgpcrOutput)[ul];
		// we are only interested in non-system columns that are defined as
		// being NOT NULL

		if (pcoldesc->IsSystemColumn())
		{
			continue;
		}

		pdrgpcrNonSystem->Append(colref);

		if (pcoldesc->IsNullable() || colref->GetUsage() == CColRef::EUnused)
		{
			continue;
		}

		// add a "not null" constraint and an equivalence class
		gpos::owner<CConstraint *> pcnstr = CConstraintInterval::PciUnbounded(
			mp, colref, false /*fIncludesNull*/);

		if (pcnstr == nullptr)
		{
			continue;
		}
		pdrgpcnstr->Append(pcnstr);

		gpos::owner<CColRefSet *> pcrsEquiv = GPOS_NEW(mp) CColRefSet(mp);
		pcrsEquiv->Include(colref);
		pdrgpcrs->Append(pcrsEquiv);
	}

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	gpos::pointer<const IMDRelation *> pmdrel =
		md_accessor->RetrieveRel(ptabdesc->MDId());

	const ULONG ulCheckConstraint = pmdrel->CheckConstraintCount();
	for (ULONG ul = 0; ul < ulCheckConstraint; ul++)
	{
		gpos::pointer<IMDId *> pmdidCheckConstraint =
			pmdrel->CheckConstraintMDidAt(ul);

		gpos::pointer<const IMDCheckConstraint *> pmdCheckConstraint =
			md_accessor->RetrieveCheckConstraints(pmdidCheckConstraint);

		// extract the check constraint expression
		gpos::owner<CExpression *> pexprCheckConstraint =
			pmdCheckConstraint->GetCheckConstraintExpr(mp, md_accessor,
													   pdrgpcrNonSystem);
		GPOS_ASSERT(nullptr != pexprCheckConstraint);
		GPOS_ASSERT(CUtils::FPredicate(pexprCheckConstraint));

		gpos::owner<CColRefSetArray *> pdrgpcrsChild = nullptr;

		// Check constraints are satisfied if the check expression evaluates to
		// true or NULL, so infer NULLs as true here.
		gpos::owner<CConstraint *> pcnstr = CConstraint::PcnstrFromScalarExpr(
			mp, pexprCheckConstraint, &pdrgpcrsChild,
			true /* infer_nulls_as */);
		if (nullptr != pcnstr)
		{
			pdrgpcnstr->Append(pcnstr);

			// merge with the equivalence classes we have so far
			gpos::owner<CColRefSetArray *> pdrgpcrsMerged =
				CUtils::PdrgpcrsMergeEquivClasses(mp, pdrgpcrs, pdrgpcrsChild);
			pdrgpcrs->Release();
			pdrgpcrs = pdrgpcrsMerged;
		}
		CRefCount::SafeRelease(pdrgpcrsChild);
		pexprCheckConstraint->Release();
	}

	pdrgpcrNonSystem->Release();

	return GPOS_NEW(mp) CPropConstraint(
		mp, std::move(pdrgpcrs),
		CConstraint::PcnstrConjunction(mp, std::move(pdrgpcnstr)));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PpcDeriveConstraintFromTableWithPredicates
//
//	@doc:
//		Derive constraint property from a table/index get with predicates
//
//---------------------------------------------------------------------------
gpos::owner<CPropConstraint *>
CLogical::PpcDeriveConstraintFromTableWithPredicates(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::pointer<const CTableDescriptor *> ptabdesc,
	gpos::pointer<const CColRefArray *> pdrgpcrOutput)
{
	gpos::owner<CConstraintArray *> pdrgpcnstr =
		GPOS_NEW(mp) CConstraintArray(mp);
	gpos::owner<CPropConstraint *> ppcTable =
		PpcDeriveConstraintFromTable(mp, ptabdesc, pdrgpcrOutput);
	CConstraint *pcnstrTable = ppcTable->Pcnstr();
	if (nullptr != pcnstrTable)
	{
		pcnstrTable->AddRef();
		pdrgpcnstr->Append(pcnstrTable);
	}
	CColRefSetArray *pdrgpcrsEquivClassesTable =
		ppcTable->PdrgpcrsEquivClasses();

	gpos::owner<CPropConstraint *> ppcnstrCond =
		PpcDeriveConstraintFromPredicates(mp, exprhdl);
	CConstraint *pcnstrCond = ppcnstrCond->Pcnstr();

	if (nullptr != pcnstrCond)
	{
		pcnstrCond->AddRef();
		pdrgpcnstr->Append(pcnstrCond);
	}
	else if (nullptr == pcnstrTable)
	{
		ppcTable->Release();
		pdrgpcnstr->Release();

		return ppcnstrCond;
	}

	gpos::pointer<CColRefSetArray *> pdrgpcrsCond =
		ppcnstrCond->PdrgpcrsEquivClasses();
	gpos::owner<CColRefSetArray *> pdrgpcrs = CUtils::PdrgpcrsMergeEquivClasses(
		mp, pdrgpcrsEquivClassesTable, pdrgpcrsCond);
	gpos::owner<CPropConstraint *> ppc = GPOS_NEW(mp) CPropConstraint(
		mp, std::move(pdrgpcrs),
		CConstraint::PcnstrConjunction(mp, std::move(pdrgpcnstr)));

	ppcnstrCond->Release();
	ppcTable->Release();

	return ppc;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PpcDeriveConstraintPassThru
//
//	@doc:
//		Shorthand to addref and pass through constraint from a given child
//
//---------------------------------------------------------------------------
CPropConstraint *
CLogical::PpcDeriveConstraintPassThru(CExpressionHandle &exprhdl, ULONG ulChild)
{
	// return constraint property of child
	CPropConstraint *ppc = exprhdl.DerivePropertyConstraint(ulChild);
	if (nullptr != ppc)
	{
		ppc->AddRef();
	}
	return ppc;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PpcDeriveConstraintRestrict
//
//	@doc:
//		Derive constraint property only on the given columns
//
//---------------------------------------------------------------------------
gpos::owner<CPropConstraint *>
CLogical::PpcDeriveConstraintRestrict(CMemoryPool *mp,
									  CExpressionHandle &exprhdl,
									  gpos::pointer<CColRefSet *> pcrsOutput)
{
	// constraint property from relational child
	gpos::pointer<CPropConstraint *> ppc = exprhdl.DerivePropertyConstraint(0);
	gpos::pointer<CColRefSetArray *> pdrgpcrs = ppc->PdrgpcrsEquivClasses();

	// construct new array of equivalence classes
	gpos::owner<CColRefSetArray *> pdrgpcrsNew =
		GPOS_NEW(mp) CColRefSetArray(mp);

	const ULONG length = pdrgpcrs->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::owner<CColRefSet *> pcrsEquiv = GPOS_NEW(mp) CColRefSet(mp);
		pcrsEquiv->Include((*pdrgpcrs)[ul]);
		pcrsEquiv->Intersection(pcrsOutput);

		if (0 < pcrsEquiv->Size())
		{
			pdrgpcrsNew->Append(pcrsEquiv);
		}
		else
		{
			pcrsEquiv->Release();
		}
	}

	gpos::pointer<CConstraint *> pcnstrChild = ppc->Pcnstr();
	if (nullptr == pcnstrChild)
	{
		return GPOS_NEW(mp)
			CPropConstraint(mp, std::move(pdrgpcrsNew), nullptr);
	}

	gpos::owner<CConstraintArray *> pdrgpcnstr =
		GPOS_NEW(mp) CConstraintArray(mp);

	// include only constraints on given columns
	CColRefSetIter crsi(*pcrsOutput);
	while (crsi.Advance())
	{
		CColRef *colref = crsi.Pcr();
		gpos::owner<CConstraint *> pcnstrCol = pcnstrChild->Pcnstr(mp, colref);
		if (nullptr == pcnstrCol)
		{
			continue;
		}

		if (pcnstrCol->IsConstraintUnbounded())
		{
			pcnstrCol->Release();
			continue;
		}

		pdrgpcnstr->Append(pcnstrCol);
	}

	gpos::owner<CConstraint *> pcnstr =
		CConstraint::PcnstrConjunction(mp, std::move(pdrgpcnstr));

	return GPOS_NEW(mp)
		CPropConstraint(mp, std::move(pdrgpcrsNew), std::move(pcnstr));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::DeriveFunctionProperties
//
//	@doc:
//		Derive function properties
//
//---------------------------------------------------------------------------
gpos::owner<CFunctionProp *>
CLogical::DeriveFunctionProperties(CMemoryPool *mp,
								   CExpressionHandle &exprhdl) const
{
	IMDFunction::EFuncStbl efs =
		EfsDeriveFromChildren(exprhdl, IMDFunction::EfsImmutable);

	return GPOS_NEW(mp) CFunctionProp(
		efs, exprhdl.FChildrenHaveVolatileFuncScan(), false /*fScan*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::DeriveTableDescriptor
//
//	@doc:
//		Derive table descriptor for tables used by operator
//
//---------------------------------------------------------------------------
gpos::pointer<CTableDescriptor *>
CLogical::DeriveTableDescriptor(CMemoryPool *, CExpressionHandle &) const
{
	//currently return null unless there is a single table being used. Later we may want
	//to make this return a set of table descriptors and pass them up all operators
	return nullptr;
}
//---------------------------------------------------------------------------
//	@function:
//		CLogical::PfpDeriveFromScalar
//
//	@doc:
//		Derive function properties using data access property of scalar child
//
//---------------------------------------------------------------------------
gpos::owner<CFunctionProp *>
CLogical::PfpDeriveFromScalar(CMemoryPool *mp, CExpressionHandle &exprhdl)
{
	GPOS_CHECK_ABORT;

	// collect stability from all children
	IMDFunction::EFuncStbl efs =
		EfsDeriveFromChildren(exprhdl, IMDFunction::EfsImmutable);

	return GPOS_NEW(mp) CFunctionProp(
		efs, exprhdl.FChildrenHaveVolatileFuncScan(), false /*fScan*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogical::DeriveMaxCard(CMemoryPool *,		 // mp
						CExpressionHandle &	 // exprhdl
) const
{
	// unbounded by default
	return CMaxCard();
}


//---------------------------------------------------------------------------
//	@function:
//		CLogical::DeriveJoinDepth
//
//	@doc:
//		Derive join depth
//
//---------------------------------------------------------------------------
ULONG
CLogical::DeriveJoinDepth(CMemoryPool *,  // mp
						  CExpressionHandle &exprhdl) const
{
	const ULONG arity = exprhdl.Arity();

	// sum-up join depth of all relational children
	ULONG ulDepth = 0;
	for (ULONG ul = 0; ul < arity; ul++)
	{
		if (!exprhdl.FScalarChild(ul))
		{
			ulDepth = ulDepth + exprhdl.DeriveJoinDepth(ul);
		}
	}

	return ulDepth;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::MaxcardDef
//
//	@doc:
//		Default max card for join and apply operators
//
//---------------------------------------------------------------------------
CMaxCard
CLogical::MaxcardDef(CExpressionHandle &exprhdl)
{
	const ULONG arity = exprhdl.Arity();

	CMaxCard maxcard = exprhdl.DeriveMaxCard(0);
	for (ULONG ul = 1; ul < arity - 1; ul++)
	{
		if (!exprhdl.FScalarChild(ul))
		{
			maxcard *= exprhdl.DeriveMaxCard(ul);
		}
	}

	return maxcard;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::DeriveMaxCard
//
//	@doc:
//		Derive max card given scalar child and constraint property. If a
//		contradiction is detected then return maxcard of zero, otherwise
//		use the given default maxcard
//
//---------------------------------------------------------------------------
CMaxCard
CLogical::Maxcard(CExpressionHandle &exprhdl, ULONG ulScalarIndex,
				  CMaxCard maxcard)
{
	// in case of a false condition (when the operator is not Full / Left Outer Join) or a contradiction, maxcard should be zero
	gpos::pointer<CExpression *> pexprScalar =
		exprhdl.PexprScalarExactChild(ulScalarIndex);

	if (nullptr != pexprScalar)
	{
		if (COperator::EopScalarNAryJoinPredList == pexprScalar->Pop()->Eopid())
		{
			// look at the inner join predicates only
			pexprScalar = (*pexprScalar)[0];
		}

		if ((CUtils::FScalarConstFalse(pexprScalar) &&
			 COperator::EopLogicalFullOuterJoin != exprhdl.Pop()->Eopid() &&
			 COperator::EopLogicalLeftOuterJoin != exprhdl.Pop()->Eopid() &&
			 COperator::EopLogicalRightOuterJoin != exprhdl.Pop()->Eopid()) ||
			exprhdl.DerivePropertyConstraint()->FContradiction())
		{
			return CMaxCard(0 /*ull*/);
		}
	}

	return maxcard;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PcrsReqdChildStats
//
//	@doc:
//		Helper for compute required stat columns of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogical::PcrsReqdChildStats(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::pointer<CColRefSet *> pcrsInput,
	gpos::pointer<CColRefSet *> pcrsUsed,  // columns used by scalar child(ren)
	ULONG child_index)
{
	GPOS_ASSERT(child_index < exprhdl.Arity() - 1);
	GPOS_CHECK_ABORT;

	gpos::owner<CColRefSet *> pcrs = GPOS_NEW(mp) CColRefSet(mp);
	pcrs->Union(pcrsInput);
	pcrs->Union(pcrsUsed);

	// intersect with the output columns of relational child
	pcrs->Intersection(exprhdl.DeriveOutputColumns(child_index));

	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogical::PcrsStatsPassThru
//
//	@doc:
//		Helper for common case of passing through required stat columns
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogical::PcrsStatsPassThru(gpos::pointer<CColRefSet *> pcrsInput)
{
	GPOS_ASSERT(nullptr != pcrsInput);
	GPOS_CHECK_ABORT;

	pcrsInput->AddRef();
	return pcrsInput;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogical::PstatsPassThruOuter
//
//	@doc:
//		Helper for common case of passing through derived stats
//
//---------------------------------------------------------------------------
gpos::owner<IStatistics *>
CLogical::PstatsPassThruOuter(CExpressionHandle &exprhdl)
{
	GPOS_CHECK_ABORT;

	gpos::owner<IStatistics *> stats = exprhdl.Pstats(0);
	stats->AddRef();

	return stats;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PstatsBaseTable
//
//	@doc:
//		Helper for deriving statistics on a base table
//
//---------------------------------------------------------------------------
gpos::owner<IStatistics *>
CLogical::PstatsBaseTable(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::pointer<CTableDescriptor *> ptabdesc,
	gpos::pointer<CColRefSet *>
		pcrsHistExtra  // additional columns required for stats, not required by parent
)
{
	gpos::pointer<CReqdPropRelational *> prprel =
		gpos::dyn_cast<CReqdPropRelational>(exprhdl.Prp());
	gpos::owner<CColRefSet *> pcrsHist = GPOS_NEW(mp) CColRefSet(mp);
	pcrsHist->Include(prprel->PcrsStat());
	if (nullptr != pcrsHistExtra)
	{
		pcrsHist->Include(pcrsHistExtra);
	}

	gpos::pointer<CColRefSet *> pcrsOutput = exprhdl.DeriveOutputColumns();
	gpos::owner<CColRefSet *> pcrsWidth = GPOS_NEW(mp) CColRefSet(mp);
	pcrsWidth->Include(pcrsOutput);
	pcrsWidth->Exclude(pcrsHist);

	const COptCtxt *poctxt = COptCtxt::PoctxtFromTLS();
	CMDAccessor *md_accessor = poctxt->Pmda();
	gpos::pointer<CStatisticsConfig *> stats_config =
		poctxt->GetOptimizerConfig()->GetStatsConf();

	gpos::owner<IStatistics *> stats = md_accessor->Pstats(
		mp, ptabdesc->MDId(), pcrsHist, pcrsWidth, stats_config);

	// clean up
	pcrsWidth->Release();
	pcrsHist->Release();

	return stats;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PstatsDeriveDummy
//
//	@doc:
//		Derive dummy statistics
//
//---------------------------------------------------------------------------
gpos::owner<IStatistics *>
CLogical::PstatsDeriveDummy(CMemoryPool *mp, CExpressionHandle &exprhdl,
							CDouble rows) const
{
	GPOS_CHECK_ABORT;

	// return a dummy stats object that has a histogram for every
	// required-stats column
	GPOS_ASSERT(Esp(exprhdl) > EspNone);
	gpos::pointer<CReqdPropRelational *> prprel =
		gpos::dyn_cast<CReqdPropRelational>(exprhdl.Prp());
	gpos::pointer<CColRefSet *> pcrs = prprel->PcrsStat();
	gpos::owner<ULongPtrArray *> colids = GPOS_NEW(mp) ULongPtrArray(mp);
	pcrs->ExtractColIds(mp, colids);

	gpos::owner<IStatistics *> stats =
		CStatistics::MakeDummyStats(mp, colids, rows);

	// clean up
	colids->Release();

	return stats;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PexprPartPred
//
//	@doc:
//		Compute partition predicate to pass down to n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CLogical::PexprPartPred(CMemoryPool *,				   //mp,
						CExpressionHandle &,		   //exprhdl,
						gpos::pointer<CExpression *>,  //pexprInput,
						ULONG						   //child_index
) const
{
	GPOS_CHECK_ABORT;

	// the default behavior is to never pass down any partition predicates
	return nullptr;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogical::PdpCreate
//
//	@doc:
//		Create base container of derived properties
//
//---------------------------------------------------------------------------
gpos::owner<CDrvdProp *>
CLogical::PdpCreate(CMemoryPool *mp) const
{
	return GPOS_NEW(mp) CDrvdPropRelational(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogical::PrpCreate
//
//	@doc:
//		Create base container of required properties
//
//---------------------------------------------------------------------------
gpos::owner<CReqdProp *>
CLogical::PrpCreate(CMemoryPool *mp) const
{
	return GPOS_NEW(mp) CReqdPropRelational();
}


//---------------------------------------------------------------------------
//	@function:
//		CLogical::PtabdescFromTableGet
//
//	@doc:
//		Returns the table descriptor for (Dynamic)(BitmapTable)Get operators
//
//---------------------------------------------------------------------------
CTableDescriptor *
CLogical::PtabdescFromTableGet(gpos::pointer<COperator *> pop)
{
	GPOS_ASSERT(nullptr != pop);
	switch (pop->Eopid())
	{
		case CLogical::EopLogicalGet:
			return gpos::dyn_cast<CLogicalGet>(pop)->Ptabdesc();
		case CLogical::EopLogicalDynamicGet:
			return gpos::dyn_cast<CLogicalDynamicGet>(pop)->Ptabdesc();
		case CLogical::EopLogicalBitmapTableGet:
			return gpos::dyn_cast<CLogicalBitmapTableGet>(pop)->Ptabdesc();
		case CLogical::EopLogicalDynamicBitmapTableGet:
			return gpos::dyn_cast<CLogicalDynamicBitmapTableGet>(pop)
				->Ptabdesc();
		case CLogical::EopLogicalSelect:
			return gpos::dyn_cast<CLogicalSelect>(pop)->Ptabdesc();
		default:
			return nullptr;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PdrgpcrOutputFromLogicalGet
//
//	@doc:
//		Extract the output columns from a logical get or dynamic get operator
//
//---------------------------------------------------------------------------
CColRefArray *
CLogical::PdrgpcrOutputFromLogicalGet(gpos::pointer<CLogical *> pop)
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(COperator::EopLogicalGet == pop->Eopid() ||
				COperator::EopLogicalDynamicGet == pop->Eopid());

	if (COperator::EopLogicalGet == pop->Eopid())
	{
		return gpos::dyn_cast<CLogicalGet>(pop)->PdrgpcrOutput();
	}

	return gpos::dyn_cast<CLogicalDynamicGet>(pop)->PdrgpcrOutput();
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::NameFromLogicalGet
//
//	@doc:
//		Extract the name from a logical get or dynamic get operator
//
//---------------------------------------------------------------------------
const CName &
CLogical::NameFromLogicalGet(gpos::pointer<CLogical *> pop)
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(COperator::EopLogicalGet == pop->Eopid() ||
				COperator::EopLogicalDynamicGet == pop->Eopid());

	if (COperator::EopLogicalGet == pop->Eopid())
	{
		return gpos::dyn_cast<CLogicalGet>(pop)->Name();
	}

	return gpos::dyn_cast<CLogicalDynamicGet>(pop)->Name();
}

//---------------------------------------------------------------------------
//	@function:
//		CLogical::PcrsDist
//
//	@doc:
//		Return the set of distribution columns
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CLogical::PcrsDist(CMemoryPool *mp,
				   gpos::pointer<const CTableDescriptor *> ptabdesc,
				   gpos::pointer<const CColRefArray *> pdrgpcrOutput)
{
	GPOS_ASSERT(nullptr != ptabdesc);
	GPOS_ASSERT(nullptr != pdrgpcrOutput);

	gpos::pointer<const CColumnDescriptorArray *> pdrgpcoldesc =
		ptabdesc->Pdrgpcoldesc();
	gpos::pointer<const CColumnDescriptorArray *> pdrgpcoldescDist =
		ptabdesc->PdrgpcoldescDist();
	GPOS_ASSERT(nullptr != pdrgpcoldesc);
	GPOS_ASSERT(nullptr != pdrgpcoldescDist);
	GPOS_ASSERT(pdrgpcrOutput->Size() == pdrgpcoldesc->Size());


	// mapping base table columns to corresponding column references
	gpos::owner<IntToColRefMap *> phmicr = GPOS_NEW(mp) IntToColRefMap(mp);
	const ULONG num_cols = pdrgpcoldesc->Size();
	for (ULONG ul = 0; ul < num_cols; ul++)
	{
		gpos::pointer<CColumnDescriptor *> pcoldesc = (*pdrgpcoldesc)[ul];
		CColRef *colref = (*pdrgpcrOutput)[ul];

		phmicr->Insert(GPOS_NEW(mp) INT(pcoldesc->AttrNum()), colref);
	}

	gpos::owner<CColRefSet *> pcrsDist = GPOS_NEW(mp) CColRefSet(mp);
	const ULONG ulDistCols = pdrgpcoldescDist->Size();
	for (ULONG ul2 = 0; ul2 < ulDistCols; ul2++)
	{
		gpos::pointer<CColumnDescriptor *> pcoldesc = (*pdrgpcoldescDist)[ul2];
		const INT attno = pcoldesc->AttrNum();
		CColRef *pcrMapped = phmicr->Find(&attno);
		GPOS_ASSERT(nullptr != pcrMapped);
		pcrsDist->Include(pcrMapped);
	}

	// clean up
	phmicr->Release();

	return pcrsDist;
}

// EOF
