//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CEnfdShape.cpp
//
//	@doc:
//		Implementation of enforceable order property
//---------------------------------------------------------------------------

#include "gpopt/base/CEnfdShape.h"

#include "gpos/base.h"

#include "gpopt/base/CReqdPropPlan.h"
#include "gpopt/operators/CPhysicalSort.h"


using namespace gpopt;


// initialization of static variables
const CHAR *CEnfdShape::m_szShapeMatching[EomSentinel] = {"satisfy"};


//---------------------------------------------------------------------------
//	@function:
//		CEnfdShape::CEnfdShape
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CEnfdShape::CEnfdShape(CShapeSpec *pos, EShapeMatching eom)
	: m_pss(pos), m_eom(eom)
{
	GPOS_ASSERT(nullptr != pos);
	GPOS_ASSERT(EomSentinel > eom);
}


//---------------------------------------------------------------------------
//	@function:
//		CEnfdShape::~CEnfdShape
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CEnfdShape::~CEnfdShape()
{
	CRefCount::SafeRelease(m_pss);
}


//---------------------------------------------------------------------------
//	@function:
//		CEnfdShape::FCompatible
//
//	@doc:
//		Check if the given order specification is compatible with the
//		order specification of this object for the specified matching type
//
//---------------------------------------------------------------------------
BOOL
CEnfdShape::FCompatible(CShapeSpec *pos) const
{
	GPOS_ASSERT(nullptr != pos);

	switch (m_eom)
	{
		case EomSatisfy:
			return pos->FSatisfies(m_pss);

		case EomSentinel:
			GPOS_ASSERT("invalid matching type");
	}

	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CEnfdShape::HashValue
//
//	@doc:
// 		Hash function
//
//---------------------------------------------------------------------------
ULONG
CEnfdShape::HashValue() const
{
	return gpos::CombineHashes(m_eom + 1, m_pss->HashValue());
}


//---------------------------------------------------------------------------
//	@function:
//		CEnfdShape::Epet
//
//	@doc:
// 		Get order enforcing type for the given operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CEnfdShape::Epet(CExpressionHandle &exprhdl, CPhysical *popPhysical,
				 BOOL fShapeReqd) const
{
	if (fShapeReqd)
	{
		return popPhysical->EpetShape(exprhdl, this);
	}
	return EpetUnnecessary;
}


//---------------------------------------------------------------------------
//	@function:
//		CEnfdShape::OsPrint
//
//	@doc:
//		Print function
//
//---------------------------------------------------------------------------
IOstream &
CEnfdShape::OsPrint(IOstream &os) const
{
	return os << (*m_pss) << " match: " << m_szShapeMatching[m_eom] << " ";
}


// EOF
