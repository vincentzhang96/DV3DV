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
	CLOG(INFO, DNPAKLOGGER) << "Num entries: " << _header.hFileCount;
	for (auto i = 0U; i < _header.hFileCount; ++i)
	{
		auto start = bufPtr;
		PakFileTableEntry entry;
		bufPtr += dn::_memcpyIncr(strBuf, bufPtr, 256);
		std::string path(strBuf);
		std::transform(path.begin(), path.end(), path.begin(), ::tolower);
		entry.eFilePath = std::string(path);
		READ_FIELD_INCR(entry.eDiskSize, bufPtr);
		READ_FIELD_INCR(entry.eDecompressedSize, bufPtr);
		READ_FIELD_INCR(entry.eCompressedSize, bufPtr);
		READ_FIELD_INCR(entry.eDataOffset, bufPtr);
		READ_FIELD_INCR(entry.eUnknownA, bufPtr);
		READ_FIELD_INCR(entry.eReserved, bufPtr);
		//	Skip files that have been zeroed by the patcher
		if (entry.eDecompressedSize != 0)
		{
			std::pair<std::string, PakFileTableEntry> pair(path, entry);
			auto ret = _fileTable.insert(pair);
			if (!ret.second)
			{
				CLOG(INFO, DNPAKLOGGER) << path << " skipped, duplicate entry";
			}
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
	//	Only one thread may read at a time since we share the file handle
	std::lock_guard<std::mutex> lock(_readMutex);
	auto iter = _fileTable.find(fileName);
	if (iter == _fileTable.end())
	{
		//	Not found
		return std::unique_ptr<cPakData>(nullptr);
	}
	PakFileTableEntry entry = iter->second;
	LONG lDistanceToMove = entry.eDataOffset;
	LONG lDistanceToMoveHigh = 0L;
	if (SetFilePointer(_handle.get()->_handle, lDistanceToMove, &lDistanceToMoveHigh, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Failed to read subfile SetFilePointer " << GetLastError();
		return std::unique_ptr<cPakData>(nullptr);
	}
	std::unique_ptr<uint8[]> buffer(new uint8[entry.eDiskSize]);
	DWORD dwBytesRead;
	BOOL bFlag = ReadFile(_handle.get()->_handle, buffer.get(), entry.eDiskSize, &dwBytesRead, nullptr);
	if (bFlag == FALSE)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Failed to read subfile" << GetLastError();
		throw "Unable to read file";
	}
	if (dwBytesRead != entry.eDiskSize)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Incomplete read of subfile";
		throw "Incomplete read";
	}
	auto bufPtr = buffer.get();
	auto ret = std::make_unique<cPakData>(entry.eFilePath, entry.eDecompressedSize);
	auto retVec = &ret.get()->_data;
	z_stream zInfo = { nullptr };
	zInfo.total_in = entry.eDiskSize;
	zInfo.avail_in = entry.eDiskSize;
	zInfo.total_out = entry.eDecompressedSize;
	zInfo.avail_out = entry.eDecompressedSize;
	zInfo.next_in = bufPtr;
	zInfo.next_out = retVec->data();
	int zError;
	int zRet = -1;
	zError = inflateInit(&zInfo);
	if (zError == Z_OK)
	{
		zError = inflate(&zInfo, Z_FINISH);
		if (zError == Z_STREAM_END)
		{
			zRet = zInfo.total_out;
		}
	}
	else
	{
		CLOG(WARNING, DNPAKLOGGER) << "Error while decompressing: " << zError;
	}
	inflateEnd(&zInfo);
	if (zRet != entry.eDecompressedSize)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Decompression amount doesn't match index size: "
			<< entry.eDecompressedSize << ", got " << zRet;
	}
	return ret;
}

std::vector<PakFileTableEntry> cPak::GetFiles()
{
	std::vector<PakFileTableEntry> ret(_fileTable.size());
	for (auto it : _fileTable)
	{
		ret.push_back(it.second);
	}
	return ret;
}

cPak::PakFileTable cPak::GetFileTable()
{
	return _fileTable;
}
