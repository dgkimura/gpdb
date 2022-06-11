//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 VMware, Inc. or its affiliates.
//
//	@filename:
//		CCastUtils.cpp
//
//	@doc:
//		Implementation of cast utility functions
//---------------------------------------------------------------------------

#include "gpopt/base/CCastUtils.h"

#include "gpos/common/owner.h"
#include "gpos/memory/CAutoMemoryPool.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/mdcache/CMDAccessorUtils.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarArrayCoerceExpr.h"
#include "gpopt/operators/CScalarCast.h"
#include "gpopt/operators/CScalarIdent.h"
#include "naucrates/md/CMDArrayCoerceCastGPDB.h"
#include "naucrates/md/IMDCast.h"

using namespace gpopt;
using namespace gpmd;

// is the given expression a binary coercible cast of a scalar identifier
BOOL
CCastUtils::FBinaryCoercibleCastedScId(CExpression *pexpr, CColRef *colref)
{
	GPOS_ASSERT(nullptr != pexpr);

	if (!FBinaryCoercibleCast(pexpr))
	{
		return false;
	}

	CExpression *pexprChild = (*pexpr)[0];

	// cast(col1)
	return COperator::EopScalarIdent == pexprChild->Pop()->Eopid() &&
		   colref == gpos::dyn_cast<CScalarIdent>(pexprChild->Pop())->Pcr();
}

// is the given expression a binary coercible cast of a scalar identifier
BOOL
CCastUtils::FBinaryCoercibleCastedScId(CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	if (!FBinaryCoercibleCast(pexpr))
	{
		return false;
	}

	CExpression *pexprChild = (*pexpr)[0];

	// cast(col1)
	return COperator::EopScalarIdent == pexprChild->Pop()->Eopid();
}

// is the given expression a binary coercible cast of a const
BOOL
CCastUtils::FBinaryCoercibleCastedConst(CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	if (!FBinaryCoercibleCast(pexpr))
	{
		return false;
	}

	CExpression *pexprChild = (*pexpr)[0];

	// cast(const)
	return COperator::EopScalarConst == pexprChild->Pop()->Eopid();
}

// extract the column reference if the given expression a scalar identifier
// or a cast of a scalar identifier or a function that casts a scalar identifier.
// Else return NULL.
const CColRef *
CCastUtils::PcrExtractFromScIdOrCastScId(CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	BOOL fScIdent = COperator::EopScalarIdent == pexpr->Pop()->Eopid();
	BOOL fCastedScIdent = CScalarIdent::FCastedScId(pexpr);

	// col or cast(col)
	if (!fScIdent && !fCastedScIdent)
	{
		return nullptr;
	}

	CScalarIdent *popScIdent = nullptr;
	if (fScIdent)
	{
		popScIdent = gpos::dyn_cast<CScalarIdent>(pexpr->Pop());
	}
	else
	{
		GPOS_ASSERT(fCastedScIdent);
		popScIdent = gpos::dyn_cast<CScalarIdent>((*pexpr)[0]->Pop());
	}

	return popScIdent->Pcr();
}

// cast the input column reference to the destination mdid
gpos::Ref<CExpression>
CCastUtils::PexprCast(CMemoryPool *mp, CMDAccessor *md_accessor,
					  const CColRef *colref, IMDId *mdid_dest)
{
	GPOS_ASSERT(nullptr != mdid_dest);

	IMDId *mdid_src = colref->RetrieveType()->MDId();
	GPOS_ASSERT(
		CMDAccessorUtils::FCastExists(md_accessor, mdid_src, mdid_dest));

	const IMDCast *pmdcast = md_accessor->Pmdcast(mdid_src, mdid_dest);

	;
	;
	gpos::Ref<CExpression> pexpr;

	if (pmdcast->GetMDPathType() == IMDCast::EmdtArrayCoerce)
	{
		CMDArrayCoerceCastGPDB *parrayCoerceCast =
			const_cast<CMDArrayCoerceCastGPDB *>(
				gpos::cast<CMDArrayCoerceCastGPDB>(pmdcast));
		pexpr = GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp) CScalarArrayCoerceExpr(
				mp, parrayCoerceCast->GetCastFuncMdId(), mdid_dest,
				parrayCoerceCast->TypeModifier(),
				parrayCoerceCast->IsExplicit(),
				(COperator::ECoercionForm) parrayCoerceCast->GetCoercionForm(),
				parrayCoerceCast->Location()),
			CUtils::PexprScalarIdent(mp, colref));
	}
	else
	{
		gpos::Ref<CScalarCast> popCast =
			GPOS_NEW(mp) CScalarCast(mp, mdid_dest, pmdcast->GetCastFuncMdId(),
									 pmdcast->IsBinaryCoercible());
		pexpr = GPOS_NEW(mp)
			CExpression(mp, popCast, CUtils::PexprScalarIdent(mp, colref));
	}
	return pexpr;
}

// check whether the given expression is a binary coercible cast of something
BOOL
CCastUtils::FBinaryCoercibleCast(CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	COperator *pop = pexpr->Pop();

	return FScalarCast(pexpr) &&
		   gpos::dyn_cast<CScalarCast>(pop)->IsBinaryCoercible();
}

BOOL
CCastUtils::FScalarCast(CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	COperator *pop = pexpr->Pop();

	return COperator::EopScalarCast == pop->Eopid();
}

// return the given expression without any binary coercible casts
// that exist on the top
CExpression *
CCastUtils::PexprWithoutBinaryCoercibleCasts(CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(pexpr->Pop()->FScalar());

	CExpression *pexprOutput = pexpr;

	while (FBinaryCoercibleCast(pexprOutput))
	{
		GPOS_ASSERT(1 == pexprOutput->Arity());
		pexprOutput = (*pexprOutput)[0];
	}

	return pexprOutput;
}

