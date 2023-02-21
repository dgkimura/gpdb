//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CDistributionSpecHashed.cpp
//
//	@doc:
//		Specification of hashed distribution
//---------------------------------------------------------------------------

#include "gpopt/base/CDistributionSpecHashed.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CCastUtils.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CDrvdPropCtxtPlan.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CExpressionPreprocessor.h"
#include "gpopt/operators/CPhysicalMotionBroadcast.h"
#include "gpopt/operators/CPhysicalMotionHashDistribute.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarIdent.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/traceflags/traceflags.h"


#define GPOPT_DISTR_SPEC_HASHED_EXPRESSIONS (ULONG(5))

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::CDistributionSpecHashed
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDistributionSpecHashed::CDistributionSpecHashed(
	gpos::Ref<CExpressionArray> pdrgpexpr, BOOL fNullsColocated,
	gpos::Ref<IMdIdArray> opfamilies)
	: m_pdrgpexpr(pdrgpexpr),
	  m_opfamilies(opfamilies),
	  m_fNullsColocated(fNullsColocated),
	  m_pdshashedEquiv(nullptr),
	  m_equiv_hash_exprs(nullptr)
{
	GPOS_ASSERT(nullptr != m_pdrgpexpr);
	GPOS_ASSERT(0 < m_pdrgpexpr->Size());
	if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution) &&
		nullptr == m_opfamilies)
	{
		PopulateDefaultOpfamilies();
	}
	GPOS_ASSERT(m_opfamilies == nullptr ||
				m_opfamilies->Size() == m_pdrgpexpr->Size());
}

//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::CDistributionSpecHashed
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDistributionSpecHashed::CDistributionSpecHashed(
	gpos::Ref<CExpressionArray> pdrgpexpr, BOOL fNullsColocated,
	gpos::Ref<CDistributionSpecHashed> pdshashedEquiv,
	gpos::Ref<IMdIdArray> opfamilies)
	: m_pdrgpexpr(pdrgpexpr),
	  m_opfamilies(opfamilies),
	  m_fNullsColocated(fNullsColocated),
	  m_pdshashedEquiv(pdshashedEquiv),
	  m_equiv_hash_exprs(nullptr)
{
	GPOS_ASSERT(nullptr != m_pdrgpexpr);
	if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution) &&
		nullptr == m_opfamilies)
	{
		PopulateDefaultOpfamilies();
	}
	GPOS_ASSERT(m_opfamilies == nullptr ||
				m_opfamilies->Size() == m_pdrgpexpr->Size());
}

//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::~CDistributionSpecHashed
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDistributionSpecHashed::~CDistributionSpecHashed()
{
	;
	;
	;
}

void
CDistributionSpecHashed::PopulateDefaultOpfamilies()
{
	CMemoryPool *mp = COptCtxt::PoctxtFromTLS()->Pmp();
	CMDAccessor *mda = COptCtxt::PoctxtFromTLS()->Pmda();
	m_opfamilies = GPOS_NEW(mp) IMdIdArray(mp);
	for (ULONG ul = 0; ul < m_pdrgpexpr->Size(); ++ul)
	{
		gpos::Ref<CExpression> expr = (*m_pdrgpexpr)[ul].get();
		gpos::Ref<IMDId> mdid_type =
			gpos::dyn_cast<CScalar>(expr->Pop())->MdidType();
		gpos::Ref<IMDId> mdid_opfamily =
			mda->RetrieveType(mdid_type)->GetDistrOpfamilyMdid();
		GPOS_ASSERT(nullptr != mdid_opfamily && mdid_opfamily->IsValid());
		;
		m_opfamilies->Append(mdid_opfamily);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::PdsCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the distribution spec with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpec>
CDistributionSpecHashed::PdsCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::Ref<UlongToColRefMap> colref_mapping,
	BOOL must_exist)
{
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG length = m_pdrgpexpr->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::Ref<CExpression> pexpr = (*m_pdrgpexpr)[ul].get();
		pdrgpexpr->Append(pexpr->PexprCopyWithRemappedColumns(
			mp, colref_mapping, must_exist));
	}

	if (nullptr == m_pdshashedEquiv)
	{
		return GPOS_NEW(mp)
			CDistributionSpecHashed(std::move(pdrgpexpr), m_fNullsColocated);
	}

	// copy equivalent distribution
	gpos::Ref<CDistributionSpec> pds =
		m_pdshashedEquiv->PdsCopyWithRemappedColumns(mp, colref_mapping,
													 must_exist);
	gpos::Ref<CDistributionSpecHashed> pdshashed =
		gpos::dyn_cast<CDistributionSpecHashed>(pds);
	// Remapping columns should not change opfamily, used for passing distribution requests in CTEs
	return GPOS_NEW(mp)
		CDistributionSpecHashed(std::move(pdrgpexpr), m_fNullsColocated,
								std::move(pdshashed), m_opfamilies);
}


