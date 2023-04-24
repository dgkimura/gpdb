//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.

#include "gpopt/base/CColConstraintsHashMapper.h"

#include "gpos/common/CAutoRef.h"

using namespace gpopt;

gpos::Ref<CConstraintArray>
CColConstraintsHashMapper::PdrgPcnstrLookup(gpos::Ref<CColRef> colref)
{
	gpos::Ref<CConstraintArray> pdrgpcnstrCol = m_phmColConstr->Find(colref);

	return pdrgpcnstrCol;
}

// mapping between columns and single column constraints in array of constraints
static gpos::Ref<ColRefToConstraintArrayMap>
PhmcolconstrSingleColConstr(CMemoryPool *mp,
							const gpos::Ref<CConstraintArray> drgPcnstr)
{
	gpos::Ref<ColRefToConstraintArrayMap> phmcolconstr =
		GPOS_NEW(mp) ColRefToConstraintArrayMap(mp);

	const ULONG length = drgPcnstr->Size();

	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::Ref<CConstraint> pcnstrChild = (*drgPcnstr)[ul];
		gpos::Ref<CColRefSet> pcrs = pcnstrChild->PcrsUsed();

		if (1 == pcrs->Size())
		{
			gpos::Ref<CColRef> colref = pcrs->PcrFirst();
			gpos::Ref<CConstraintArray> pcnstrMapped =
				phmcolconstr->Find(colref);
			if (nullptr == pcnstrMapped)
			{
				pcnstrMapped = GPOS_NEW(mp) CConstraintArray(mp);
				phmcolconstr->Insert(colref, pcnstrMapped);
			}

			pcnstrMapped->Append(pcnstrChild);
		}
	}

	return phmcolconstr;
}

CColConstraintsHashMapper::CColConstraintsHashMapper(
	CMemoryPool *mp, gpos::Ref<CConstraintArray> pdrgpcnstr)
	: m_phmColConstr(PhmcolconstrSingleColConstr(mp, pdrgpcnstr))
{
}

CColConstraintsHashMapper::~CColConstraintsHashMapper()
{
}
