//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CLogicalLeftOuterNLJoin.h
//
//	@doc:
//		Left outer join operator
//---------------------------------------------------------------------------
#ifndef GPOS_CLogicalLeftOuterNLJoin_H
#define GPOS_CLogicalLeftOuterNLJoin_H

#include "gpos/base.h"
#include "gpopt/operators/CLogicalJoin.h"

namespace gpopt
{
	//---------------------------------------------------------------------------
	//	@class:
	//		CLogicalLeftOuterNLJoin
	//
	//	@doc:
	//		Left outer join operator
	//
	//---------------------------------------------------------------------------
	class CLogicalLeftOuterNLJoin : public CLogicalJoin
	{
		private:

			// private copy ctor
			CLogicalLeftOuterNLJoin(const CLogicalLeftOuterNLJoin &);

		public:

			// ctor
			explicit
			CLogicalLeftOuterNLJoin(CMemoryPool *mp);

			// dtor
			virtual 
			~CLogicalLeftOuterNLJoin() 
			{}

			// ident accessors
			virtual 
			EOperatorId Eopid() const
			{
				return EopLogicalLeftOuterNLJoin;
			}
			
			// return a string for operator name
			virtual 
			const CHAR *SzId() const
			{
				return "CLogicalLeftOuterNLJoin";
			}

			// return true if we can pull projections up past this operator from its given child
			virtual
			BOOL FCanPullProjectionsUp
				(
				ULONG child_index
				) const
			{
				return (0 == child_index);
			}

			//-------------------------------------------------------------------------------------
			// Derived Relational Properties
			//-------------------------------------------------------------------------------------

			// derive not nullable output columns
			virtual
			CColRefSet *DeriveNotNullColumns
				(
				CMemoryPool *,// mp
				CExpressionHandle &exprhdl
				)
				const
			{
				// left outer join passes through not null columns from outer child only
				return PcrsDeriveNotNullPassThruOuter(exprhdl);
			}

			// derive max card
			virtual
			CMaxCard DeriveMaxCard(CMemoryPool *mp, CExpressionHandle &exprhdl) const;

			// derive constraint property
			virtual
			CPropConstraint *DerivePropertyConstraint
				(
				CMemoryPool *, //mp,
				CExpressionHandle &exprhdl
				)
				const
			{
				return PpcDeriveConstraintPassThru(exprhdl, 0 /*ulChild*/);
			}

			//-------------------------------------------------------------------------------------
			// Transformations
			//-------------------------------------------------------------------------------------

			// candidate set of xforms
			CXformSet *PxfsCandidates(CMemoryPool *mp) const;

			//-------------------------------------------------------------------------------------
			//-------------------------------------------------------------------------------------
			//-------------------------------------------------------------------------------------

			// conversion function
			static
			CLogicalLeftOuterNLJoin *PopConvert
				(
				COperator *pop
				)
			{
				GPOS_ASSERT(NULL != pop);
				GPOS_ASSERT(EopLogicalLeftOuterNLJoin == pop->Eopid());
				
				return dynamic_cast<CLogicalLeftOuterNLJoin*>(pop);
			}

	}; // class CLogicalLeftOuterNLJoin

}


#endif // !GPOS_CLogicalLeftOuterNLJoin_H

// EOF
