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
	enum class EReplicatedType
	{
		ErtStrict,
		ErtTainted,
		ErtGeneral,
		ErtSentinel
	};

private:
	// replicated support
	EReplicatedType m_replicated;

public:
	// ctor
	CDistributionSpecReplicated(EReplicatedType replicated_type)
		: m_replicated(replicated_type)
	{
	}

	// accessor
	virtual EDistributionType
	Edt() const
	{
		switch (m_replicated)
		{
			case EReplicatedType::ErtGeneral:
				return CDistributionSpec::EdtGeneralReplicated;
			case EReplicatedType::ErtTainted:
				return CDistributionSpec::EdtTaintedReplicated;
			case EReplicatedType::ErtStrict:
				return CDistributionSpec::EdtReplicated;
			default:
				GPOS_ASSERT(!"Replicated type must be General, Tainted, or Strict");
				return CDistributionSpec::EdtSentinel;
		}
	}

	// should never be called on a required-only distribution
	virtual BOOL FSatisfies(const CDistributionSpec *pds) const;

	EReplicatedType Ert() const
	{
		return m_replicated;
	}

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
		GPOS_ASSERT(EdtReplicated == pds->Edt());

		return dynamic_cast<CDistributionSpecReplicated *>(pds);
	}

};	// class CDistributionSpecReplicated

}  // namespace gpopt

#endif	// !GPOPT_CDistributionSpecReplicated_H

// EOF
