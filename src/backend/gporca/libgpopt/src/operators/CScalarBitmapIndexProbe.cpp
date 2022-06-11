//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CScalarBitmapIndexProbe.cpp
//
//	@doc:
//		Bitmap index probe scalar operator
//
//	@owner:
//
//
//	@test:
//
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarBitmapIndexProbe.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/metadata/CIndexDescriptor.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/xforms/CXform.h"
#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CScalarBitmapIndexProbe::CScalarBitmapIndexProbe
//
//	@doc:
//		Ctor
//		Takes ownership of the index descriptor and the bitmap type id.
//
//---------------------------------------------------------------------------
CScalarBitmapIndexProbe::CScalarBitmapIndexProbe(
	CMemoryPool *mp, gpos::Ref<CIndexDescriptor> pindexdesc,
	gpos::Ref<IMDId> pmdidBitmapType)
	: CScalar(mp),
	  m_pindexdesc(std::move(pindexdesc)),
	  m_pmdidBitmapType(std::move(pmdidBitmapType))
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != m_pindexdesc);
	GPOS_ASSERT(nullptr != m_pmdidBitmapType);
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarBitmapIndexProbe::~CScalarBitmapIndexProbe
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CScalarBitmapIndexProbe::~CScalarBitmapIndexProbe()
{
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarBitmapIndexProbe::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CScalarBitmapIndexProbe::HashValue() const
{
	return gpos::CombineHashes(COperator::HashValue(),
							   m_pindexdesc->MDId()->HashValue());
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarBitmapIndexProbe::Matches
//
//	@doc:
//		Match this operator with the given one.
//
//---------------------------------------------------------------------------
BOOL
CScalarBitmapIndexProbe::Matches(COperator *pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}
	CScalarBitmapIndexProbe *popIndexProbe =
		gpos::dyn_cast<CScalarBitmapIndexProbe>(pop);

	return m_pindexdesc->MDId()->Equals(popIndexProbe->Pindexdesc()->MDId());
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarBitmapIndexProbe::OsPrint
//
//	@doc:
//		Debug print of this operator
//
//---------------------------------------------------------------------------
IOstream &
CScalarBitmapIndexProbe::OsPrint(IOstream &os) const
{
	os << SzId() << " ";
	// index name
	os << "  Bitmap Index Name: (";
	m_pindexdesc->Name().OsPrint(os);
	os << ")";

	return os;
}

// EOF
