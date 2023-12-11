//
//  main.cpp
//  MediaInfo
//
//  Created by mys on 2022/8/18.
//

#include <iostream>
#include "ZYFLVParser.hpp"
#include "ZYMP4Parser.hpp"

int main(int argc, const char * argv[]) {
    // insert code here...
    const char *path = argv[1];
//    ZYFLVParser *parser = new ZYFLVParser(path);
//    parser->PrintTagInfo();
//    delete parser;
    ZYMP4Parser *mp4Parser = new ZYMP4Parser(path);
    mp4Parser->PrintBox();
    return 0;
}



