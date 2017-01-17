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

OPENPACFILEHANDLE::OPENPACFILEHANDLE(HANDLE handle, std::string name)
{
	_handle = handle;
	_name = name;
}

OPENPACFILEHANDLE::~OPENPACFILEHANDLE()
{
	CloseHandle(_handle);
	std::cout << "Closing handle " << _name;
}

std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = int(s.length()) + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

cPPAC::cPPAC(std::string file)
{
	LPCWSTR lpcFile = s2ws(file).c_str();
	HANDLE hFile = CreateFileW(lpcFile,
		GENERIC_WRITE,
		0,
		nullptr,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		throw "Unable to open file";
	}
	_handle = std::make_unique<OPENPACFILEHANDLE>(hFile, file);
	std::unique_ptr<char[]> buffer(new char[DISKSZ_PPACHEADER]);
	DWORD dwBytesRead;
	BOOL bFlag = ReadFile(_handle.get(), buffer.get(), DISKSZ_PPACHEADER, &dwBytesRead, nullptr);
	if (bFlag == FALSE)
	{
		throw "Unable to read file";
	}
	if (dwBytesRead != DISKSZ_PPACHEADER)
	{
		throw "Incomplete read";
	}

}

