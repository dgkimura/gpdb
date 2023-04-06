//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CLogicalDynamicGetBase.h
//
//	@doc:
//		Base class for dynamic table accessors for partitioned tables
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalDynamicGetBase_H
#define GPOPT_CLogicalDynamicGetBase_H

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalDynamicGetBase
//
//	@doc:
//		Dynamic table accessor base class
//
//---------------------------------------------------------------------------
class CLogicalDynamicGetBase : public CLogical
{
protected:
	// alias for table
	const CName *m_pnameAlias;

	// table descriptor
	gpos::Ref<CTableDescriptor> m_ptabdesc;

	// dynamic scan id
	ULONG m_scan_id;

	// output columns
	CColRefArray *m_pdrgpcrOutput;

	// partition keys
	CColRef2dArray *m_pdrgpdrgpcrPart;

	// distribution columns (empty for master only tables)
	gpos::Ref<CColRefSet> m_pcrsDist;

	// private copy ctor
	CLogicalDynamicGetBase(const CLogicalDynamicGetBase &);

	// given a colrefset from a table, get colids and attno
	void ExtractColIdsAttno(CMemoryPool *mp, gpos::Ref<CColRefSet> pcrs,
							ULongPtrArray *colids,
							ULongPtrArray *pdrgpulPos) const;

	// Child partitions
	IMdIdArray *m_partition_mdids = nullptr;
	// Map of Root colref -> col index in child tabledesc
	// per child partition in m_partition_mdid
	ColRefToUlongMapArray *m_root_col_mapping_per_part = nullptr;

	// Construct a mapping from each column in root table to an index in each
	// child partition's table descr by matching column names$
	static ColRefToUlongMapArray *ConstructRootColMappingPerPart(
		CMemoryPool *mp, CColRefArray *root_cols, IMdIdArray *partition_mdids);

public:
	// ctors
	explicit CLogicalDynamicGetBase(CMemoryPool *mp);

	CLogicalDynamicGetBase(CMemoryPool *mp, const CName *pnameAlias,
						   gpos::Ref<CTableDescriptor> ptabdesc, ULONG scan_id,
						   CColRefArray *pdrgpcrOutput,
						   CColRef2dArray *pdrgpdrgpcrPart,
						   IMdIdArray *partition_mdids);

	CLogicalDynamicGetBase(CMemoryPool *mp, const CName *pnameAlias,
						   gpos::Ref<CTableDescriptor> ptabdesc, ULONG scan_id,
						   IMdIdArray *partition_mdids);

	// dtor
	~CLogicalDynamicGetBase() override;

	// accessors
	virtual CColRefArray *
	PdrgpcrOutput() const
	{
		return m_pdrgpcrOutput;
	}

	// return table's name
	virtual const CName &
	Name() const
	{
		return *m_pnameAlias;
	}

	// distribution columns
	virtual const gpos::Ref<CColRefSet>
	PcrsDist() const
	{
		return m_pcrsDist;
	}

	// return table's descriptor
	virtual gpos::Ref<CTableDescriptor>
	Ptabdesc() const
	{
		return m_ptabdesc;
	}

	// return scan id
	virtual ULONG
	ScanId() const
	{
		return m_scan_id;
	}

	// return the partition columns
	virtual CColRef2dArray *
	PdrgpdrgpcrPart() const
	{
		return m_pdrgpdrgpcrPart;
	}

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::Ref<CColRefSet> DeriveOutputColumns(CMemoryPool *,
											  CExpressionHandle &) override;

	// derive keys
	gpos::Ref<CKeyCollection> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive partition consumer info
	gpos::Ref<CPartInfo> DerivePartitionInfo(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive constraint property
	gpos::Ref<CPropConstraint> DerivePropertyConstraint(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

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
		return m_ptabdesc;
	}

	IMdIdArray *
	GetPartitionMdids() const
	{
		return m_partition_mdids;
	}

	ColRefToUlongMapArray *
	GetRootColMappingPerPart() const
	{
		return m_root_col_mapping_per_part;
	}
};	// class CLogicalDynamicGetBase

}  // namespace gpopt


#endif	// !GPOPT_CLogicalDynamicGetBase_H

// EOF
