//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (c) 2023 VMware, Inc. or its affiliates. All Rights Reserved.
//
//	@filename:
//		CScanHint.h
//
//	@doc:
//		CScanHint represents an ORCA optimizer hint that favors a specific scan
//		type (e.g. index scan, bitmap scan, seq scan) on a specific relation
//		(identified by table name or alias) in a query.
//---------------------------------------------------------------------------
#ifndef GPOS_CScanHint_H
#define GPOS_CScanHint_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/CRefCount.h"

#include "gpopt/hints/IHint.h"
#include "gpopt/operators/COperator.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

namespace gpopt
{
class CScanHint : public IHint, public DbgPrintMixin<CScanHint>
{
private:
	CMemoryPool *m_mp;

	// relation or alias used in the query
	const CWStringBase *m_name;

	// specfied index names (if any) for index and bitmap scans
	StringPtrArray *m_indexnames;

	// scan operator to use
	COperator::EOperatorId m_id;

	// is this a No* hint (e.g. NoSeqScan)?
	BOOL m_isnot;

public:
	CScanHint(CMemoryPool *mp, const CWStringBase *name,
			  StringPtrArray *indexnames, COperator::EOperatorId id, BOOL isnot)
		: m_mp(mp),
		  m_name(name),
		  m_indexnames(indexnames),
		  m_id(id),
		  m_isnot(isnot)
	{
	}

	~CScanHint() override
	{
		GPOS_DELETE(m_name);
		m_indexnames->Release();
	}

	const CWStringBase *
	GetName() const
	{
		return m_name;
	}

	const StringPtrArray *
	GetIndexNames() const
	{
		return m_indexnames;
	}

	COperator::EOperatorId
	GetOperator() const
	{
		return m_id;
	}

	BOOL
	GetIsNot() const
	{
		return m_isnot;
	}

	IOstream &OsPrint(IOstream &os) const;

	void Serialize(CXMLSerializer *xml_serializer) const;
};

using ScanHintList = CDynamicPtrArray<CScanHint, CleanupRelease>;

}  // namespace gpopt

#endif	// !GPOS_CScanHint_H

// EOF