gpos::Ref<CDistributionSpec>
CDistributionSpecHashed::StripEquivColumns(CMemoryPool *mp)
{
	;
	return GPOS_NEW(mp)
		CDistributionSpecHashed(m_pdrgpexpr, m_fNullsColocated, m_opfamilies);
}


BOOL
CDistributionSpecHashed::FDistributionSpecHashedOnlyOnGpSegmentId() const
{
	const ULONG length = m_pdrgpexpr->Size();
	gpos::Ref<COperator> pop = (*(m_pdrgpexpr))[0]->Pop();

	return length == 1 && pop->Eopid() == COperator::EopScalarIdent &&
		   gpos::dyn_cast<CScalarIdent>(pop)->Pcr()->IsSystemCol() &&
		   gpos::dyn_cast<CScalarIdent>(pop)->Pcr()->Name().Equals(
			   CDXLTokens::GetDXLTokenStr(EdxltokenGpSegmentIdColName));
}


//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::FSatisfies
//
//	@doc:
//		Check if this distribution spec satisfies the given one
//
//---------------------------------------------------------------------------
BOOL
CDistributionSpecHashed::FSatisfies(
	const gpos::Ref<CDistributionSpec> pds) const
{
	if (nullptr != m_pdshashedEquiv && m_pdshashedEquiv->FSatisfies(pds))
	{
		return true;
	}

	if (Matches(pds))
	{
		// exact match implies satisfaction
		return true;
	}

	if (EdtAny == pds->Edt() || EdtNonSingleton == pds->Edt())
	{
		// hashed distribution satisfies the "any" and "non-singleton" distribution requirement
		return true;
	}

	if (EdtHashed != pds->Edt())
	{
		return false;
	}

	GPOS_ASSERT(EdtHashed == pds->Edt());

	const gpos::Ref<CDistributionSpecHashed> pdsHashed =
		gpos::dyn_cast<CDistributionSpecHashed>(pds);

	// Assumes that 'this' distribution spec is based on the underlying table
	// structure, whereas 'pds' distribution spec is based on the query
	// structure. Given that table based distribution spec is derived from
	// distribution columns, 'this' distribution spec should never satisfy
	// FDistributionSpecHashedOnlyOnGpSegmentId().
	if (pdsHashed->FDistributionSpecHashedOnlyOnGpSegmentId())
	{
		// If there exist HashSpecEquivExprs(), then we deny the match in order
		// to prevent incorrectly removing a REDISTRIBUTE MOTION. For example,
		// in the following query:
		//
		// SELECT * FROM t t1, t t2 WHERE t1.gp_segment_id=t2.id;
		if (nullptr == pdsHashed->HashSpecEquivExprs())
		{
			return true;
		}
	}

	return FMatchSubset(pdsHashed);
}


