#include "../stdafx.h"
#include "DnPak.h"

using namespace dn;

PakFileTableEntry::PakFileTableEntry()
{
	eDiskSize = 0;
	eDecompressedSize = 0;
	eCompressedSize = 0;
	eDataOffset = 0;
	eUnknownA = 0;
}

PakFileTableEntry::PakFileTableEntry(void* ignored)
{
	eDiskSize = 0;
	eDecompressedSize = 0;
	eCompressedSize = 0;
	eDataOffset = 0;
	eUnknownA = 0;
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

cPak::cPak(const std::wstring file)
{
	CLOG(DEBUG, DNPAKLOGGER) << "Reading file " << file;
	HANDLE hFile = CreateFileW(file.c_str(),
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

cPakManager::cPakManager() : _loadedPaks(512)
{
}

cPakManager::~cPakManager()
{
}

PAKHANDLE cPakManager::LoadPak(std::wstring pathToPak)
{
	try
	{
		//	Construct the PPAC in place in the packed freelist
		PAKHANDLE hPak = _loadedPaks.emplace(pathToPak.c_str());
		if (hPak == PAKHANDLE_INVALID)
		{
			return PAKHANDLE_INVALID;
		}
		//	Index all the entries
		auto entries = _loadedPaks[hPak].GetFiles();
		for (auto entry : entries)
		{
			//	Use insert, which does not write if the key already exists
			//	DN uses whatever is loaded first, not last
			_pathToPakMap.insert(std::pair<std::string, PAKHANDLE>(entry.eFilePath, hPak));
		}
		CLOG(TRACE, DNPAKLOGGER) << "Indexed " << entries.size() << " subfiles";
		return hPak;
	}
	catch (const std::string msg)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Failed to load " << pathToPak << ": " << msg << " (file skipped)";
		return PAKHANDLE_INVALID;
	}
	catch (...)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Failed to load " << pathToPak << ": unknown (file skipped)";
		return PAKHANDLE_INVALID;
	}
}

bool cPakManager::comparePakFileNames(FileEntry a, FileEntry b)
{
	bool aPatch = _startsWith(a.fileName, L"Patch");
	bool bPatch = _startsWith(b.fileName, L"Patch");
	if (aPatch != bPatch)
	{
		//	Patches come before rest of paks
		return aPatch;
	}
	if (aPatch)
	{
		//	Patches load in reverse ABC order
		return b.fileName.compare(a.fileName) < 0;
	}
	else
	{
		//	Rest of paks load in ABC order
		return a.fileName.compare(b.fileName) < 0;
	}
}

bool cPakManager::LoadDir(std::wstring pathToDir)
{
	//	We aren't recursively loading the directory
	WIN32_FIND_DATAW fdData;
	HANDLE hFind;
	DWORD dwError;
	size_t szPathLength;
	StringCchLengthW(pathToDir.c_str(), MAX_PATH, &szPathLength);
	if (szPathLength > MAX_PATH - 3)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Ignoring directory " << pathToDir << " since its too deep";
		return false;
	}
	//	Append \* to the path since we want to recursively look through all files in the dir
	std::wstring extDir = pathToDir + L"\\*";
	hFind = FindFirstFileW(extDir.c_str(), &fdData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		CLOG(WARNING, "PPAC") << "Unable to get first file in directory " << pathToDir;
		return false;
	}
	std::vector<FileEntry> files;
	do
	{
		//	Ignore "." and ".." and other directories
		if (wcscmp(fdData.cFileName, L".") == 0 ||
			wcscmp(fdData.cFileName, L"..") == 0 || fdData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			//	Ignore
		}
		else if (_endsWith(fdData.cFileName, L".pak"))
		{
			files.emplace_back(pathToDir + L"\\" + fdData.cFileName, fdData.cFileName);
		}
	} while (FindNextFileW(hFind, &fdData) != FALSE);
	dwError = GetLastError();
	FindClose(hFind);
	if (dwError != ERROR_NO_MORE_FILES)
	{
		CLOG(WARNING, DNPAKLOGGER) << "Failed to load directory " << pathToDir << ": " << dwError;
		return false;
	}
	//	Also, load in the following order:
	//		1. Files ending in .pak that start with Patch, in reverse alphabetical order
	//		2. Files ending in .pak that do not start with Patch, in alphabetical order
	//	This allows us to load files in the same order tha game does, except we can support patch files
	//	without having to apply them to the paks themselves.
	std::sort(files.begin(), files.end(), comparePakFileNames);
	for (auto filePath : files)
	{
		auto fullPath = filePath.fullPath;
		CLOG(INFO, DNPAKLOGGER) << "\t" << fullPath;
		//	Load
		PAKHANDLE handle = LoadPak(fullPath);
		if (handle == PAKHANDLE_INVALID)
		{
			CLOG(WARNING, DNPAKLOGGER) << "Failed to read pak " << fullPath;
		}
		else
		{
			CLOG(DEBUG, DNPAKLOGGER) << "Loaded pak " << fullPath;
		}
	}
	return true;
}

bool cPakManager::FileExists(const std::string& path)
{
	return _pathToPakMap.find(path) != _pathToPakMap.end();
}

std::unique_ptr<cPakData> cPakManager::GetFileData(const std::string& path)
{
	//	Find the PPAC that the entry belongs to
	auto iter = _pathToPakMap.find(path);
	if (iter != _pathToPakMap.end())
	{
		//	Grab the handle
		PAKHANDLE hPak = iter->second;
		if (hPak != PAKHANDLE_INVALID)
		{
			//	Get the data
			return _loadedPaks[hPak].GetFileData(path);
		}
	}
	//	Failed
	CLOG(WARNING, DNPAKLOGGER) << "No such file " << path;
	return std::unique_ptr<cPakData>(nullptr);
}

void cPakManager::Unload()
{
	//	packed_freelist doesn't implement clear, so we'll just deconstruct it and reconstruct it
	(&_loadedPaks)->~packed_freelist();
	_pathToPakMap.clear();
	_loadedPaks = packed_freelist<cPak>(512);
}

bool dn::_endsWith(std::wstring const &a, std::wstring const &suffix)
{
	if (a.length() >= suffix.length())
	{
		return (a.compare(a.length() - suffix.length(), suffix.length(), suffix)) == 0;
	}
	return false;
}

bool dn::_startsWith(std::wstring const& a, std::wstring const& b)
{
	return (a.compare(0, b.length(), b) == 0);
}

