//
//  ZYMP4Parser.hpp
//  MediaInfo
//
//  Created by mysong on 2023/11/1.
//

#ifndef ZYMP4Parser_hpp
#define ZYMP4Parser_hpp

#include <stdio.h>
#include "QuickTimeAtomTypeDefine.h"
#include "QuickTimeDefine.h"
#include "ZYFileReader.hpp"

class ZYMP4Parser {
private:
    ZYFileReader *_fileReader;
    //bool ReadNormalAtom(ZYFileReader *fileReader, Mp4BoxHeader &head);
    //bool ReadFtypAtom(ZYFileReader *fileReader, Mp4FtypBox &box);
    //void ReadAllAtom(ZYFileReader *fileReader);
    
    std::shared_ptr<Mp4FtypBox> ftypeBox;
    void Parse(ZYFileReader *fileReader);
public:
    ZYMP4Parser(const char *filePath);
    ~ZYMP4Parser();
    
    void PrintBox();
};
#endif /* ZYMP4Parser_hpp */