//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::FMatchSubset
//
//	@doc:
//		Check if the expressions of the object match a subset of the passed spec's
//		expressions
//
//---------------------------------------------------------------------------
BOOL
CDistributionSpecHashed::FMatchSubset(
	const gpos::Ref<CDistributionSpecHashed> pdsHashed) const
{
	const ULONG ulOwnExprs = m_pdrgpexpr->Size();
	const ULONG ulOtherExprs = pdsHashed->m_pdrgpexpr->Size();

	if (ulOtherExprs < ulOwnExprs || !FNullsColocatedCompatible(pdsHashed) ||
		!FDuplicateSensitiveCompatible(pdsHashed))
	{
		return false;
	}

	for (ULONG ulOuter = 0; ulOuter < ulOwnExprs; ulOuter++)
	{
		gpos::Ref<CExpression> pexprOwn =
			CCastUtils::PexprWithoutBinaryCoercibleCasts(
				(*m_pdrgpexpr)[ulOuter]);
		gpos::Ref<IMDId> opfamily_own = nullptr;

		BOOL fFound = false;
		gpos::Ref<CExpressionArrays> equiv_hash_exprs =
			pdsHashed->HashSpecEquivExprs();
		for (ULONG ulInner = 0; ulInner < ulOtherExprs; ulInner++)
		{
			gpos::Ref<CExpression> pexprOther =
				CCastUtils::PexprWithoutBinaryCoercibleCasts(
					(*(pdsHashed->m_pdrgpexpr))[ulInner]);
			gpos::Ref<IMDId> opfamily_other = nullptr;

			if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
			{
				opfamily_own = (*m_opfamilies)[ulOuter];
				opfamily_other = (*pdsHashed->m_opfamilies)[ulInner];
			}

			if (CUtils::Equals(pexprOwn, pexprOther) &&
				CUtils::Equals(opfamily_own.get(), opfamily_other.get()))
			{
				fFound = true;
				break;
			}

			if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
			{
				CMDAccessor *mda = COptCtxt::PoctxtFromTLS()->Pmda();
				gpos::Ref<IMDId> expr_type_mdid =
					gpos::dyn_cast<CScalar>(pexprOwn->Pop())->MdidType();
				const gpos::Ref<IMDType> expr_type =
					mda->RetrieveType(expr_type_mdid);

				if (expr_type->GetDistrOpfamilyMdid() != opfamily_own.get() ||
					expr_type->GetDistrOpfamilyMdid() != opfamily_other.get())
				{
					// check equiv_hash_exprs only for default opfamilies
					continue;
				}
			}

			if (nullptr != equiv_hash_exprs && equiv_hash_exprs->Size() > 0)
			{
				GPOS_ASSERT(false == fFound);
				gpos::Ref<CExpressionArray> equiv_distribution_exprs =
					(*equiv_hash_exprs)[ulInner].get();
				if (CUtils::Contains(equiv_distribution_exprs, pexprOwn))
				{
					fFound = true;
					break;
				}
			}
		}

		if (!fFound)
		{
			return false;
		}
	}

	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::PdshashedExcludeColumns
//
//	@doc:
//		Return a copy of the distribution spec after excluding the given
//		columns, return NULL if all distribution expressions are excluded
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpecHashed>
CDistributionSpecHashed::PdshashedExcludeColumns(CMemoryPool *mp,
												 gpos::Ref<CColRefSet> pcrs)
{
	GPOS_ASSERT(nullptr != pcrs);

	gpos::Ref<CExpressionArray> pdrgpexprNew =
		GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG ulExprs = m_pdrgpexpr->Size();
	for (ULONG ul = 0; ul < ulExprs; ul++)
	{
		gpos::Ref<CExpression> pexpr = (*m_pdrgpexpr)[ul];
		gpos::Ref<COperator> pop = pexpr->Pop();
		if (COperator::EopScalarIdent == pop->Eopid())
		{
			// we only care here about column identifiers,
			// any more complicated expressions are copied to output
			const CColRef *colref = gpos::dyn_cast<CScalarIdent>(pop)->Pcr();
			if (pcrs->FMember(colref))
			{
				continue;
			}
		}

		;
		pdrgpexprNew->Append(pexpr);
	}

	if (0 == pdrgpexprNew->Size())
	{
		;
		return nullptr;
	}

	return GPOS_NEW(mp)
		CDistributionSpecHashed(std::move(pdrgpexprNew), m_fNullsColocated);
}


//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::AppendEnforcers
//
//	@doc:
//		Add required enforcers to dynamic array
//
//---------------------------------------------------------------------------
void
CDistributionSpecHashed::AppendEnforcers(CMemoryPool *mp,
										 CExpressionHandle &,  // exprhdl
										 gpos::Ref<CReqdPropPlan>
#ifdef GPOS_DEBUG
											 prpp
#endif	// GPOS_DEBUG
										 ,
										 gpos::Ref<CExpressionArray> pdrgpexpr,
										 gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != prpp);
	GPOS_ASSERT(nullptr != pdrgpexpr);
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(!GPOS_FTRACE(EopttraceDisableMotions));
	GPOS_ASSERT(
		this == prpp->Ped()->PdsRequired() &&
		"required plan properties don't match enforced distribution spec");

	if (GPOS_FTRACE(EopttraceDisableMotionHashDistribute))
	{
		// hash-distribute Motion is disabled
		return;
	}

	// add a hashed distribution enforcer
	;
	;
	gpos::Ref<CExpression> pexprMotion = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CPhysicalMotionHashDistribute(mp, this), pexpr);
	pdrgpexpr->Append(std::move(pexprMotion));
}


