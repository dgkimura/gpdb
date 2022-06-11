//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CScalarBitmapIndexProbe.h
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

#ifndef GPOPT_CScalarBitmapIndexProbe_H
#define GPOPT_CScalarBitmapIndexProbe_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CScalar.h"

namespace gpopt
{
// fwd declarations
class CIndexDescriptor;

//---------------------------------------------------------------------------
//	@class:
//		CScalarBitmapIndexProbe
//
//	@doc:
//		Bitmap index probe scalar operator
//
//---------------------------------------------------------------------------
class CScalarBitmapIndexProbe : public CScalar
{
private:
	// index descriptor
	gpos::owner<CIndexDescriptor *> m_pindexdesc;

	// bitmap type id
	gpos::owner<IMDId *> m_pmdidBitmapType;

	// private copy ctor
	CScalarBitmapIndexProbe(const CScalarBitmapIndexProbe &);

public:
	// ctor
	CScalarBitmapIndexProbe(CMemoryPool *mp,
							gpos::owner<CIndexDescriptor *> pindexdesc,
							gpos::owner<IMDId *> pmdidBitmapType);

	// ctor
	// only for transforms
	explicit CScalarBitmapIndexProbe(CMemoryPool *mp);

	// dtor
	~CScalarBitmapIndexProbe() override;

	// index descriptor
	gpos::pointer<CIndexDescriptor *>
	Pindexdesc() const
	{
		return m_pindexdesc;
	}

	// bitmap type id
	gpos::pointer<IMDId *>
	MdidType() const override
	{
		return m_pmdidBitmapType;
	}

	// identifier
	EOperatorId
	Eopid() const override
	{
		return EopScalarBitmapIndexProbe;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CScalarBitmapIndexProbe";
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return false;
	}

	// return a copy of the operator with remapped columns
	gpos::owner<COperator *>
	PopCopyWithRemappedColumns(
		CMemoryPool *,						//mp,
		gpos::pointer<UlongToColRefMap *>,	//colref_mapping,
		BOOL								//must_exist
		) override
	{
		return PopCopyDefault();
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

	// conversion
	static gpos::cast_func<CScalarBitmapIndexProbe *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopScalarBitmapIndexProbe == pop->Eopid());

		return dynamic_cast<CScalarBitmapIndexProbe *>(pop);
	}

};	// class CScalarBitmapIndexProbe
}  // namespace gpopt

#endif	// !GPOPT_CScalarBitmapIndexProbe_H

// EOF
