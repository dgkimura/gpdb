//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalCTEConsumer.h
//
//	@doc:
//		Logical CTE consumer operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalCTEConsumer_H
#define GPOPT_CLogicalCTEConsumer_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalCTEConsumer
//
//	@doc:
//		CTE consumer operator
//
//---------------------------------------------------------------------------
class CLogicalCTEConsumer : public CLogical
{
private:
	// cte identifier
	ULONG m_id;

	// mapped cte columns
	gpos::Ref<CColRefArray> m_pdrgpcr;

	// inlined expression
	gpos::Ref<CExpression> m_pexprInlined;

	// map of CTE producer's output column ids to consumer's output columns
	gpos::Ref<UlongToColRefMap> m_phmulcr;

	// output columns
	gpos::Ref<CColRefSet> m_pcrsOutput;

	// create the inlined version of this consumer as well as the column mapping
	void CreateInlinedExpr(CMemoryPool *mp);

public:
	CLogicalCTEConsumer(const CLogicalCTEConsumer &) = delete;

	// ctor
	explicit CLogicalCTEConsumer(CMemoryPool *mp);

	// ctor
	CLogicalCTEConsumer(CMemoryPool *mp, ULONG id,
						gpos::Ref<CColRefArray> colref_array);

	// dtor
	~CLogicalCTEConsumer() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalCTEConsumer;
	}

	const CHAR *
	SzId() const override
	{
		return "CLogicalCTEConsumer";
	}

	// cte identifier
	ULONG
	UlCTEId() const
	{
		return m_id;
	}

	// cte columns
	CColRefArray *
	Pdrgpcr() const
	{
		return m_pdrgpcr.get();
	}

	// column mapping
	UlongToColRefMap *
	Phmulcr() const
	{
		return m_phmulcr.get();
	}

	CExpression *
	PexprInlined() const
	{
		return m_pexprInlined.get();
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(COperator *pop) const override;

	// sensitivity to order of inputs
	BOOL FInputOrderSensitive() const override;

	// return a copy of the operator with remapped columns
	gpos::Ref<COperator> PopCopyWithRemappedColumns(
		CMemoryPool *mp, UlongToColRefMap *colref_mapping,
		BOOL must_exist) override;

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::Ref<CColRefSet> DeriveOutputColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) override;

	// dervive keys
	gpos::Ref<CKeyCollection> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive max card
	CMaxCard DeriveMaxCard(CMemoryPool *mp,
						   CExpressionHandle &exprhdl) const override;

	// derive join depth
	ULONG DeriveJoinDepth(CMemoryPool *mp,
						  CExpressionHandle &exprhdl) const override;

	// derive not nullable output columns
	gpos::Ref<CColRefSet> DeriveNotNullColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive constraint property
	gpos::Ref<CPropConstraint> DerivePropertyConstraint(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive partition consumer info
	gpos::Ref<CPartInfo> DerivePartitionInfo(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive table descriptor
	CTableDescriptor *DeriveTableDescriptor(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// compute required stats columns of the n-th child
	gpos::Ref<CColRefSet>
	PcrsStat(CMemoryPool *,		   // mp
			 CExpressionHandle &,  // exprhdl
			 CColRefSet *,		   //pcrsInput,
			 ULONG				   // child_index
	) const override
	{
		GPOS_ASSERT(!"CLogicalCTEConsumer has no children");
		return nullptr;
	}

	// stat promise
	EStatPromise
	Esp(CExpressionHandle &) const override
	{
		return CLogical::EspHigh;
	}

	// derive statistics
	gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		IStatisticsArray *stats_ctxt) const override;

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::Ref<CXformSet> PxfsCandidates(CMemoryPool *mp) const override;

	//-------------------------------------------------------------------------------------

	// conversion function
	static CLogicalCTEConsumer *
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalCTEConsumer == pop->Eopid());

		return dynamic_cast<CLogicalCTEConsumer *>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CLogicalCTEConsumer

}  // namespace gpopt

#endif	// !GPOPT_CLogicalCTEConsumer_H

// EOF
