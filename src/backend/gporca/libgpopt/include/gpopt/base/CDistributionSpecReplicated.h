//	Greenplum Database
//	Copyright (C) 2020 VMware Inc.

#ifndef GPOPT_CDistributionSpecReplicated_H
#define GPOPT_CDistributionSpecReplicated_H

#include "gpos/base.h"

#include "gpopt/base/CDistributionSpec.h"

namespace gpopt
{

	// derive-only: unsafe computation over replicated data
	class CDistributionSpecReplicated : public CDistributionSpec
	{
		private:

			// replicated support
			CDistributionSpec::EDistributionType m_replicated;

		public:
			// ctor
			CDistributionSpecReplicated(CDistributionSpec::EDistributionType replicated_type)
				: m_replicated(replicated_type)
			{
				GPOS_ASSERT(replicated_type == CDistributionSpec::EdtReplicated ||
							replicated_type == CDistributionSpec::EdtTaintedReplicated ||
							replicated_type == CDistributionSpec::EdtStrictReplicated);
			}

			// accessor
			virtual
			EDistributionType Edt() const
			{
				return m_replicated;
			}

			// should never be called on a required-only distribution
			virtual
			BOOL FSatisfies(const CDistributionSpec *pds) const;

			// should never be called on a derive-only distribution
			virtual
			void AppendEnforcers(gpos::CMemoryPool *mp, CExpressionHandle &exprhdl, CReqdPropPlan *prpp, CExpressionArray *pdrgpexpr, CExpression *pexpr);

			// return distribution partitioning type
			virtual
			EDistributionPartitioningType Edpt() const
			{
				return EdptNonPartitioned;
			}

			// print
			virtual
			IOstream &OsPrint(IOstream &os) const
			{
				switch (Edt())
				{
					case CDistributionSpec::EdtReplicated:
						os << "REPLICATED";
						break;
					case CDistributionSpec::EdtTaintedReplicated:
						os << "TAINTED REPLICATED";
						break;
					case CDistributionSpec::EdtStrictReplicated:
						os << "STRICT REPLICATED";
						break;
					default:
						GPOS_ASSERT(!"Replicated type must be General, Tainted, or Strict");
				}
				return os;
			}

			// conversion function
			static
			CDistributionSpecReplicated *PdsConvert
				(
				CDistributionSpec *pds
				)
			{
				GPOS_ASSERT(NULL != pds);
				GPOS_ASSERT(EdtStrictReplicated == pds->Edt() ||
							EdtReplicated == pds->Edt() ||
							EdtTaintedReplicated == pds->Edt());

				return dynamic_cast<CDistributionSpecReplicated*>(pds);
			}
	};
	// class CDistributionSpecReplicated

}

#endif // !GPOPT_CDistributionSpecReplicated_H
