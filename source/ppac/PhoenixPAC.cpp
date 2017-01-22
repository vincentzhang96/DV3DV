#include "PhoenixPAC.h"

using namespace ppac;

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

bool ppac::operator==(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid == rhs.tpuid;
}

bool ppac::operator!=(const TPUID& lhs, const TPUID& rhs)
{
	return !(lhs == rhs);
}

bool ppac::operator<(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid < rhs.tpuid;
}

bool ppac::operator>(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid > rhs.tpuid;
}

bool ppac::operator>=(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid >= rhs.tpuid;
}

bool ppac::operator<=(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid <= rhs.tpuid;
}

void PPACMETABLOCKENTRY::AddToMap(PPACMETABLOCK::PPACMETABLOCKENTRIES& map)
{
	map.insert_or_assign(mbeKey, mbeVal);
}

OPENPACFILEHANDLE::OPENPACFILEHANDLE(HANDLE handle, LPCWSTR name)
{
	_handle = handle;
	_name = name;
}

OPENPACFILEHANDLE::~OPENPACFILEHANDLE()
{
	CloseHandle(_handle);
	CLOG(DEBUG, "PPAC") << "Closing handle " << _name;
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

bool cPPAC::_ReadHeader(const LPCWSTR file)
{
	bool needSwp = false;
	std::unique_ptr<uint8[]> buffer(new uint8[DISKSZ_PPACHEADER]);
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
	auto bufPtr = buffer.get();
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
	return needSwp;
}

void cPPAC::_ReadIndex(const bool needSwp, const LPCWSTR file)
{
	//	Move to index
#ifndef PPAC_OPT_LONG_OFFSET
	LONG lDistanceToMove = _header.hIndexOffset;
	LONG lDistanceToMoveHigh = 0L;
#else
	LONG lDistanceToMove = _header.hIndexOffset & 0xFFFFFFFFL;
	LONG lDistanceToMoveHigh = (_header.hIndexOffset >> 32) 0xFFFFFFFFL;
#endif
	if (SetFilePointer(_handle.get()->_handle, lDistanceToMove, &lDistanceToMoveHigh, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		CLOG(WARNING, "PPAC") << "Failed to read file " << file << " SetFilePointer " << GetLastError();
		throw "Unable to read file";
	}

	//	Read index size
	DWORD dwBytesRead;
	BOOL bFlag = ReadFile(_handle.get()->_handle, &_index.iCount, sizeof(_index.iCount), &dwBytesRead, nullptr);
	if (bFlag == FALSE)
	{
		CLOG(WARNING, "PPAC") << "Failed to read file " << file << " " << GetLastError();
		throw "Unable to read file";
	}
	if (dwBytesRead != sizeof(_index.iCount))
	{
		CLOG(WARNING, "PPAC") << "Incomplete read of file " << file;
		throw "Incomplete read";
	}
	if (needSwp)
	{
		_swp32(&_index.iCount);
	}
	size_t indexBodyLen = DISKSZ_PPACINDEXENTRY * _index.iCount;
	CLOG(DEBUG, "PPAC") << "Index with " << _index.iCount << " sz " << indexBodyLen;

	//	Read in the index body
	std::unique_ptr<uint8[]> buffer(new uint8[indexBodyLen]);
	bFlag = ReadFile(_handle.get()->_handle, buffer.get(), indexBodyLen, &dwBytesRead, nullptr);
	if (bFlag == FALSE)
	{
		CLOG(WARNING, "PPAC") << "Failed to read file " << file << " " << GetLastError();
		throw "Unable to read file";
	}
	if (dwBytesRead != indexBodyLen)
	{
		CLOG(WARNING, "PPAC") << "Incomplete read of file " << file;
		throw "Incomplete read";
	}
	auto bufPtr = buffer.get();
	for (auto i = 0U; i < _index.iCount; ++i)
	{
		PPACINDEXENTRY entry;
		READ_FIELD_INCROPTSWP(entry.ieTPUID.t, bufPtr, needSwp);
		READ_FIELD_INCROPTSWP(entry.ieTPUID.p, bufPtr, needSwp);
		READ_FIELD_INCROPTSWP(entry.ieTPUID.u, bufPtr, needSwp);
		READ_FIELD_INCROPTSWP(entry.ieDiskOffset, bufPtr, needSwp);
		READ_FIELD_INCROPTSWP(entry.ieDiskSize, bufPtr, needSwp);
		READ_FIELD_INCROPTSWP(entry.ieMemorySize, bufPtr, needSwp);
		READ_FIELD_INCROPTSWP(entry.ieCompressionType, bufPtr, needSwp);
		READ_FIELD_INCR(entry.ieReserved, bufPtr);
		READ_FIELD_INCR(entry.ieSHA256, bufPtr);
		_index.iEntries.insert_or_assign(entry.ieTPUID, entry);
	}

	//	Read guard bytes
	bFlag = ReadFile(_handle.get()->_handle, &_index.iGuard, sizeof(_index.iGuard), &dwBytesRead, nullptr);
	if (bFlag == FALSE)
	{
		CLOG(WARNING, "PPAC") << "Failed to read file " << file << " " << GetLastError();
		throw "Unable to read file";
	}
	if (dwBytesRead != sizeof(_index.iGuard))
	{
		CLOG(WARNING, "PPAC") << "Incomplete read of file " << file;
		throw "Incomplete read";
	}
	if (needSwp)
	{
		_swp32(&_index.iGuard);
	}
	if (!PPAC_CHK_INDEX_GUARD(_index))
	{
		CLOG(WARNING, "PPAC") << "Index guard bytes are incorrect, this file may be corrupt, but continuing";
	}
}

void cPPAC::_ReadMetadata(const bool needSwp, const LPCWSTR file)
{
	//	TODO implement later, for now pretend no meta
	_meta.mCount = 0;
	_meta.mBlocks.clear();
}

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

	bool needSwp;
	//	READ HEADER
	needSwp = _ReadHeader(file);
	//	READ INDEX
	_ReadIndex(needSwp, file);
}

std::unique_ptr<cPPACData> cPPAC::GetFileData(const TPUID& tpuid)
{
	//	Only one thread may read at a time since we share the file handle
	std::lock_guard<std::mutex> lock(_readMutex);
	auto iter = _index.iEntries.find(tpuid);
	if (iter == _index.iEntries.end())
	{
		//	Not found
		return std::unique_ptr<cPPACData>(nullptr);
	}
	PPACINDEXENTRY entry = iter->second;
	int compType = entry.ieCompressionType;
	if (!PPAC_COMPRESSION_SUPPORTED(compType))
	{
		CLOG(WARNING, "PPAC") << "Unsupported compression type " << compType;
		return std::unique_ptr<cPPACData>(nullptr);
	}
	//	Move to entry
#ifndef PPAC_OPT_LONG_OFFSET
	LONG lDistanceToMove = entry.ieDiskOffset;
	LONG lDistanceToMoveHigh = 0L;
#else
	LONG lDistanceToMove = entry.ieDiskOffset & 0xFFFFFFFFL;
	LONG lDistanceToMoveHigh = (entry.ieDiskOffset >> 32) 0xFFFFFFFFL;
#endif
	if (SetFilePointer(_handle.get()->_handle, lDistanceToMove, &lDistanceToMoveHigh, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		CLOG(WARNING, "PPAC") << "Failed to read subfile SetFilePointer " << GetLastError();
		return std::unique_ptr<cPPACData>(nullptr);
	}
	std::unique_ptr<uint8[]> buffer(new uint8[entry.ieDiskSize]);
	DWORD dwBytesRead;
	BOOL bFlag = ReadFile(_handle.get()->_handle, buffer.get(), entry.ieDiskSize, &dwBytesRead, nullptr);
	if (bFlag == FALSE)
	{
		CLOG(WARNING, "PPAC") << "Failed to read subfile" << GetLastError();
		throw "Unable to read file";
	}
	if (dwBytesRead != entry.ieDiskSize)
	{
		CLOG(WARNING, "PPAC") << "Incomplete read of subfile";
		throw "Incomplete read";
	}
	auto bufPtr = buffer.get();
	auto ret = std::make_unique<cPPACData>(entry.ieTPUID, entry.ieMemorySize);
	auto retVec = &ret.get()->_data;
	//	Decompress if necessary
	if (compType != 0)
	{
		if (compType == 1)
		{
			z_stream zInfo = { nullptr };
			zInfo.total_in = entry.ieDiskSize;
			zInfo.avail_in = entry.ieDiskSize;
			zInfo.total_out = entry.ieMemorySize;
			zInfo.avail_out = entry.ieMemorySize;
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
				CLOG(WARNING, "PPAC") << "Error while decompressing: " << zError;
			}
			inflateEnd(&zInfo);
			if (zRet != entry.ieMemorySize)
			{
				CLOG(WARNING, "PPAC") << "Decompression amount doesn't match index size: "
					<< entry.ieMemorySize << ", got " << zRet;
			}
		}
		else
		{
			throw "Unsupported compression";
		}
	}
	else
	{
		if (entry.ieDiskSize != entry.ieMemorySize)
		{
			CLOG(WARNING, "PPAC") << "Uncompressed file has different disk and memory sizes";
			throw "Mismatched disk and memory size";
		}
		//	Copy the data to the vector
		retVec->insert(retVec->begin(), bufPtr, bufPtr + sizeof(uint8) * entry.ieDiskSize);
	}
	return ret;
}

std::vector<PPACINDEXENTRY> cPPAC::GetEntries(const TPUID& filter, const TPUID& mask)
{
	std::vector<PPACINDEXENTRY> ret;
	//	Fast path, we want all the entries
	if (mask == 0 && filter == 0)
	{
		ret.reserve(_index.iEntries.size());
		for (auto kv : _index.iEntries)
		{
			ret.push_back(kv.second);
		}
	}
	//	Fast path, we care about all the bits of filter -> match one
	else if (mask == 0xFFFFFFFFFFFFFFFFL)
	{
		auto iter = _index.iEntries.find(filter);
		if (iter != _index.iEntries.end())
		{
			ret.push_back(iter->second);
		}
	}
	//	Fast path, no results
	else if (mask == 0)
	{
		// do nothing
	}
	//	Must filter
	else
	{
		ret.reserve(_index.iEntries.size());
		for (auto kv : _index.iEntries)
		{
			if ((kv.first & mask) == filter)
			{
				ret.push_back(kv.second);
			}
		}
	}
	return ret;
}

const PPACINDEX::PPACINDEXENTRIES & cPPAC::GetEntryMap() const
{
	return _index.iEntries;
}

PPACMETABLOCK::PPACMETABLOCKENTRIES cPPAC::GetMetadata(const TPUID& tpuid)
{
	//	TODO
	return PPACMETABLOCK::PPACMETABLOCKENTRIES();
}

cPPACData::cPPACData(TPUID tpuid, uint32 size)
{
	_tpuid = tpuid;
	_size = size;
	_data.reserve(size);
}

cPPACData::~cPPACData()
{
}

cPPACManager::cPPACManager()
{
	_loadedPPACs = packed_freelist<cPPAC>(1000);
}

cPPACManager::~cPPACManager()
{
}

PPACHANDLE cPPACManager::LoadPPAC(std::wstring pathToPPAC)
{
	try
	{
		//	Construct the PPAC in place in the packed freelist
		PPACHANDLE hPPAC = _loadedPPACs.emplace(pathToPPAC.c_str());
		if (hPPAC == PPACHANDLE_INVALID)
		{
			return PPACHANDLE_INVALID;
		}
		//	Index all the entries
		auto entries = _loadedPPACs[hPPAC].GetEntries();
		for (auto entry : entries)
		{
			_tpuidToPPACMap.insert_or_assign(entry.ieTPUID, hPPAC);
		}
		CLOG(DEBUG, "PPAC") << "Indexed " << entries.size() << " subfiles";
		return hPPAC;
	}
	catch(const std::string msg)
	{
		CLOG(WARNING, "PPAC") << "Failed to load " << pathToPPAC << ": " << msg << " (file skipped)";
		return PPACHANDLE_INVALID;
	}
	catch(...)
	{
		CLOG(WARNING, "PPAC") << "Failed to load " << pathToPPAC << ": unknown (file skipped)";
		return PPACHANDLE_INVALID;
	}
}

bool _endsWith(std::wstring const &a, std::wstring const &suffix)
{
	if (a.length() >= suffix.length())
	{
		return (a.compare(a.length() - suffix.length(), suffix.length(), suffix)) == 0;
	}
	return false;
}

bool cPPACManager::LoadDir(std::wstring pathToDir)
{
	WIN32_FIND_DATAW fdData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;
	size_t szPathLength;
	StringCchLengthW(pathToDir.c_str(), MAX_PATH, &szPathLength);
	if (szPathLength > MAX_PATH - 3)
	{
		CLOG(WARNING, "PPAC") << "Ignoring directory " << pathToDir << " since its too deep";
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
	do
	{
		//	Ignore "." and ".."
		if (wcscmp(fdData.cFileName, L".") == 0 ||
			wcscmp(fdData.cFileName, L"..") == 0)
		{
			//	Skip
		}
		else if (fdData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			//	Recursive call
			bool result = LoadDir(fdData.cFileName);
			if (!result)
			{
				CLOG(WARNING, "PPAC") << "Failed to load subdirectory " << fdData.cFileName;
			}
		}
		else if (_endsWith(fdData.cFileName, L".ppac"))
		{
			//	Load
			PPACHANDLE handle = LoadPPAC(pathToDir + L"\\" + fdData.cFileName);
			if (handle == PPACHANDLE_INVALID)
			{
				CLOG(WARNING, "PPAC") << "Failed to read PPAC " << fdData.cFileName;
			}
			else
			{
				CLOG(DEBUG, "PPAC") << "Loaded PPAC " << (pathToDir + L"\\" + fdData.cFileName);
			}
		}
	} while (FindNextFileW(hFind, &fdData) != FALSE);
	dwError = GetLastError();
	FindClose(hFind);
	if (dwError != ERROR_NO_MORE_FILES)
	{
		CLOG(WARNING, "PPAC") << "Failed to load directory " << pathToDir << ": " << dwError;
		return false;
	}
	return true;
}

bool cPPACManager::FileExists(const TPUID& tpuid)
{
	return _tpuidToPPACMap.find(tpuid) != _tpuidToPPACMap.end();
}

std::unique_ptr<cPPACData> cPPACManager::GetFileData(const TPUID& tpuid)
{
	//	Find the PPAC that the entry belongs to
	auto iter = _tpuidToPPACMap.find(tpuid);
	if (iter != _tpuidToPPACMap.end())
	{
		//	Grab the handle
		PPACHANDLE hPPAC = iter->second;
		if (hPPAC != PPACHANDLE_INVALID)
		{
			//	Get the data
			return _loadedPPACs[hPPAC].GetFileData(tpuid);
		}
	}
	//	Failed
	return std::unique_ptr<cPPACData>(nullptr);
}
