//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CScalarCmp.cpp
//
//	@doc:
//		Implementation of scalar comparison operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarCmp.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/mdcache/CMDAccessorUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/search/CGroupExpression.h"
#include "naucrates/md/IMDScalarOp.h"
#include "naucrates/md/IMDTypeBool.h"

using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::CScalarCmp
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarCmp::CScalarCmp(CMemoryPool *mp, gpos::Ref<IMDId> mdid_op,
					   const CWStringConst *pstrOp, IMDType::ECmpType cmp_type)
	: CScalar(mp),
	  m_mdid_op(mdid_op),
	  m_pstrOp(pstrOp),
	  m_comparision_type(cmp_type),
	  m_returns_null_on_null_input(false)
{
	GPOS_ASSERT(m_mdid_op->IsValid());

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	m_returns_null_on_null_input =
		CMDAccessorUtils::FScalarOpReturnsNullOnNullInput(md_accessor,
														  m_mdid_op.get());
	m_fCommutative =
		CMDAccessorUtils::FCommutativeScalarOp(md_accessor, m_mdid_op.get());
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::GetMDName
//
//	@doc:
//		Comparison operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CScalarCmp::Pstr() const
{
	return m_pstrOp;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::MdIdOp
//
//	@doc:
//		Comparison operator metadata id
//
//---------------------------------------------------------------------------
gpos::Ref<IMDId>
CScalarCmp::MdIdOp() const
{
	return m_mdid_op.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::HashValue
//
//	@doc:
//		Operator specific hash function; combined hash of operator id and
//		metadata id
//
//---------------------------------------------------------------------------
ULONG
CScalarCmp::HashValue() const
{
	return gpos::CombineHashes(COperator::HashValue(), m_mdid_op->HashValue());
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarCmp::Matches(gpos::Ref<COperator> pop) const
{
	if (pop->Eopid() == Eopid())
	{
		gpos::Ref<CScalarCmp> popScCmp = gpos::dyn_cast<CScalarCmp>(pop);

		// match if operator oid are identical
		return m_mdid_op->Equals(popScCmp->MdIdOp());
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::FInputOrderSensitive
//
//	@doc:
//		Sensitivity to order of inputs
//
//---------------------------------------------------------------------------
BOOL
CScalarCmp::FInputOrderSensitive() const
{
	return !m_fCommutative;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::MdidType
//
//	@doc:
//		Expression type
//
//---------------------------------------------------------------------------
gpos::Ref<IMDId>
CScalarCmp::MdidType() const
{
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	return md_accessor->PtMDType<IMDTypeBool>()->MDId();
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::Eber
//
//	@doc:
//		Perform boolean expression evaluation
//
//---------------------------------------------------------------------------
CScalar::EBoolEvalResult
CScalarCmp::Eber(gpos::Ref<ULongPtrArray> pdrgpulChildren) const
{
	if (m_returns_null_on_null_input)
	{
		return EberNullOnAnyNullChild(pdrgpulChildren);
	}

	return EberAny;
}

// get metadata id of the commuted operator
gpos::Ref<IMDId>
CScalarCmp::PmdidCommuteOp(CMDAccessor *md_accessor, gpos::Ref<COperator> pop)
{
	gpos::Ref<CScalarCmp> popScalarCmp = gpos::dyn_cast<CScalarCmp>(pop);
	const gpos::Ref<IMDScalarOp> pmdScalarCmpOp =
		md_accessor->RetrieveScOp(popScalarCmp->MdIdOp());

	gpos::Ref<IMDId> pmdidScalarCmpCommute = pmdScalarCmpOp->GetCommuteOpMdid();
	return pmdidScalarCmpCommute;
}

// get the string representation of a metadata object
CWStringConst *
CScalarCmp::Pstr(CMemoryPool *mp, CMDAccessor *md_accessor,
				 gpos::Ref<IMDId> mdid)
{
	;
	return GPOS_NEW(mp) CWStringConst(
		mp,
		(md_accessor->RetrieveScOp(mdid)->Mdname().GetMDName())->GetBuffer());
}

// get commuted scalar comparision operator
gpos::Ref<CScalarCmp>
CScalarCmp::PopCommutedOp(CMemoryPool *mp)
{
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	gpos::Ref<IMDId> mdid = PmdidCommuteOp(md_accessor, this);
	if (nullptr != mdid && mdid->IsValid())
	{
		return GPOS_NEW(mp) CScalarCmp(mp, mdid, Pstr(mp, md_accessor, mdid),
									   CUtils::ParseCmpType(mdid));
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CScalarCmp::OsPrint(IOstream &os) const
{
	os << SzId() << " (";
	os << Pstr()->GetBuffer();
	os << ")";

	return os;
}


// EOF
