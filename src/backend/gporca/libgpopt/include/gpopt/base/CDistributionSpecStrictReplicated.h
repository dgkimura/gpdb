//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CDistributionSpecStrictReplicated.h
//
//	@doc:
//		Description of a replicated distribution; 
//		Can be used as required or derived property;
//---------------------------------------------------------------------------
#ifndef GPOPT_CDistributionSpecStrictReplicated_H
#define GPOPT_CDistributionSpecStrictReplicated_H

#include "gpos/base.h"

#include "gpopt/base/CDistributionSpec.h"

namespace gpopt
{
	using namespace gpos;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDistributionSpecStrictReplicated
	//
	//	@doc:
	//		Class for representing replicated distribution specification.
	//
	//---------------------------------------------------------------------------
	class CDistributionSpecStrictReplicated : public CDistributionSpec
	{
		private:

			// private copy ctor
			CDistributionSpecStrictReplicated(const CDistributionSpecStrictReplicated &);
			
		public:
			// ctor
			CDistributionSpecStrictReplicated()
			{}
			
			// accessor
			virtual 
			EDistributionType Edt() const
			{
				return CDistributionSpec::EdtReplicated;
			}
			
			// does this distribution satisfy the given one
			virtual 
			BOOL FSatisfies(const CDistributionSpec *pds) const;
			
			// append enforcers to dynamic array for the given plan properties
			virtual
			void AppendEnforcers(CMemoryPool *mp, CExpressionHandle &exprhdl, CReqdPropPlan *prpp, CExpressionArray *pdrgpexpr, CExpression *pexpr);

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
				return os << "REPLICATED ";
			}
			
			// conversion function
			static
			CDistributionSpecStrictReplicated *PdsConvert
				(
				CDistributionSpec *pds
				)
			{
				GPOS_ASSERT(NULL != pds);
				GPOS_ASSERT(EdtReplicated == pds->Edt());

				return dynamic_cast<CDistributionSpecStrictReplicated*>(pds);
			}

	}; // class CDistributionSpecStrictReplicated

}

#endif // !GPOPT_CDistributionSpecStrictReplicated_H

// EOF