//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CDistributionSpecHashed::HashValue() const
{
	gpos::Ref<CDistributionSpecHashed> equiv_spec = this->PdshashedEquiv();
	if (nullptr != equiv_spec)
	{
		return equiv_spec->HashValue();
	}

	ULONG ulHash = (ULONG) Edt();

	ULONG ulHashedExpressions =
		std::min(m_pdrgpexpr->Size(), GPOPT_DISTR_SPEC_HASHED_EXPRESSIONS);

	for (ULONG ul = 0; ul < ulHashedExpressions; ul++)
	{
		gpos::Ref<CExpression> pexpr = (*m_pdrgpexpr)[ul].get();
		ulHash = gpos::CombineHashes(ulHash, CExpression::HashValue(pexpr));
	}

	if (nullptr != m_opfamilies && m_opfamilies->Size() > 0)
	{
		for (ULONG ul = 0; ul < m_opfamilies->Size(); ul++)
		{
			gpos::Ref<IMDId> mdid = (*m_opfamilies)[ul];
			ulHash = gpos::CombineHashes(ulHash, mdid->HashValue());
		}
	}

	if (nullptr != m_equiv_hash_exprs && m_equiv_hash_exprs->Size() > 0)
	{
		for (ULONG ul = 0; ul < m_equiv_hash_exprs->Size(); ul++)
		{
			gpos::Ref<CExpressionArray> equiv_distribution_exprs =
				(*m_equiv_hash_exprs)[ul].get();
			for (ULONG id = 0; id < equiv_distribution_exprs->Size(); id++)
			{
				gpos::Ref<CExpression> pexpr =
					(*equiv_distribution_exprs)[id].get();
				ulHash =
					gpos::CombineHashes(ulHash, CExpression::HashValue(pexpr));
			}
		}
	}
	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::PcrsUsed
//
//	@doc:
//		Extract columns used by the distribution spec
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CDistributionSpecHashed::PcrsUsed(CMemoryPool *mp) const
{
	return CUtils::PcrsExtractColumns(mp, m_pdrgpexpr.get());
}


//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::FMatchHashedDistribution
//
//	@doc:
//		Exact match against given hashed distribution
//
//---------------------------------------------------------------------------
BOOL
CDistributionSpecHashed::FMatchHashedDistribution(
	const gpos::Ref<CDistributionSpecHashed> pdshashed) const
{
	GPOS_ASSERT(nullptr != pdshashed);

	if (m_pdrgpexpr->Size() != pdshashed->m_pdrgpexpr->Size() ||
		!FNullsColocatedCompatible(pdshashed) ||
		IsDuplicateSensitive() != pdshashed->IsDuplicateSensitive())
	{
		return false;
	}

	if (!CUtils::Equals(m_opfamilies.get(), pdshashed->m_opfamilies.get()))
	{
		return false;
	}

	const ULONG length = m_pdrgpexpr->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::Ref<CExpressionArrays> all_equiv_exprs =
			pdshashed->HashSpecEquivExprs();
		gpos::Ref<CExpressionArray> equiv_distribution_exprs = nullptr;
		if (nullptr != all_equiv_exprs && all_equiv_exprs->Size() > 0)
		{
			equiv_distribution_exprs = (*all_equiv_exprs)[ul];
		}
		gpos::Ref<CExpression> pexprLeft =
			(*(pdshashed->m_pdrgpexpr))[ul].get();
		gpos::Ref<CExpression> pexprRight = (*m_pdrgpexpr)[ul].get();
		BOOL fSuccess = CUtils::Equals(pexprLeft, pexprRight);
		if (!fSuccess)
		{
			// if failed to find a equal match in the source distribution expr
			// array, check the equivalent exprs to find a match
			fSuccess = CUtils::Contains(equiv_distribution_exprs, pexprRight);
		}
		if (!fSuccess)
		{
			return fSuccess;
		}
	}

	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::Matches
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CDistributionSpecHashed::Matches(const gpos::Ref<CDistributionSpec> pds) const
{
	GPOS_CHECK_STACK_SIZE;

	if (Edt() != pds->Edt())
	{
		return false;
	}

	const gpos::Ref<CDistributionSpecHashed> pdshashed =
		CDistributionSpecHashed::PdsConvert(pds);

	if (nullptr != m_pdshashedEquiv && m_pdshashedEquiv->Matches(pdshashed))
	{
		return true;
	}

	if (nullptr != pdshashed->PdshashedEquiv() &&
		pdshashed->PdshashedEquiv()->Matches(
			gpos::dyn_cast<CDistributionSpec>(SharedFromThis())))
	{
		return true;
	}

	return FMatchHashedDistribution(pdshashed);
}

BOOL
CDistributionSpecHashed::Equals(
	const gpos::Ref<CDistributionSpec> input_spec) const
{
	GPOS_CHECK_STACK_SIZE;

	if (input_spec->Edt() != Edt())
	{
		return false;
	}

	const gpos::Ref<CDistributionSpecHashed> other_spec =
		CDistributionSpecHashed::PdsConvert(input_spec);

	gpos::Ref<CDistributionSpecHashed> spec_equiv = this->PdshashedEquiv();
	gpos::Ref<CDistributionSpecHashed> other_spec_equiv =
		other_spec->PdshashedEquiv();
	// if one of the spec has equivalent spec and other doesn't, they are not equal
	if ((spec_equiv != nullptr && other_spec_equiv == nullptr) ||
		(spec_equiv == nullptr && other_spec_equiv != nullptr))
	{
		return false;
	}

	BOOL equals = true;
	// if both the specs has equivalent specs, compare them
	if (nullptr != spec_equiv && nullptr != other_spec_equiv)
	{
		equals = spec_equiv->Equals(other_spec_equiv);
	}
	// if the equivalent spec are not equal, the spec objects are not equal
	if (!equals)
	{
		return false;
	}

	if (!CUtils::Equals(m_opfamilies.get(), other_spec->m_opfamilies.get()))
	{
		return false;
	}

	BOOL matches =
		m_fNullsColocated == other_spec->FNullsColocated() &&
		m_is_duplicate_sensitive == other_spec->IsDuplicateSensitive() &&
		m_fSatisfiedBySingleton == other_spec->FSatisfiedBySingleton() &&
		CUtils::Equals(m_pdrgpexpr.get(), other_spec->m_pdrgpexpr.get());

	if (!matches)
	{
		return false;
	}

	// compare the equivalent expression arrays
	gpos::Ref<CExpressionArrays> spec_equiv_exprs = m_equiv_hash_exprs.get();
	gpos::Ref<CExpressionArrays> other_spec_equiv_exprs =
		other_spec->HashSpecEquivExprs();

	return CUtils::Equals(spec_equiv_exprs, other_spec_equiv_exprs);
}

//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::PdshashedMaximal
//
//	@doc:
//		Return a hashed distribution on the maximal hashable subset of
//		given columns,
//		if all columns are not hashable, return NULL
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpecHashed>
CDistributionSpecHashed::PdshashedMaximal(CMemoryPool *mp,
										  gpos::Ref<CColRefArray> colref_array,
										  BOOL fNullsColocated)
{
	GPOS_ASSERT(nullptr != colref_array);
	GPOS_ASSERT(0 < colref_array->Size());

	gpos::Ref<CColRefArray> pdrgpcrHashable =
		CUtils::PdrgpcrRedistributableSubset(mp, colref_array);
	gpos::Ref<CDistributionSpecHashed> pdshashed = nullptr;
	if (0 < pdrgpcrHashable->Size())
	{
		gpos::Ref<CExpressionArray> pdrgpexpr =
			CUtils::PdrgpexprScalarIdents(mp, pdrgpcrHashable.get());
		pdshashed =
			GPOS_NEW(mp) CDistributionSpecHashed(pdrgpexpr, fNullsColocated);
	};

	return pdshashed;
}

// check if the distribution key expression are covered by the input
// expression array
BOOL
CDistributionSpecHashed::IsCoveredBy(
	const gpos::Ref<CExpressionArray> dist_cols_expr_array)
{
	BOOL covers = false;
	gpos::Ref<CDistributionSpecHashed> pds =
		gpos::dyn_cast<CDistributionSpecHashed>(SharedFromThis());
	while (pds && !covers)
	{
		covers = CUtils::Contains(dist_cols_expr_array, pds->Pdrgpexpr());
		pds = pds->PdshashedEquiv();
	}
	return covers;
}

// iterate over all the distribution exprs keys and create an array holding
// equivalent exprs which correspond to the same distribution based on equivalence
// for example: select * from t1, t2 where t1.a = t2.c and t1.b = t2.d;
// if the resulting spec consists of column t1.a, t1,b, based on the equivalence,
// it implies that t1.a distribution is equivalent to t2.c and t1.b is equivalent to t2.d
// t1.a --> equivalent exprs: [t1.a, t2.c]
// t1.b --> equivalent exprs:[t1.b, t2.d]
void
CDistributionSpecHashed::ComputeEquivHashExprs(
	CMemoryPool *mp, CExpressionHandle &expression_handle)
{
	gpos::Ref<CExpressionArray> distribution_exprs = m_pdrgpexpr.get();
	gpos::Ref<CExpressionArrays> equiv_distribution_all_exprs =
		m_equiv_hash_exprs;
	if (nullptr == equiv_distribution_all_exprs)
	{
		// array which holds equivalent scalar expression for each of the distribution key
		equiv_distribution_all_exprs = GPOS_NEW(mp) CExpressionArrays(mp);
		for (ULONG distribution_key_idx = 0;
			 distribution_key_idx < distribution_exprs->Size();
			 distribution_key_idx++)
		{
			// array which holds equivalent expression for the current distribution key
			gpos::Ref<CExpressionArray> equiv_distribution_exprs =
				GPOS_NEW(mp) CExpressionArray(mp);

			gpos::Ref<CExpression> distribution_expr =
				(*distribution_exprs)[distribution_key_idx];
			gpos::Ref<CColRefSet> distribution_expr_cols =
				distribution_expr->DeriveUsedColumns();
			// the input expr is always equivalent to itself, so add it to the equivalent expr array
			;
			equiv_distribution_exprs->Append(distribution_expr);

			if (1 != distribution_expr_cols->Size())
			{
				// if there are 0 or more than 1 column in the distribution expr, there will be no
				// equivalent columns for the expr
				equiv_distribution_all_exprs->Append(equiv_distribution_exprs);
				continue;
			}

			// there is only one colref in the set
			const CColRef *distribution_colref =
				distribution_expr_cols->PcrAny();
			GPOS_ASSERT(nullptr != distribution_colref);
			gpos::Ref<CColRefSet> equiv_cols =
				expression_handle.DerivePropertyConstraint()->PcrsEquivClass(
					distribution_colref);
			// if there are equivalent columns, then we have a chance to create equivalent distribution exprs
			if (nullptr != equiv_cols)
			{
				CColRefSetIter equiv_cols_iter(*equiv_cols);
				while (equiv_cols_iter.Advance())
				{
					CColRef *equiv_col = equiv_cols_iter.Pcr();
					if (equiv_col == distribution_colref)
					{
						// skip the current distribution colref as we need to create the scalar
						// condition with its equivalent colrefs
						continue;
					}
					gpos::Ref<CExpression> predicate_expr_with_inferred_quals =
						CUtils::PexprScalarEqCmp(mp, distribution_colref,
												 equiv_col);
					// add cast on the expressions if required, both the outer and inner hash exprs
					// should be of the same data type else they may be hashed to different segments
					gpos::Ref<CExpression> predicate_expr_with_casts =
						CCastUtils::PexprAddCast(
							mp, predicate_expr_with_inferred_quals.get());
					gpos::Ref<CExpression> original_predicate_expr =
						predicate_expr_with_inferred_quals;
					if (predicate_expr_with_casts)
					{
						;
						original_predicate_expr = predicate_expr_with_casts;
					}
					gpos::Ref<CExpression> left_distribution_expr =
						CCastUtils::PexprWithoutBinaryCoercibleCasts(
							(*original_predicate_expr)[0]);
					gpos::Ref<CExpression> right_distribution_expr =
						CCastUtils::PexprWithoutBinaryCoercibleCasts(
							(*original_predicate_expr)[1]);
					// if the predicate is a = b, and a is the current distribution expr,
					// then the equivalent expr is b
					gpos::Ref<CExpression> equiv_distribution_expr = nullptr;
					if (CUtils::Equals(left_distribution_expr,
									   distribution_expr))
					{
						equiv_distribution_expr = right_distribution_expr;
					}
					else if (CUtils::Equals(right_distribution_expr,
											distribution_expr))
					{
						equiv_distribution_expr = left_distribution_expr;
					}

					// if equivalent distributione expr is found, add it to the array holding equivalent distribution exprs
					if (equiv_distribution_expr)
					{
						;
						equiv_distribution_exprs->Append(
							equiv_distribution_expr);
					};
				}
			}
			equiv_distribution_all_exprs->Append(equiv_distribution_exprs);
		}
	}
	GPOS_ASSERT(equiv_distribution_all_exprs->Size() == m_pdrgpexpr->Size());
	m_equiv_hash_exprs = equiv_distribution_all_exprs;
}

gpos::Ref<CDistributionSpecHashed>
CDistributionSpecHashed::Copy(CMemoryPool *mp)
{
	gpos::Ref<CExpressionArray> distribution_exprs = this->Pdrgpexpr();
	gpos::Ref<CExpressionArrays> equiv_distribution_exprs =
		GPOS_NEW(mp) CExpressionArrays(mp);
	gpos::Ref<CDistributionSpecHashed> pds = this;
	while (pds)
	{
		gpos::Ref<CExpressionArray> distribution_exprs = pds->Pdrgpexpr();
		;
		equiv_distribution_exprs->Append(distribution_exprs);
		pds = pds->PdshashedEquiv();
	}

	gpos::Ref<CDistributionSpecHashed> spec = nullptr;
	for (ULONG ul = 1; ul < equiv_distribution_exprs->Size(); ul++)
	{
		gpos::Ref<CExpressionArray> distribution_exprs =
			(*equiv_distribution_exprs)[ul];
		;
		spec = GPOS_NEW(mp) CDistributionSpecHashed(
			distribution_exprs, this->FNullsColocated(), spec);
	}

	;
	gpos::Ref<CDistributionSpecHashed> spec_copy = GPOS_NEW(mp)
		CDistributionSpecHashed(distribution_exprs, this->FNullsColocated(),
								std::move(spec), m_opfamilies);
	;
	GPOS_ASSERT(nullptr != spec_copy);
	return spec_copy;
}
//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecHashed::OsPrint
//
//	@doc:
//		Print function
//
//---------------------------------------------------------------------------
IOstream &
CDistributionSpecHashed::OsPrint(IOstream &os) const
{
	return OsPrintWithPrefix(os, "");
}

IOstream &
CDistributionSpecHashed::OsPrintWithPrefix(IOstream &os,
										   const char *prefix) const
{
	os << this->SzId() << ": [ ";
	const ULONG length = m_pdrgpexpr->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::Ref<CExpression> hash_expr = (*m_pdrgpexpr)[ul].get();

		if (!GPOS_FTRACE(EopttracePrintEquivDistrSpecs) && 1 == length &&
			COperator::EopScalarIdent == hash_expr->Pop()->Eopid())
		{
			// just print the scalar ident, without surrounding decoration
			hash_expr->Pop()->OsPrint(os);
		}
		else
		{
			os << *(hash_expr);
			// the expression added a newline, indent the following with the prefix
			os << prefix;
		}
	}
	if (m_fNullsColocated)
	{
		os << ", nulls colocated";
	}
	else
	{
		os << ", nulls not colocated";
	}

	if (m_is_duplicate_sensitive)
	{
		os << ", duplicate sensitive";
	}

	if (!m_fSatisfiedBySingleton)
	{
		os << ", across-segments";
	}

	os << " ]";

	if (nullptr != m_pdshashedEquiv &&
		GPOS_FTRACE(EopttracePrintEquivDistrSpecs))
	{
		os << ", equiv. dist: ";
		m_pdshashedEquiv->OsPrint(os);
	}

	if (nullptr != m_equiv_hash_exprs && m_equiv_hash_exprs->Size() > 0 &&
		GPOS_FTRACE(EopttracePrintEquivDistrSpecs))
	{
		os << "," << std::endl;
		for (ULONG ul = 0; ul < m_equiv_hash_exprs->Size(); ul++)
		{
			gpos::Ref<CExpressionArray> equiv_distribution_exprs =
				(*m_equiv_hash_exprs)[ul].get();
			os << "equiv exprs: " << ul << ":";
			for (ULONG id = 0; id < equiv_distribution_exprs->Size(); id++)
			{
				gpos::Ref<CExpression> equiv_distribution_expr =
					(*equiv_distribution_exprs)[id].get();
				os << *equiv_distribution_expr << ",";
			}
		}
	}

	if (nullptr != m_opfamilies && m_opfamilies->Size() > 0)
	{
		os << ", opfamilies: ";
		for (ULONG ul = 0; ul < m_opfamilies->Size(); ul++)
		{
			(*m_opfamilies)[ul]->OsPrint(os);
			os << ",";
		}
	}

	return os;
}

