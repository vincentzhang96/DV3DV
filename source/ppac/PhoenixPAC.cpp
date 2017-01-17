#include "PhoenixPAC.h"
#include <handleapi.h>

using namespace PPAC;

TPUID::TPUID()
{
	tpuid = 0;
}

TPUID::TPUID(uint64 tpu)
{
	tpuid = tpu;
}

TPUID::TPUID(const TPUID& tpu)
{
	tpuid = tpu.tpuid;
}

TPUID::TPUID(uint16 type, uint16 purpose, uint32 unique)
{
	t = type;
	p = purpose;
	u = unique;
}

TPUID TPUID::operator+(const TPUID& rhs) const
{
	auto ret = *this;
	ret.t += rhs.t;
	ret.p += rhs.p;
	ret.u += rhs.u;
	return ret;
}

TPUID TPUID::operator-(const TPUID& rhs) const
{
	auto ret = *this;
	ret.t -= rhs.t;
	ret.p -= rhs.p;
	ret.u -= rhs.u;
	return ret;
}

TPUID TPUID::operator^(const TPUID& rhs) const
{
	auto ret = *this;
	ret.tpuid ^= rhs.tpuid;
	return ret;
}

TPUID TPUID::operator|(const TPUID& rhs) const
{
	auto ret = *this;
	ret.tpuid |= rhs.tpuid;
	return ret;
}

TPUID TPUID::operator&(const TPUID& rhs) const
{
	auto ret = *this;
	ret.tpuid &= rhs.tpuid;
	return ret;
}

TPUID TPUID::operator~() const
{
	auto ret = *this;
	ret.tpuid = ~ret.tpuid;
	return ret;
}

TPUID& TPUID::operator+=(const TPUID& rhs)
{
	this->t += rhs.t;
	this->p += rhs.p;
	this->u += rhs.u;
	return *this;
}

TPUID& TPUID::operator-=(const TPUID& rhs)
{
	this->t -= rhs.t;
	this->p -= rhs.p;
	this->u -= rhs.u;
	return *this;
}

TPUID& TPUID::operator&=(const TPUID& rhs)
{
	this->tpuid &= rhs.tpuid;
	return *this;
}

TPUID& TPUID::operator|=(const TPUID& rhs)
{
	this->tpuid |= rhs.tpuid;
	return *this;
}

TPUID& TPUID::operator^=(const TPUID& rhs)
{
	this->tpuid ^= rhs.tpuid;
	return *this;
}

bool PPAC::operator==(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid == rhs.tpuid;
}

bool PPAC::operator!=(const TPUID& lhs, const TPUID& rhs)
{
	return !(lhs == rhs);
}

bool PPAC::operator<(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid < rhs.tpuid;
}

bool PPAC::operator>(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid > rhs.tpuid;
}

bool PPAC::operator>=(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid >= rhs.tpuid;
}

bool PPAC::operator<=(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid <= rhs.tpuid;
}

OPENPACFILEHANDLE::OPENPACFILEHANDLE(HANDLE handle, LPCWSTR name)
{
	_handle = handle;
	_name = name;
}

OPENPACFILEHANDLE::~OPENPACFILEHANDLE()
{
	CloseHandle(_handle);
	std::cout << "Closing handle " << _name;
}

//std::wstring s2ws(const std::string& s)
//{
//	int len;
//	int slength = int(s.length()) + 1;
//	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
//	wchar_t* buf = new wchar_t[len];
//	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
//	std::wstring r(buf);
//	delete[] buf;
//	return r;
//}

int _memcpyIncr(void* dest, const void* src, const size_t sz)
{
	memcpy(dest, src, sz);
	return sz;
}

void _swp16(uint16* i)
{
	*i = (*i << 8) | (*i >> 8);
}

void _swp32(uint32* i)
{
	*i = ((*i << 8) & 0xFF00FF00) | ((*i >> 8) & 0xFF00FF);
	*i = (*i << 16) | (*i >> 16);
}

void _swp64(uint64* i)
{
	*i = ((*i << 8) & 0xFF00FF00FF00FF00ULL) | ((*i >> 8) & 0x00FF00FF00FF00FFULL);
	*i = ((*i << 16) & 0xFFFF0000FFFF0000ULL) | ((*i >> 16) & 0x0000FFFF0000FFFFULL);
	*i = (*i << 32) | (*i >> 32);
}

int _memcpyIncrSwp(void* dest, const void* src, const size_t sz)
{
	auto ret = _memcpyIncr(dest, src, sz);
	if (sz == 2)
	{
		uint16* asUint16 = static_cast<uint16*>(dest);
		_swp16(asUint16);
	}
	else if (sz == 4)
	{
		uint32* asUint32 = static_cast<uint32*>(dest);
		_swp32(asUint32);
	}
	else if (sz == 8)
	{
		uint64* asUint64 = static_cast<uint64*>(dest);
		_swp64(asUint64);
	}
	return ret;
}

#define READ_FIELD_INCR(field, buffer) buffer += _memcpyIncr(&field, buffer, sizeof(field))
#define READ_FIELD_INCRSWP(field, buffer) buffer += _memcpyIncrSwp(&field, buffer, sizeof(field))
#define READ_FIELD_INCROPTSWP(field, buffer, swap) buffer += swap ? _memcpyIncrSwp(&field, buffer, sizeof(field)) : _memcpyIncr(&field, buffer, sizeof(field))

