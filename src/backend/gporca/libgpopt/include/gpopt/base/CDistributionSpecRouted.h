//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDistributionSpecRouted.h
//
//	@doc:
//		Description of a routed distribution;
//		Can be used as required or derived property;
//---------------------------------------------------------------------------
#ifndef GPOPT_CDistributionSpecRouted_H
#define GPOPT_CDistributionSpecRouted_H

#include "gpos/base.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CDistributionSpec.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CDistributionSpecRouted
//
//	@doc:
//		Class for representing routed distribution specification.
//
//---------------------------------------------------------------------------
class CDistributionSpecRouted : public CDistributionSpec
{
private:
	// segment id column
	gpos::Ref<CColRef> m_pcrSegmentId;

public:
	CDistributionSpecRouted(const CDistributionSpecRouted &) = delete;

	// ctor
	explicit CDistributionSpecRouted(gpos::Ref<CColRef> pcrSegmentId);

	// dtor
	~CDistributionSpecRouted() override;

	// distribution type accessor
	EDistributionType
	Edt() const override
	{
		return CDistributionSpec::EdtRouted;
	}

	// segment id column accessor
	gpos::Ref<CColRef>
	Pcr() const
	{
		return m_pcrSegmentId;
	}

	// does this distribution satisfy the given one
	BOOL Matches(const gpos::Ref<CDistributionSpec> pds) const override;

	// does this distribution satisfy the given one
	BOOL FSatisfies(const gpos::Ref<CDistributionSpec> pds) const override;

	// return a copy of the distribution spec with remapped columns
	gpos::Ref<CDistributionSpec> PdsCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::Ref<UlongToColRefMap> colref_mapping,
		BOOL must_exist) override;

	// append enforcers to dynamic array for the given plan properties
	void AppendEnforcers(CMemoryPool *mp, CExpressionHandle &exprhdl,
						 gpos::Ref<CReqdPropPlan> prpp,
						 gpos::Ref<CExpressionArray> pdrgpexpr,
						 gpos::Ref<CExpression> pexpr) override;

	// hash function for routed distribution spec
	ULONG HashValue() const override;

	// extract columns used by the distribution spec
	gpos::Ref<CColRefSet> PcrsUsed(CMemoryPool *mp) const override;

	// return distribution partitioning type
	EDistributionPartitioningType
	Edpt() const override
	{
		return EdptPartitioned;
	}

	// print
	IOstream &OsPrint(IOstream &os) const override;

	// conversion function
	static gpos::Ref<CDistributionSpecRouted>
	PdsConvert(gpos::Ref<CDistributionSpec> pds)
	{
		GPOS_ASSERT(nullptr != pds);
		GPOS_ASSERT(EdtRouted == pds->Edt());

		return gpos::dyn_cast<CDistributionSpecRouted>(pds);
	}

};	// class CDistributionSpecRouted

}  // namespace gpopt

#endif	// !GPOPT_CDistributionSpecRouted_H

// EOF
