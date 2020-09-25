//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CDistributionSpecReplicated.h
//
//	@doc:
//		Description of a replicated distribution;
//		Can be used as required or derived property;
//---------------------------------------------------------------------------
#ifndef GPOPT_CDistributionSpecReplicated_H
#define GPOPT_CDistributionSpecReplicated_H

#include "gpos/base.h"

#include "gpopt/base/CDistributionSpec.h"

namespace gpopt
{
using namespace gpos;

// derive-only: unsafe computation over replicated data
class CDistributionSpecReplicated : public CDistributionSpec
{
public:
	// ctor
	CDistributionSpecReplicated() = default

	// accessor
	virtual EDistributionType
	Edt() const
	{
		return CDistributionSpec::EdtGeneralReplicated;
	}

	// should never be called on a required-only distribution
	virtual BOOL FSatisfies(const CDistributionSpec *pds) const;

	// should never be called on a derive-only distribution
	virtual void AppendEnforcers(CMemoryPool *mp, CExpressionHandle &exprhdl,
								 CReqdPropPlan *prpp,
								 CExpressionArray *pdrgpexpr,
								 CExpression *pexpr);

	// return distribution partitioning type
	virtual EDistributionPartitioningType
	Edpt() const
	{
		return EdptNonPartitioned;
	}

	// print
	virtual IOstream &
	OsPrint(IOstream &os) const
	{
		return os << "GENERAL REPLICATED";
	}

	// conversion function
	static CDistributionSpecReplicated *
	PdsConvert(CDistributionSpec *pds)
	{
		GPOS_ASSERT(NULL != pds);
		GPOS_ASSERT(EdtGeneralReplicated == pds->Edt());

		return dynamic_cast<CDistributionSpecReplicated *>(pds);
	}

};	// class CDistributionSpecReplicated

}  // namespace gpopt

#endif	// !GPOPT_CDistributionSpecReplicated_H

// EOF
