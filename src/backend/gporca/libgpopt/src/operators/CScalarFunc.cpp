//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CScalarFunc.cpp
//
//	@doc:
//		Implementation of scalar function call operators
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarFunc.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/mdcache/CMDAccessorUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "naucrates/md/IMDFunction.h"


using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CScalarFunc::CScalarFunc
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarFunc::CScalarFunc(CMemoryPool *mp)
	: CScalar(mp),
	  m_func_mdid(nullptr),
	  m_return_type_mdid(nullptr),
	  m_return_type_modifier(default_type_modifier),
	  m_pstrFunc(nullptr),
	  m_efs(IMDFunction::EfsSentinel),
	  m_returns_set(false),
	  m_returns_null_on_null_input(false),
	  m_fBoolReturnType(false)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarFunc::CScalarFunc
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarFunc::CScalarFunc(CMemoryPool *mp, gpos::Ref<IMDId> mdid_func,
						 gpos::Ref<IMDId> mdid_return_type,
						 INT return_type_modifier,
						 const CWStringConst *pstrFunc)
	: CScalar(mp),
	  m_func_mdid(mdid_func),
	  m_return_type_mdid(mdid_return_type),
	  m_return_type_modifier(return_type_modifier),
	  m_pstrFunc(pstrFunc),
	  m_returns_set(false),
	  m_returns_null_on_null_input(false),
	  m_fBoolReturnType(false)
{
	GPOS_ASSERT(m_func_mdid->IsValid());
	GPOS_ASSERT(m_return_type_mdid->IsValid());

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	const IMDFunction *pmdfunc = md_accessor->RetrieveFunc(m_func_mdid.get());

	m_efs = pmdfunc->GetFuncStability();
	m_returns_set = pmdfunc->ReturnsSet();

	m_returns_null_on_null_input = pmdfunc->IsStrict();
	m_fBoolReturnType =
		CMDAccessorUtils::FBoolType(md_accessor, m_return_type_mdid.get());
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarFunc::~CScalarFunc
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CScalarFunc::~CScalarFunc()
{
	GPOS_DELETE(m_pstrFunc);
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarFunc::PstrFunc
//
//	@doc:
//		Function name
//
//---------------------------------------------------------------------------
const CWStringConst *
CScalarFunc::PstrFunc() const
{
	return m_pstrFunc;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarFunc::FuncMdId
//
//	@doc:
//		Func id
//
//---------------------------------------------------------------------------
gpos::Ref<IMDId>
CScalarFunc::FuncMdId() const
{
	return m_func_mdid.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarFunc::EFuncStbl
//
//	@doc:
//		Function stability enum
//
//---------------------------------------------------------------------------
IMDFunction::EFuncStbl
CScalarFunc::EfsGetFunctionStability() const
{
	return m_efs;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarFunc::HashValue
//
//	@doc:
//		Operator specific hash function; combined hash of operator id and
//		id of function
//
//---------------------------------------------------------------------------
ULONG
CScalarFunc::HashValue() const
{
	return gpos::CombineHashes(
		COperator::HashValue(),
		gpos::CombineHashes(m_func_mdid->HashValue(),
							m_return_type_mdid->HashValue()));
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarFunc::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarFunc::Matches(COperator *pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}
	CScalarFunc *popScFunc = gpos::dyn_cast<CScalarFunc>(pop);

	// match if func ids are identical
	return popScFunc->FuncMdId()->Equals(m_func_mdid.get()) &&
		   popScFunc->MdidType()->Equals(m_return_type_mdid.get());
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarFunc::MdidType
//
//	@doc:
//		Expression type
//
//---------------------------------------------------------------------------
gpos::Ref<IMDId>
CScalarFunc::MdidType() const
{
	return m_return_type_mdid.get();
}

INT
CScalarFunc::TypeModifier() const
{
	return m_return_type_modifier;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarFunc::FHasNonScalarFunction
//
//	@doc:
//		Derive existence of non-scalar functions from expression handle
//
//---------------------------------------------------------------------------
BOOL
CScalarFunc::FHasNonScalarFunction(CExpressionHandle &exprhdl)
{
	return m_returns_set || CScalar::FHasNonScalarFunction(exprhdl);
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarFunc::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CScalarFunc::OsPrint(IOstream &os) const
{
	os << SzId() << " (";
	os << PstrFunc()->GetBuffer();
	os << ")";

	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarFunc::Eber
//
//	@doc:
//		Perform boolean expression evaluation
//
//---------------------------------------------------------------------------
CScalar::EBoolEvalResult
CScalarFunc::Eber(ULongPtrArray *pdrgpulChildren) const
{
	if (m_returns_null_on_null_input)
	{
		return EberNullOnAnyNullChild(pdrgpulChildren);
	}

	return EberAny;
}


// EOF
