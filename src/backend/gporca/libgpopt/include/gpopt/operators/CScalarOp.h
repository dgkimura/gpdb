//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CScalarOp.h
//
//	@doc:
//		Base class for all scalar operations such as arithmetic and string
//		evaluations
//---------------------------------------------------------------------------
#ifndef GPOPT_CScalarOp_H
#define GPOPT_CScalarOp_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDrvdProp.h"
#include "gpopt/operators/CScalar.h"
#include "naucrates/md/IMDId.h"

namespace gpopt
{
using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@class:
//		CScalarOp
//
//	@doc:
//		general scalar operation such as arithmetic and string evaluations
//
//---------------------------------------------------------------------------
class CScalarOp : public CScalar
{
private:
	// metadata id in the catalog
	gpos::owner<IMDId *> m_mdid_op;

	// return type id or NULL if it can be inferred from the metadata
	gpos::owner<IMDId *> m_return_type_mdid;

	// scalar operator name
	const CWStringConst *m_pstrOp;

	// does operator return NULL on NULL input?
	BOOL m_returns_null_on_null_input;

	// is operator return type BOOL?
	BOOL m_fBoolReturnType;

	// is operator commutative
	BOOL m_fCommutative;

	// private copy ctor
	CScalarOp(const CScalarOp &);

public:
	// ctor
	CScalarOp(CMemoryPool *mp, gpos::owner<IMDId *> mdid_op,
			  gpos::owner<IMDId *> return_type_mdid,
			  const CWStringConst *pstrOp);

	// dtor
	~CScalarOp() override
	{
		m_mdid_op->Release();
		CRefCount::SafeRelease(m_return_type_mdid);
		GPOS_DELETE(m_pstrOp);
	}


	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopScalarOp;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CScalarOp";
	}

	// accessor to the return type field
	gpos::pointer<IMDId *> GetReturnTypeMdId() const;

	// the type of the scalar expression
	gpos::pointer<IMDId *> MdidType() const override;

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

	// sensitivity to order of inputs
	BOOL FInputOrderSensitive() const override;

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

	// conversion function
	static gpos::cast_func<CScalarOp *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopScalarOp == pop->Eopid());

		return dynamic_cast<CScalarOp *>(pop);
	}

	// helper function
	static BOOL FCommutative(gpos::pointer<const IMDId *> pcmdidOtherOp);

	// boolean expression evaluation
	EBoolEvalResult Eber(
		gpos::pointer<ULongPtrArray *> pdrgpulChildren) const override;

	// name of the scalar operator
	const CWStringConst *Pstr() const;

	// metadata id
	gpos::pointer<IMDId *> MdIdOp() const;

	// print
	IOstream &OsPrint(IOstream &os) const override;

};	// class CScalarOp

}  // namespace gpopt

#endif	// !GPOPT_CScalarOp_H

// EOF