// add explicit casting to equality operations between compatible types
gpos::Ref<CExpressionArray>
CCastUtils::PdrgpexprCastEquality(CMemoryPool *mp, CExpression *pexpr)
{
	GPOS_ASSERT(pexpr->Pop()->FScalar());

	gpos::Ref<CExpressionArray> pdrgpexpr =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexpr);
	gpos::Ref<CExpressionArray> pdrgpexprNew =
		GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG ulPreds = pdrgpexpr->Size();
	for (ULONG ul = 0; ul < ulPreds; ul++)
	{
		gpos::Ref<CExpression> pexprPred = (*pdrgpexpr)[ul];
		;
		gpos::Ref<CExpression> pexprNewPred = pexprPred;

		if (CPredicateUtils::IsEqualityOp(pexprPred.get()) ||
			CPredicateUtils::FINDF(pexprPred.get()))
		{
			gpos::Ref<CExpression> pexprCasted =
				PexprAddCast(mp, pexprPred.get());
			if (nullptr != pexprCasted)
			{
				// release predicate since we will construct a new one
				;
				pexprNewPred = pexprCasted;
			}
		}
		pdrgpexprNew->Append(pexprNewPred);
	}

	;

	return pdrgpexprNew;
}

// add explicit casting to left child of given equality or INDF predicate
// and return resulting casted expression;
// the function returns NULL if operation failed
gpos::Ref<CExpression>
CCastUtils::PexprAddCast(CMemoryPool *mp, CExpression *pexprPred)
{
	GPOS_ASSERT(nullptr != pexprPred);
	GPOS_ASSERT(CUtils::FScalarCmp(pexprPred) ||
				CPredicateUtils::FINDF(pexprPred));

	CExpression *pexprChild = pexprPred;

	if (!CUtils::FScalarCmp(pexprPred))
	{
		pexprChild = (*pexprPred)[0];
	}

	CExpression *pexprLeft = (*pexprChild)[0];
	CExpression *pexprRight = (*pexprChild)[1];

	IMDId *mdid_type_left =
		gpos::dyn_cast<CScalar>(pexprLeft->Pop())->MdidType();
	IMDId *mdid_type_right =
		gpos::dyn_cast<CScalar>(pexprRight->Pop())->MdidType();

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	gpos::Ref<CExpression> pexprNewPred = nullptr;

	BOOL fTypesEqual = mdid_type_left->Equals(mdid_type_right);
	BOOL fCastLtoR = CMDAccessorUtils::FCastExists(md_accessor, mdid_type_left,
												   mdid_type_right);
	BOOL fCastRtoL = CMDAccessorUtils::FCastExists(md_accessor, mdid_type_right,
												   mdid_type_left);

	if (fTypesEqual || !(fCastLtoR || fCastRtoL))
	{
		return pexprNewPred;
	}

	;
	;

	gpos::Ref<CExpression> pexprNewLeft = pexprLeft;
	gpos::Ref<CExpression> pexprNewRight = pexprRight;

	if (fCastLtoR)
	{
		pexprNewLeft = PexprCast(mp, md_accessor, pexprLeft, mdid_type_right);
	}
	else
	{
		GPOS_ASSERT(fCastRtoL);
		pexprNewRight = PexprCast(mp, md_accessor, pexprRight, mdid_type_left);
		;
	}

	GPOS_ASSERT(nullptr != pexprNewLeft && nullptr != pexprNewRight);

	if (CUtils::FScalarCmp(pexprPred))
	{
		pexprNewPred = CUtils::PexprScalarCmp(mp, pexprNewLeft, pexprNewRight,
											  IMDType::EcmptEq);
	}
	else
	{
		pexprNewPred = CUtils::PexprINDF(mp, pexprNewLeft, pexprNewRight);
	}

	return pexprNewPred;
}

// add explicit casting on the input expression to the destination type
gpos::Ref<CExpression>
CCastUtils::PexprCast(CMemoryPool *mp, CMDAccessor *md_accessor,
					  CExpression *pexpr, IMDId *mdid_dest)
{
	IMDId *mdid_src = gpos::dyn_cast<CScalar>(pexpr->Pop())->MdidType();
	const IMDCast *pmdcast = md_accessor->Pmdcast(mdid_src, mdid_dest);

	;
	;
	gpos::Ref<CExpression> pexprCast;

	if (pmdcast->GetMDPathType() == IMDCast::EmdtArrayCoerce)
	{
		CMDArrayCoerceCastGPDB *parrayCoerceCast =
			const_cast<CMDArrayCoerceCastGPDB *>(
				gpos::cast<CMDArrayCoerceCastGPDB>(pmdcast));
		pexprCast = GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp) CScalarArrayCoerceExpr(
				mp, parrayCoerceCast->GetCastFuncMdId(), mdid_dest,
				parrayCoerceCast->TypeModifier(),
				parrayCoerceCast->IsExplicit(),
				(COperator::ECoercionForm) parrayCoerceCast->GetCoercionForm(),
				parrayCoerceCast->Location()),
			pexpr);
	}
	else
	{
		gpos::Ref<CScalarCast> popCast =
			GPOS_NEW(mp) CScalarCast(mp, mdid_dest, pmdcast->GetCastFuncMdId(),
									 pmdcast->IsBinaryCoercible());
		pexprCast = GPOS_NEW(mp) CExpression(mp, popCast, pexpr);
	}

	return pexprCast;
}

// EOF
