//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CScalarArray.cpp
//
//	@doc:
//		Implementation of scalar arrays
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarArray.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "naucrates/md/IMDAggregate.h"


using namespace gpopt;
using namespace gpmd;

// Ctor
CScalarArray::CScalarArray(CMemoryPool *mp, gpos::Ref<IMDId> elem_type_mdid,
						   gpos::Ref<IMDId> array_type_mdid,
						   BOOL is_multidimenstional)
	: CScalar(mp),
	  m_pmdidElem(std::move(elem_type_mdid)),
	  m_pmdidArray(std::move(array_type_mdid)),
	  m_fMultiDimensional(is_multidimenstional)
{
	GPOS_ASSERT(m_pmdidElem->IsValid());
	GPOS_ASSERT(m_pmdidArray->IsValid());
	m_pdrgPconst = GPOS_NEW(mp) CScalarConstArray(mp);
}


// Ctor
CScalarArray::CScalarArray(CMemoryPool *mp, gpos::Ref<IMDId> elem_type_mdid,
						   gpos::Ref<IMDId> array_type_mdid,
						   BOOL is_multidimenstional,
						   gpos::Ref<CScalarConstArray> pdrgPconst)
	: CScalar(mp),
	  m_pmdidElem(std::move(elem_type_mdid)),
	  m_pmdidArray(std::move(array_type_mdid)),
	  m_fMultiDimensional(is_multidimenstional),
	  m_pdrgPconst(std::move(pdrgPconst))
{
	GPOS_ASSERT(m_pmdidElem->IsValid());
	GPOS_ASSERT(m_pmdidArray->IsValid());
}

// Dtor
CScalarArray::~CScalarArray()
{
	;
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArray::PmdidElem
//
//	@doc:
//		Element type id
//
//---------------------------------------------------------------------------
IMDId *
CScalarArray::PmdidElem() const
{
	return m_pmdidElem.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArray::PmdidArray
//
//	@doc:
//		Array type id
//
//---------------------------------------------------------------------------
IMDId *
CScalarArray::PmdidArray() const
{
	return m_pmdidArray.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArray::FMultiDimensional
//
//	@doc:
//		Is array multi-dimensional
//
//---------------------------------------------------------------------------
BOOL
CScalarArray::FMultiDimensional() const
{
	return m_fMultiDimensional;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArray::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CScalarArray::HashValue() const
{
	return gpos::CombineHashes(
		CombineHashes(m_pmdidElem->HashValue(), m_pmdidArray->HashValue()),
		gpos::HashValue<BOOL>(&m_fMultiDimensional));
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarArray::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarArray::Matches(COperator *pop) const
{
	if (pop->Eopid() == Eopid())
	{
		CScalarArray *popArray = gpos::dyn_cast<CScalarArray>(pop);

		// match if components are identical
		if (popArray->FMultiDimensional() == FMultiDimensional() &&
			PmdidElem()->Equals(popArray->PmdidElem()) &&
			PmdidArray()->Equals(popArray->PmdidArray()) &&
			m_pdrgPconst->Size() == popArray->PdrgPconst()->Size())
		{
			for (ULONG ul = 0; ul < m_pdrgPconst->Size(); ul++)
			{
				CScalarConst *popConst1 = (*m_pdrgPconst)[ul].get();
				CScalarConst *popConst2 = (*popArray->PdrgPconst())[ul].get();
				if (!popConst1->Matches(popConst2))
				{
					return false;
				}
			}
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArray::MdidType
//
//	@doc:
//		Type of expression's result
//
//---------------------------------------------------------------------------
IMDId *
CScalarArray::MdidType() const
{
	return m_pmdidArray.get();
}

CScalarConstArray *
CScalarArray::PdrgPconst() const
{
	return m_pdrgPconst.get();
}

IOstream &
CScalarArray::OsPrint(IOstream &os) const
{
	os << "CScalarArray: {eleMDId: ";
	m_pmdidElem->OsPrint(os);
	os << ", arrayMDId: ";
	m_pmdidArray->OsPrint(os);
	if (m_fMultiDimensional)
	{
		os << ", multidimensional";
	}
	for (ULONG ul = 0; ul < m_pdrgPconst->Size(); ul++)
	{
		os << " ";
		(*m_pdrgPconst)[ul]->OsPrint(os);
	}
	os << "}";
	return os;
}


// EOF
