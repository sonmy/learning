//
//  ZYFileReader.cpp
//  MediaInfo
//
//  Created by mys on 2022/8/18.
//

#include "ZYFileReader.hpp"

ZYFileReader::ZYFileReader(std::string filePath)
{
    this->_fileHandle = fopen(filePath.c_str(), "rb");
}

bool ZYFileReader::Seek(long offset)
{
    int ret = fseek(this->_fileHandle,  offset, SEEK_CUR);
    if (ret != 0) {
        this->errCode = errno;
        return false;
    }
    return true;
}

bool ZYFileReader::SeekTo(long offset)
{
    int ret = fseek(this->_fileHandle, offset, SEEK_SET);
    if (ret != 0) {
        this->errCode = errno;
        return false;
    }
    return true;
}

std::shared_ptr<struct ZYBufferItem> ZYFileReader::CopyNextBuffer(size_t size, int *error)
{
    char *buf = (char *)malloc(size);
    size_t readSize = fread(buf, size, 1, this->_fileHandle);
    if (readSize > 0) {
        std::shared_ptr<struct ZYBufferItem> value = std::make_shared<ZYBufferItem>(buf, readSize*size);
        return value;
    }
    return nullptr;
}

size_t ZYFileReader::ReadBytes(void *buf, size_t size)
{
    size_t readSize = fread(buf, size, 1, this->_fileHandle);
    return readSize;
}


double ZYFileReader::ReadFloat64()
{
    uint64_t value;
    size_t readSize = fread(&value, sizeof(value), 1, this->_fileHandle);
    if (readSize == 1) {
        value = ntohll(value);
        return *((double *)(&value));
    }
    return 0;
}

float ZYFileReader::ReadFloat32()
{
    uint32_t value;
    size_t readSize = fread(&value, sizeof(value), 1, this->_fileHandle);
    if (readSize == 1) {
        value = ntohl(value);
        return *((float *)(&value));
    }
    return 0;
}

uint64_t ZYFileReader::ReadUInt64()
{
    uint64_t value;
    size_t readSize = fread(&value, sizeof(value), 1, this->_fileHandle);
    if (readSize == 1) {
        return ntohll(value);
    }
    return 0;
}

uint32_t ZYFileReader::ReadUInt32()
{
    uint32_t value;
    size_t readSize = fread(&value, sizeof(value), 1, this->_fileHandle);
    if (readSize == 1) {
        return ntohl(value);
    }
    return 0;
}

uint16_t ZYFileReader::ReadUInt16()
{
    uint16_t value;
    size_t readSize = fread(&value, sizeof(value), 1, this->_fileHandle);
    if (readSize == 1) {
        return ntohs(value);
    }
    return 0;
}
uint8_t ZYFileReader::ReadUInt8()
{
    uint8_t value;
    size_t readSize = fread(&value, sizeof(value), 1, this->_fileHandle);
    if (readSize == 1) {
        return value;
    }
    return 0;
}

size_t ZYFileReader::CurPos()
{
    return ftell(this->_fileHandle);
}

bool ZYFileReader::isEOF(void)
{
    return feof(this->_fileHandle);
}

uint32_t ZYFileReader::Read3BytesUInt()
{
    uint32_t value = 0;
    uint8_t buf[3];
    fread(buf, sizeof(buf), 1, this->_fileHandle);
    value |= buf[0] << 16;
    value |= buf[1] << 8;
    value |= buf[2];
    return value;
}