cPPAC::cPPAC(LPCWSTR file)
{
	CLOG(DEBUG, "PPAC") << "Reading file " << file;
	HANDLE hFile = CreateFileW(file,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CLOG(WARNING, "PPAC") << "Failed to open handle to file " << file;
		throw "Unable to open file";
	}
	_handle = std::make_unique<OPENPACFILEHANDLE>(hFile, file);

	bool needSwp = false;
	//	READ HEADER
	std::unique_ptr<char[]> buffer(new char[DISKSZ_PPACHEADER]);
	DWORD dwBytesRead;
	BOOL bFlag = ReadFile(_handle.get()->_handle, buffer.get(), DISKSZ_PPACHEADER, &dwBytesRead, nullptr);
	if (bFlag == FALSE)
	{
		CLOG(WARNING, "PPAC") << "Failed to read file " << file << " " << GetLastError();
		throw "Unable to read file";
	}
	if (dwBytesRead != DISKSZ_PPACHEADER)
	{
		CLOG(WARNING, "PPAC") << "Incomplete read of file " << file;
		throw "Incomplete read";
	}
	char* bufPtr = buffer.get();
	//	Read the magic word and decide if we need to swap bytes (LE vs BE)
	uint32 magic;
	READ_FIELD_INCR(magic, bufPtr);
	if (magic == PPAC_HEAD_MAGICSWP)
	{
		needSwp = true;
		_swp32(&magic);
	}
	else if (magic != PPAC_HEAD_MAGIC)
	{
		CLOG(WARNING, "PPAC") << "Invalid PPAC file: bad magic";
		throw "Bad magic";
	}
	_header.hMagic = magic;
	READ_FIELD_INCROPTSWP(_header.hMajorVer, bufPtr, needSwp);
	READ_FIELD_INCROPTSWP(_header.hMinorVer, bufPtr, needSwp);
	if (_header.hMajorVer != 4)
	{
		CLOG(WARNING, "PPAC") << "Unsupported PPAC file version " << _header.hMajorVer << "." << _header.hMinorVer;
		throw "Unsupported PPAC major version";
	}
	if (_header.hMinorVer != 0)
	{
		CLOG(WARNING, "PPAC") << "Unsupported PPAC file version " << _header.hMajorVer << "." << _header.hMinorVer;
		throw "Unsupported PPAC minor version";
	}
	READ_FIELD_INCR(_header.hReserved, bufPtr);
	READ_FIELD_INCROPTSWP(_header.hFlags, bufPtr, needSwp);
	if (PPACF_CHK_FLAG(PPACF_USE_LONG_OFFSETS, _header.hFlags))
	{
		//	Long offsets
#ifndef PPAC_OPT_LONG_OFFSET
		//	If not compiled with long offsets, reject
		CLOG(WARNING, "PPAC") << "Long offsets are not supported";
		throw "Unsupported flag PPACF_USE_LONG_OFFSETS";
#else
		static_assert(sizeof(_header.hIndexOffset) == sizeof(OFFSET), "Field size doesn't equal offset size");
		//	Using long offsets
		READ_FIELD_INCROPTSWP(_header.hIndexOffset, bufPtr, needSwp);
		READ_FIELD_INCROPTSWP(_header.hMetadataOffset, bufPtr, needSwp);
		READ_FIELD_INCROPTSWP(_header.hTrashIndexOffset, bufPtr, needSwp);
#endif
	}
	else
	{
		//	Narrow offsets
#ifndef PPAC_OPT_LONG_OFFSET
		static_assert(sizeof(_header.hIndexOffset) == sizeof(OFFSET), "Field size doesn't equal offset size");
		//	If not compiled with long offsets, no conversions necessary
		READ_FIELD_INCROPTSWP(_header.hIndexOffset, bufPtr, needSwp);
		READ_FIELD_INCROPTSWP(_header.hMetadataOffset, bufPtr, needSwp);
		READ_FIELD_INCROPTSWP(_header.hTrashIndexOffset, bufPtr, needSwp);
#else
		//	Need to upconvert
		static_assert(sizeof(_header.hIndexOffset) == sizeof(OFFSET), "Field size doesn't equal offset size");
		uint32 hIndexOffset;
		uint32 hMetadataOffset;
		uint32 hTrashIndexOffset;
		READ_FIELD_INCROPTSWP(hIndexOffset, bufPtr, needSwp);
		READ_FIELD_INCROPTSWP(hMetadataOffset, bufPtr, needSwp);
		READ_FIELD_INCROPTSWP(hTrashIndexOffset, bufPtr, needSwp);
		_header.hIndexOffset = hIndexOffset;
		_header.hMetadataOffset = hMetadataOffset;
		_header.hTrashIndexOffset = hTrashIndexOffset;
#endif
	}

	//	READ INDEX

}

std::unique_ptr<cPPACData> cPPAC::GetFileData(const TPUID& tpuid)
{
	//	TODO
	return nullptr;
}

std::vector<PPACINDEXENTRY> cPPAC::GetEntries(const TPUID& mask)
{
	//	TODO
	return std::vector<PPACINDEXENTRY>(0);
}

std::map<std::string, std::string> cPPAC::GetMetadata(const TPUID& tpuid)
{
	//	TODO
	return std::map<std::string, std::string>();
}



