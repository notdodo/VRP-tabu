#include "Utils.h"

Utils::Utils() {
}

bool Utils::ParseInt(std::string a) {
    std::regex integer("\\+?[[:digit:]]+");
    return (regex_match(a, integer))? true : false;
}

bool Utils::ParseInput(char **argv) {
    /* open the file */
    FILE* fp = fopen(argv[1], "r");
    if (fp == NULL)
        fprintf(stderr,"The file %s doesn't exist.",argv[1]);
    /* prepare the buffer: little buf more fread, big buff less fread big access time */
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    rapidjson::Document d;
    d.ParseStream(is);
    fclose(fp);
    /* check error */
    if (d.HasParseError()) {
        std::cout << d.GetParseError() << std::endl;
    }
    const int numVertices = d["vertices"].Size();
    /* servirà una classe come p2d per i vertici e una per le coord
    *  una classe v2d per le distanze? (non penso)
    */
    for (int i = 0; i < numVertices; i++) {
        std::cout << d["vertices"][i]["name"].GetString();
        std::cout << ": ";
        std::cout << d["vertices"][i]["coord"].GetString() << std::endl;
    }
    return true;
}