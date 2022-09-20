//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CIndexDescriptor.h
//
//	@doc:
//		Base class for index descriptor
//---------------------------------------------------------------------------
#ifndef GPOPT_CIndexDescriptor_H
#define GPOPT_CIndexDescriptor_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/metadata/CColumnDescriptor.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "naucrates/md/IMDId.h"
#include "naucrates/md/IMDIndex.h"

namespace gpopt
{
using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@class:
//		CIndexDescriptor
//
//	@doc:
//		Base class for index descriptor
//
//---------------------------------------------------------------------------
class CIndexDescriptor : public CRefCount,
						 public DbgPrintMixin<CIndexDescriptor>
{
private:
	// mdid of the index
	gpos::Ref<IMDId> m_pmdidIndex;

	// name of index
	CName m_name;

	// array of index key columns
	gpos::Ref<CColumnDescriptorArray> m_pdrgpcoldescKeyCols;

	// array of index included columns
	gpos::Ref<CColumnDescriptorArray> m_pdrgpcoldescIncludedCols;

	// clustered index
	BOOL m_clustered;

	// index type
	IMDIndex::EmdindexType m_index_type;

public:
	CIndexDescriptor(const CIndexDescriptor &) = delete;

	// ctor
	CIndexDescriptor(CMemoryPool *mp, gpos::Ref<IMDId> pmdidIndex,
					 const CName &name,
					 gpos::Ref<CColumnDescriptorArray> pdrgcoldescKeyCols,
					 gpos::Ref<CColumnDescriptorArray> pdrgcoldescIncludedCols,
					 BOOL is_clustered, IMDIndex::EmdindexType emdindt);

	// dtor
	~CIndexDescriptor() override;

	// number of key columns
	ULONG Keys() const;

	// number of included columns
	ULONG UlIncludedColumns() const;

	// index mdid accessor
	IMDId *
	MDId() const
	{
		return m_pmdidIndex.get();
	}

	// index name
	const CName &
	Name() const
	{
		return m_name;
	}

	// key column descriptors
	CColumnDescriptorArray *
	PdrgpcoldescKey() const
	{
		return m_pdrgpcoldescKeyCols.get();
	}

	// included column descriptors
	CColumnDescriptorArray *
	PdrgpcoldescIncluded() const
	{
		return m_pdrgpcoldescIncludedCols.get();
	}

	// is index clustered
	BOOL
	IsClustered() const
	{
		return m_clustered;
	}

	IMDIndex::EmdindexType
	IndexType() const
	{
		return m_index_type;
	}

	BOOL SupportsIndexOnlyScan() const;

	// create an index descriptor
	static gpos::Ref<CIndexDescriptor> Pindexdesc(
		CMemoryPool *mp, const CTableDescriptor *ptabdesc,
		const IMDIndex *pmdindex);

	IOstream &OsPrint(IOstream &os) const;

};	// class CIndexDescriptor
}  // namespace gpopt

#endif	// !GPOPT_CIndexDescriptor_H

// EOF
