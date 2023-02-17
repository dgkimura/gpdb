//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDynamicIndexGet.h
//
//	@doc:
//		Dynamic index get operator for partitioned tables
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalDynamicIndexGet_H
#define GPOPT_CLogicalDynamicIndexGet_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/metadata/CIndexDescriptor.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/operators/CLogicalDynamicGetBase.h"


namespace gpopt
{
// fwd declarations
class CPartConstraint;

//---------------------------------------------------------------------------
//	@class:
//		CLogicalDynamicIndexGet
//
//	@doc:
//		Dynamic index accessor for partitioned tables
//
//---------------------------------------------------------------------------
class CLogicalDynamicIndexGet : public CLogicalDynamicGetBase
{
private:
	// index descriptor
	gpos::Ref<CIndexDescriptor> m_pindexdesc;

	// origin operator id -- gpos::ulong_max if operator was not generated via a transformation
	ULONG m_ulOriginOpId;

	// order spec
	gpos::Ref<COrderSpec> m_pos;

public:
	CLogicalDynamicIndexGet(const CLogicalDynamicIndexGet &) = delete;

	// ctors
	explicit CLogicalDynamicIndexGet(CMemoryPool *mp);

	CLogicalDynamicIndexGet(CMemoryPool *mp, const gpos::Ref<IMDIndex> pmdindex,
							gpos::Ref<CTableDescriptor> ptabdesc,
							ULONG ulOriginOpId, const CName *pnameAlias,
							ULONG ulPartIndex,
							gpos::Ref<CColRefArray> pdrgpcrOutput,
							gpos::Ref<CColRef2dArray> pdrgpdrgpcrPart,
							gpos::Ref<IMdIdArray> partition_mdids);

	// dtor
	~CLogicalDynamicIndexGet() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalDynamicIndexGet;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalDynamicIndexGet";
	}

	// origin operator id -- gpos::ulong_max if operator was not generated via a transformation
	ULONG
	UlOriginOpId() const
	{
		return m_ulOriginOpId;
	}

	// index name
	const CName &
	Name() const override
	{
		return m_pindexdesc->Name();
	}

	// table alias name
	const CName &
	NameAlias() const
	{
		return *m_pnameAlias;
	}

	// index descriptor
	gpos::Ref<CIndexDescriptor>
	Pindexdesc() const
	{
		return m_pindexdesc.get();
	}

	// order spec
	COrderSpec *
	Pos() const
	{
		return m_pos.get();
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::Ref<COperator> pop) const override;

	// derive outer references
	gpos::Ref<CColRefSet> DeriveOuterReferences(
		CMemoryPool *mp, CExpressionHandle &exprhdl) override;

	// sensitivity to order of inputs
	BOOL FInputOrderSensitive() const override;

	// return a copy of the operator with remapped columns
	gpos::Ref<COperator> PopCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::Ref<UlongToColRefMap> colref_mapping,
		BOOL must_exist) override;

	//-------------------------------------------------------------------------------------
	// Required Relational Properties
	//-------------------------------------------------------------------------------------

	// compute required stat columns of the n-th child
	gpos::Ref<CColRefSet>
	PcrsStat(CMemoryPool *,			 //mp
			 CExpressionHandle &,	 // exprhdl
			 gpos::Ref<CColRefSet>,	 //pcrsInput
			 ULONG					 // child_index
	) const override
	{
		GPOS_ASSERT(!"CLogicalDynamicIndexGet has no children");
		return nullptr;
	}

	// derive statistics
	gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		IStatisticsArray *stats_ctxt) const override;

	// stat promise
	EStatPromise
	Esp(CExpressionHandle &) const override
	{
		return CLogical::EspHigh;
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::Ref<CXformSet> PxfsCandidates(CMemoryPool *mp) const override;

	//-------------------------------------------------------------------------------------
	// conversion function
	//-------------------------------------------------------------------------------------

	static gpos::Ref<CLogicalDynamicIndexGet>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalDynamicIndexGet == pop->Eopid());

		return gpos::dyn_cast<CLogicalDynamicIndexGet>(pop);
	}


	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CLogicalDynamicIndexGet

}  // namespace gpopt

#endif	// !GPOPT_CLogicalDynamicIndexGet_H

// EOF
