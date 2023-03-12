//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CScalarIdent.cpp
//
//	@doc:
//		Implementation of scalar identity operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarIdent.h"

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CScalarFunc.h"
#include "gpopt/search/CGroupExpression.h"


using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::HashValue
//
//	@doc:
//		Hash value built from colref and Eop
//
//---------------------------------------------------------------------------
ULONG
CScalarIdent::HashValue() const
{
	return gpos::CombineHashes(COperator::HashValue(),
							   CColRef::HashValue(m_pcr));
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarIdent::Matches(gpos::Ref<COperator> pop) const
{
	if (pop->Eopid() == Eopid())
	{
		gpos::Ref<CScalarIdent> popIdent = CScalarIdent::PopConvert(pop);

		// match if column reference is same
		return Pcr() == popIdent->Pcr();
	}

	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::FInputOrderSensitive
//
//	@doc:
//		Not called for leaf operators
//
//---------------------------------------------------------------------------
BOOL
CScalarIdent::FInputOrderSensitive() const
{
	GPOS_ASSERT(!"Unexpected call of function FInputOrderSensitive");
	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CScalarIdent::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::Ref<UlongToColRefMap> colref_mapping,
	BOOL must_exist)
{
	ULONG id = m_pcr->Id();
	gpos::Ref<CColRef> colref = colref_mapping->Find(id);
	if (nullptr == colref)
	{
		if (must_exist)
		{
			// not found in hashmap, so create a new colref and add to hashmap
			CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();

			colref = col_factory->PcrCopy(m_pcr);

			BOOL result GPOS_ASSERTS_ONLY = colref_mapping->Insert(id, colref);
			GPOS_ASSERT(result);
		}
		else
		{
			colref = m_pcr;
		}
	}

	return GPOS_NEW(mp) CScalarIdent(mp, colref);
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::MdidType
//
//	@doc:
//		Expression type
//
//---------------------------------------------------------------------------
gpos::Ref<IMDId>
CScalarIdent::MdidType() const
{
	return m_pcr->RetrieveType()->MDId();
}

INT
CScalarIdent::TypeModifier() const
{
	return m_pcr->TypeModifier();
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::FCastedScId
//
//	@doc:
// 		Is the given expression a scalar cast of a scalar identifier
//
//---------------------------------------------------------------------------
BOOL
CScalarIdent::FCastedScId(gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	// cast(col1)
	if (COperator::EopScalarCast == pexpr->Pop()->Eopid())
	{
		if (COperator::EopScalarIdent == (*pexpr)[0]->Pop()->Eopid())
		{
			return true;
		}
	}

	return false;
}

BOOL
CScalarIdent::FCastedScId(gpos::Ref<CExpression> pexpr,
						  gpos::Ref<CColRef> colref)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != colref);

	if (!FCastedScId(pexpr))
	{
		return false;
	}

	gpos::Ref<CScalarIdent> pScIdent =
		CScalarIdent::PopConvert((*pexpr)[0]->Pop());

	return colref == pScIdent->Pcr();
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::FAllowedFuncScId
//
//	@doc:
// 		Is the given expression a scalar func (which is an increasing function
//		allowed for partition selection) of a scalar identifier
//
//---------------------------------------------------------------------------
BOOL
CScalarIdent::FAllowedFuncScId(gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	if (COperator::EopScalarFunc == pexpr->Pop()->Eopid() &&
		pexpr->Arity() > 0 &&
		COperator::EopScalarIdent == (*pexpr)[0]->Pop()->Eopid())
	{
		gpos::Ref<CScalarFunc> func = CScalarFunc::PopConvert(pexpr->Pop());
		gpos::Ref<CMDIdGPDB> funcmdid = CMDIdGPDB::CastMdid(func->FuncMdId());
		CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
		return md_accessor->RetrieveFunc(funcmdid)->IsAllowedForPS();
	}
	return false;
}

BOOL
CScalarIdent::FAllowedFuncScId(gpos::Ref<CExpression> pexpr,
							   gpos::Ref<CColRef> colref)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != colref);

	if (!FAllowedFuncScId(pexpr))
	{
		return false;
	}

	gpos::Ref<CScalarIdent> pScIdent =
		CScalarIdent::PopConvert((*pexpr)[0]->Pop());

	return colref == pScIdent->Pcr();
}
//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CScalarIdent::OsPrint(IOstream &os) const
{
	os << SzId() << " ";
	m_pcr->OsPrint(os);

	return os;
}

// EOF
