#include "DnPak.h"

using namespace dn;

OPENPAKFILEHANDLE::OPENPAKFILEHANDLE(HANDLE handle, std::wstring name)
{
	_handle = handle;
	_name = name;
}

OPENPAKFILEHANDLE::~OPENPAKFILEHANDLE()
{
	CloseHandle(_handle);
	std::cout << "Closing handle " << _name.c_str();
}

cPakData::cPakData(std::string name, uint32 size)
{
}

cPakData::~cPakData()
{
}

bool cPak::_ReadHeader(const std::wstring file)
{
	return false;
}

void cPak::_ReadIndex(const bool needSwp, const std::wstring file)
{
}

cPak::cPak(LPCWSTR file)
{
}

std::unique_ptr<cPakData> cPak::GetFileData(const std::string fileName)
{
	return std::unique_ptr<cPakData>(nullptr);
}

std::vector<PakFileTableEntry> cPak::GetEntries()
{
	return std::vector<PakFileTableEntry>();
}
