#include "DnPak.h"

using namespace dn;

PakFileTableEntry::PakFileTableEntry()
{
}

PakFileTableEntry::PakFileTableEntry(void* ignored)
{
}


OPENPAKFILEHANDLE::OPENPAKFILEHANDLE(HANDLE handle, std::wstring name)
{
	_handle = handle;
	_name = name;
}

OPENPAKFILEHANDLE::~OPENPAKFILEHANDLE()
{
	CLOG(DEBUG, DNPAKLOGGER) << "Closing handle " << _name;
	CloseHandle(_handle);
}

cPakData::cPakData(std::string name, uint32 size)
{
	_name = name;
	_size = size;
	_data.reserve(_size);
}

cPakData::~cPakData()
{
}

int dn::_memcpyIncr(void* dest, const void* src, const size_t sz)
{
	memcpy(dest, src, sz);
	return sz;
}

#define READ_FIELD_INCR(field, buffer) buffer += dn::_memcpyIncr(&field, buffer, sizeof(field))

void cPak::_ReadHeader(const std::wstring file)
{
	std::unique_ptr<uint8[]> buffer(new uint8[szPakHeader]);
	DWORD dwBytesRead;
	BOOL bFlag = ReadFile(_handle.get()->_handle, buffer.get(), szPakHeader, &dwBytesRead, nullptr);
	if (bFlag == FALSE)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Failed to read file " << file << " " << GetLastError();
		throw "Unable to read file";
	}
	if (dwBytesRead != szPakHeader)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Incomplete read of file " << file;
		throw "Incomplete read";
	}
	auto bufPtr = buffer.get();
	READ_FIELD_INCR(_header.hMagic, bufPtr);
	READ_FIELD_INCR(_header.hVersion, bufPtr);
	if (_header.hVersion != 11)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Unsupported version " << _header.hVersion;
		throw "Unsupported version";
	}
	READ_FIELD_INCR(_header.hFileCount, bufPtr);
	READ_FIELD_INCR(_header.hFileTableOffset, bufPtr);

}

void cPak::_ReadIndex(const std::wstring file)
{
	LONG lDistanceToMove = _header.hFileTableOffset;
	LONG lDistanceToMoveHigh = 0L;
	if (SetFilePointer(_handle.get()->_handle, lDistanceToMove, &lDistanceToMoveHigh, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Failed to read file " << file << " SetFilePointer " << GetLastError();
		throw "Unable to read file";
	}
	size_t tableLen = szPakFileTableEntry * _header.hFileCount;
	_fileTable.reserve(_header.hFileCount);
	std::unique_ptr<uint8[]> buffer(new uint8[tableLen]);
	DWORD dwBytesRead;
	BOOL bFlag = ReadFile(_handle.get()->_handle, buffer.get(), tableLen, &dwBytesRead, nullptr);
	if (bFlag == FALSE)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Failed to read file " << file << " " << GetLastError();
		throw "Unable to read file";
	}
	if (dwBytesRead != tableLen)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Incomplete read of file " << file;
		throw "Incomplete read";
	}
	auto bufPtr = buffer.get();
	char strBuf[256];
	for (auto i = 0U; i < _header.hFileCount; ++i)
	{
		auto strt = bufPtr;
		bufPtr += dn::_memcpyIncr(strBuf, bufPtr, 256);
		std::string path(strBuf);
		std::transform(path.begin(), path.end(), path.begin(), ::tolower);
		auto ret = _fileTable.emplace(path, nullptr);
		if (ret.second)
		{
			auto entryIter = ret.first;
			auto entry = entryIter->second;
			entry.eFilePath = entryIter->first;
			READ_FIELD_INCR(entry.eDiskSize, bufPtr);
			READ_FIELD_INCR(entry.eDecompressedSize, bufPtr);
			READ_FIELD_INCR(entry.eCompressedSize, bufPtr);
			READ_FIELD_INCR(entry.eDataOffset, bufPtr);
			READ_FIELD_INCR(entry.eUnknownA, bufPtr);
			READ_FIELD_INCR(entry.eReserved, bufPtr);
		}
		else
		{
			bufPtr += szPakFileTableEntry - 256;
		}
	}
}

cPak::cPak(LPCWSTR file)
{
	CLOG(DEBUG, DNPAKLOGGER) << "Reading file " << file;
	HANDLE hFile = CreateFileW(file,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Failed to open handle to file " << file;
		throw "Unable to open file";
	}
	_handle = std::make_unique<OPENPAKFILEHANDLE>(hFile, file);

	//	READ HEADER
	_ReadHeader(file);
	//	READ INDEX
	_ReadIndex(file);
}

std::unique_ptr<cPakData> cPak::GetFileData(const std::string fileName)
{
	return std::unique_ptr<cPakData>(nullptr);
}

std::vector<PakFileTableEntry> cPak::GetEntries()
{
	return std::vector<PakFileTableEntry>();
}