gpos::Ref<CExpressionArrays>
CDistributionSpecHashed::GetAllDistributionExprs(CMemoryPool *mp)
{
	gpos::Ref<CExpressionArrays> all_distribution_exprs =
		GPOS_NEW(mp) CExpressionArrays(mp);
	gpos::Ref<CDistributionSpecHashed> spec = this;
	while (spec)
	{
		gpos::Ref<CExpressionArray> distribution_exprs = spec->Pdrgpexpr();
		;
		all_distribution_exprs->Append(distribution_exprs);
		spec = spec->PdshashedEquiv();
	}

	return all_distribution_exprs;
}

// create a new spec and which marks the other incoming specs
// as equivalent
gpos::Ref<CDistributionSpecHashed>
CDistributionSpecHashed::Combine(CMemoryPool *mp,
								 gpos::Ref<CDistributionSpecHashed> other_spec)
{
	gpos::Ref<CExpressionArrays> distribution_exprs =
		this->GetAllDistributionExprs(mp);
	gpos::Ref<CExpressionArrays> other_distribution_exprs =
		other_spec->GetAllDistributionExprs(mp);
	gpos::Ref<CExpressionArrays> all_distribution_exprs =
		CUtils::GetCombinedExpressionArrays(mp, distribution_exprs.get(),
											other_distribution_exprs.get());

	gpos::Ref<CDistributionSpecHashed> combined_hashed_spec = nullptr;
	for (ULONG ul = 0; ul < all_distribution_exprs->Size(); ul++)
	{
		gpos::Ref<CExpressionArray> exprs = (*all_distribution_exprs)[ul];
#ifdef GPOS_DEBUG
		// ensure that all the spec has the same size
		GPOS_ASSERT(this->Pdrgpexpr()->Size() == exprs->Size());
#endif
		;
		combined_hashed_spec = GPOS_NEW(mp) CDistributionSpecHashed(
			exprs, this->FNullsColocated(), combined_hashed_spec, m_opfamilies);
	};
	;
	;
	GPOS_ASSERT(nullptr != combined_hashed_spec);
	return combined_hashed_spec;
}

