//
//  ZYMP4Parser.cpp
//  MediaInfo
//
//  Created by mysong on 2023/11/1.
//

#include "ZYMP4Parser.hpp"

bool ReadAtomHead(ZYFileReader *fileReader, Mp4BoxHeader &head) {
    head.offset = fileReader->CurPos();
    head.size = fileReader->ReadUInt32();
    if (head.size == 0) {
        return false;
    } else if (head.size == 1) {
        head.largeSize = fileReader->ReadUInt64();
    }
    fileReader->ReadBytes(head.type, sizeof(head.type));
    return true;
}

std::shared_ptr<Mp4FtypBox>ReadFtypAtom(ZYFileReader *fileReader, Mp4BoxHeader *head) {
    std::shared_ptr<Mp4FtypBox> ftypAtom = std::make_shared<Mp4FtypBox>();
    if (head) {
        std::memcpy(&ftypAtom->head, head, sizeof(Mp4BoxHeader));
    } else {
        ReadAtomHead(fileReader, ftypAtom->head);
    }

    fileReader->ReadBytes(ftypAtom->majorBrand, sizeof(ftypAtom->majorBrand));
    ftypAtom->minorVersion = fileReader->ReadUInt32();
    
    fileReader->ReadBytes(ftypAtom->compatibleBrands, ftypAtom->head.RealSize() - 16);
    
    return ftypAtom;
}



ZYMP4Parser::ZYMP4Parser(const char *filePath)
{
    _fileReader = new ZYFileReader(filePath);
}

ZYMP4Parser::~ZYMP4Parser()
{
    if (_fileReader) {
        delete _fileReader;
    }
}

void ZYMP4Parser::PrintBox() {
    
    _fileReader->SeekTo(0);
    
    Parse(_fileReader);
}

void ZYMP4Parser::Parse(ZYFileReader *fileReader) {
    while (true) {
        Mp4BoxHeader head = {0};
        bool result = ReadAtomHead(fileReader, head);
        if (!result) {
            break;
        }
        printf("atom类型 %c%c%c%c\n", head.type[0], head.type[1], head.type[2], head.type[3]);
        switch (head.UIntType()) {
            case Mp4AtomType_ftyp:
                this->ftypeBox = ReadFtypAtom(_fileReader, &head);
                break;
            case Mp4AtomType_moov:
                this->ftypeBox = ReadFtypAtom(_fileReader, &head);
                break;
                
            default:
                break;
        }
        fileReader->SeekTo(head.offset + head.RealSize());
    }
    
}
//- (void)parseBox:()
