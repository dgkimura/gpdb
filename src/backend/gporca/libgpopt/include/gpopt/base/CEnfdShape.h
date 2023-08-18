//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CEnfdShape.h
//
//	@doc:
//		Enforceable order property
//---------------------------------------------------------------------------
#ifndef GPOPT_CEnfdShape_H
#define GPOPT_CEnfdShape_H

#include "gpos/base.h"

#include "gpopt/base/CEnfdProp.h"
#include "gpopt/base/CShapeSpec.h"


namespace gpopt
{
using namespace gpos;

// prototypes
class CPhysical;


//---------------------------------------------------------------------------
//	@class:
//		CEnfdShape
//
//	@doc:
//		Enforceable order property;
//
//---------------------------------------------------------------------------
class CEnfdShape : public CEnfdProp
{
public:
	// type of order matching function
	enum EShapeMatching
	{
		EomSatisfy = 0,

		EomSentinel
	};

private:
	// required sort order
	CShapeSpec *m_pss;

	// order matching type
	EShapeMatching m_eom;

	// names of order matching types
	static const CHAR *m_szShapeMatching[EomSentinel];

public:
	CEnfdShape(const CEnfdShape &) = delete;

	// ctor
	CEnfdShape(CShapeSpec *pos, EShapeMatching eom);

	// dtor
	~CEnfdShape() override;

	// hash function
	ULONG HashValue() const override;

	// check if the given order specification is compatible with the
	// order specification of this object for the specified matching type
	BOOL FCompatible(CShapeSpec *pos) const;

	// required order accessor
	CShapeSpec *
	PssRequired() const
	{
		return m_pss;
	}

	// get order enforcing type for the given operator
	EPropEnforcingType Epet(CExpressionHandle &exprhdl, CPhysical *popPhysical,
							BOOL fShapeReqd) const;

	// property spec accessor
	CPropSpec *
	Pps() const override
	{
		return m_pss;
	}

	// return matching type
	EShapeMatching
	Eom() const
	{
		return m_eom;
	}

	// matching function
	BOOL
	Matches(CEnfdShape *peo)
	{
		GPOS_ASSERT(nullptr != peo);

		return m_eom == peo->Eom() && m_pss->Matches(peo->PssRequired());
	}

	// print function
	IOstream &OsPrint(IOstream &os) const override;

};	// class CEnfdShape

}  // namespace gpopt


#endif	// !GPOPT_CEnfdShape_H

// EOF