// check if the equivalent spec (if any) has no matching columns with the main spec
BOOL
CDistributionSpecHashed::HasCompleteEquivSpec(CMemoryPool *mp) const
{
	gpos::Ref<CDistributionSpecHashed> pdshashedEquiv = this->PdshashedEquiv();

	if (nullptr != pdshashedEquiv)
	{
		gpos::Ref<CColRefSet> pcrshashed = this->PcrsUsed(mp);
		gpos::Ref<CColRefSet> pcrshashedEquiv = pdshashedEquiv->PcrsUsed(mp);

		// it is considered complete, if the equivalent spec has no matching
		// columns with the main spec
		BOOL result = !pcrshashedEquiv->FIntersects(pcrshashed.get());
		;
		;
		return result;
	}

	// no equivalent spec - so, it can't be incomplete
	return false;
}

// use given predicates to complete an incomplete spec, if possible
gpos::Ref<CDistributionSpecHashed>
CDistributionSpecHashed::TryToCompleteEquivSpec(
	CMemoryPool *mp, gpos::Ref<CDistributionSpecHashed> pdshashed,
	gpos::Ref<CExpression> pexprPred, gpos::Ref<CColRefSet> outerRefs)
{
	gpos::Ref<CExpressionArray> pdrgpexprPred =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprPred);
	gpos::Ref<CExpressionArray> pdrgpexprResult =
		GPOS_NEW(mp) CExpressionArray(mp);
	gpos::Ref<CExpressionArray> pdrgpexprHashed = pdshashed->Pdrgpexpr();
	const ULONG size = pdrgpexprHashed->Size();

	ULONG num_orig_exprs_used = 0;
	for (ULONG ul = 0; ul < size; ul++)
	{
		gpos::Ref<CExpression> pexpr = (*pdrgpexprHashed)[ul];
		gpos::Ref<CExpression> pexprMatching =
			CUtils::PexprMatchEqualityOrINDF(pexpr, pdrgpexprPred.get());
		if (nullptr != pexprMatching &&
			outerRefs->FIntersects(pexprMatching->DeriveUsedColumns()))
		{
			// we are able to replace an original expression with one that refers to outer
			// references (values from the equivalent table), making it more complete
			;
			pdrgpexprResult->Append(pexprMatching);
		}
		else
		{
			++num_orig_exprs_used;
			;
			pdrgpexprResult->Append(pexpr);
		}
	};

	// don't create an equiv spec, if it uses the same exprs as the original
	if (num_orig_exprs_used == size)
	{
		;
		return nullptr;
	}

	GPOS_ASSERT(pdrgpexprResult->Size() == pdrgpexprHashed->Size());

	// create a matching hashed distribution request
	BOOL fNullsColocated = pdshashed->FNullsColocated();
	gpos::Ref<CDistributionSpecHashed> pdshashedEquiv = GPOS_NEW(mp)
		CDistributionSpecHashed(std::move(pdrgpexprResult), fNullsColocated);

	return pdshashedEquiv;
}

