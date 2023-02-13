//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDynamicGet.h
//
//	@doc:
//		Dynamic table accessor for partitioned tables
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalDynamicGet_H
#define GPOPT_CLogicalDynamicGet_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogicalDynamicGetBase.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalDynamicGet
//
//	@doc:
//		Dynamic table accessor
//
//---------------------------------------------------------------------------
class CLogicalDynamicGet : public CLogicalDynamicGetBase
{
protected:
	// Disjunction of selected child partition's constraints after static pruning
	gpos::Ref<CConstraint> m_partition_cnstrs_disj{nullptr};

	// Has done static pruning
	BOOL m_static_pruned{false};

public:
	CLogicalDynamicGet(const CLogicalDynamicGet &) = delete;

	// ctors
	explicit CLogicalDynamicGet(CMemoryPool *mp);

	CLogicalDynamicGet(CMemoryPool *mp, const CName *pnameAlias,
					   gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulPartIndex,
					   gpos::Ref<CColRefArray> pdrgpcrOutput,
					   gpos::Ref<CColRef2dArray> pdrgpdrgpcrPart,
					   gpos::Ref<IMdIdArray> partition_mdids,
					   gpos::Ref<CConstraint> partition_cnstrs_disj,
					   BOOL static_pruned);

	CLogicalDynamicGet(CMemoryPool *mp, const CName *pnameAlias,
					   gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulPartIndex,
					   gpos::Ref<IMdIdArray> partition_mdids);

	// dtor
	~CLogicalDynamicGet() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalDynamicGet;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalDynamicGet";
	}

	// return disjunctive constraint of selected partitions
	gpos::Ref<CConstraint>
	GetPartitionConstraintsDisj() const
	{
		return m_partition_cnstrs_disj.get();
	}

	// return whether static pruning is performed
	BOOL
	FStaticPruned() const
	{
		return m_static_pruned;
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::Ref<COperator> pop) const override;

	// sensitivity to order of inputs
	BOOL FInputOrderSensitive() const override;

	// return a copy of the operator with remapped columns
	gpos::Ref<COperator> PopCopyWithRemappedColumns(
		CMemoryPool *mp, UlongToColRefMap *colref_mapping,
		BOOL must_exist) override;

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------


	// derive join depth
	ULONG
	DeriveJoinDepth(CMemoryPool *,		 // mp
					CExpressionHandle &	 // exprhdl
	) const override
	{
		return 1;
	}

	// derive table descriptor
	gpos::Ref<CTableDescriptor>
	DeriveTableDescriptor(CMemoryPool *,	   // mp
						  CExpressionHandle &  // exprhdl
	) const override
	{
		return m_ptabdesc.get();
	}

	// derive max card
	CMaxCard DeriveMaxCard(CMemoryPool *mp,
						   CExpressionHandle &exprhdl) const override;


	//-------------------------------------------------------------------------------------
	// Required Relational Properties
	//-------------------------------------------------------------------------------------

	// compute required stat columns of the n-th child
	gpos::Ref<CColRefSet>
	PcrsStat(CMemoryPool *,			 // mp,
			 CExpressionHandle &,	 // exprhdl
			 gpos::Ref<CColRefSet>,	 //pcrsInput
			 ULONG					 // child_index
	) const override
	{
		GPOS_ASSERT(!"CLogicalDynamicGet has no children");
		return nullptr;
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::Ref<CXformSet> PxfsCandidates(CMemoryPool *mp) const override;

	//-------------------------------------------------------------------------------------
	// Statistics
	//-------------------------------------------------------------------------------------

	// derive statistics
	gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		IStatisticsArray *stats_ctxt) const override;

	// derive stats from base table using filters on partition and/or index columns
	gpos::Ref<IStatistics> PstatsDeriveFilter(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CExpression> pexprFilter) const;

	// stat promise
	EStatPromise
	Esp(CExpressionHandle &) const override
	{
		return CLogical::EspHigh;
	}

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::Ref<CLogicalDynamicGet>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalDynamicGet == pop->Eopid());

		return gpos::dyn_cast<CLogicalDynamicGet>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CLogicalDynamicGet

}  // namespace gpopt


#endif	// !GPOPT_CLogicalDynamicGet_H

// EOF
