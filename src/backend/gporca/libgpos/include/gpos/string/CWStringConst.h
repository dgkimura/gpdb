//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CWStringConst.h
//
//	@doc:
//		Constant string class
//---------------------------------------------------------------------------
#ifndef GPOS_CWStringConst_H
#define GPOS_CWStringConst_H

#include <cassert>
#include "gpos/string/CWStringBase.h"

namespace gpos
{
//---------------------------------------------------------------------------
//	@class:
//		CWStringConst
//
//	@doc:
//		Constant string class.
//		The class represents constant strings, which cannot be modified after creation.
//		The class can either own its own memory, or be supplied with an external
//		memory buffer holding the string characters.
//		For a general string class that can be modified, see CWString.
//
//---------------------------------------------------------------------------
class CWStringConst : public CWStringBase
{
private:
	// null terminated wide character buffer
	const WCHAR *m_w_str_buffer;

public:
	// ctors
	CWStringConst(const WCHAR *w_str_buffer);
	CWStringConst(CMemoryPool *mp, const WCHAR *w_str_buffer);
	CWStringConst(CMemoryPool *mp, const CHAR *str_buffer);

	CWStringConst(const CWStringConst &&a) : CWStringBase(std::move(a)), m_w_str_buffer(a.m_w_str_buffer) {
		assert(a.m_length == m_length);
	}
	CWStringConst& operator=(CWStringConst&& other) {
		m_w_str_buffer = other.m_w_str_buffer;
		m_length = other.m_length;
		m_owns_memory = other.m_owns_memory;
		other.m_owns_memory = false;
		return *this;
	}

	// shallow copy ctor
	//CWStringConst(const CWStringConst &);

	//dtor
	~CWStringConst() override;

	// returns the wide character buffer storing the string
	const WCHAR *GetBuffer() const override;

	// equality
	static BOOL Equals(const CWStringConst *string1,
					   const CWStringConst *string2);

	// hash function
	static ULONG HashValue(const CWStringConst *string);

	// checks whether the string is byte-wise equal to another string
	BOOL Equals(const CWStringBase *str) const override;
};
}  // namespace gpos

#endif	// #ifndef GPOS_CWStringConst_H

// EOF