gpos::Ref<CDistributionSpecHashed>
CDistributionSpecHashed::MakeHashedDistrSpec(
	CMemoryPool *mp, gpos::Ref<CExpressionArray> pdrgpexpr,
	BOOL fNullsColocated, gpos::Ref<CDistributionSpecHashed> pdshashedEquiv,
	gpos::Ref<IMdIdArray> opfamilies)
{
	if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution) &&
		nullptr == opfamilies)
	{
		CMDAccessor *mda = COptCtxt::PoctxtFromTLS()->Pmda();
		opfamilies = GPOS_NEW(mp) IMdIdArray(mp);
		for (ULONG ul = 0; ul < pdrgpexpr->Size(); ++ul)
		{
			gpos::Ref<CExpression> expr = (*pdrgpexpr)[ul].get();
			gpos::Ref<IMDId> mdid_type =
				gpos::dyn_cast<CScalar>(expr->Pop())->MdidType();
			gpos::Ref<IMDId> mdid_opfamily =
				mda->RetrieveType(mdid_type)->GetDistrOpfamilyMdid();
			if (nullptr == mdid_opfamily)
			{
				;
				return nullptr;
			};
			opfamilies->Append(mdid_opfamily);
		}
	}
	return GPOS_NEW(mp) CDistributionSpecHashed(
		std::move(pdrgpexpr), fNullsColocated, std::move(pdshashedEquiv),
		std::move(opfamilies));
}
// EOF
