#include "Utils.h"

bool Utils::ParseInt(std::string a) {
    std::regex integer("\\+?[[:digit:]]+");
    return (regex_match(a, integer))? true : false;
}

VRP* Utils::InitParameters(char **argv) {
    bool flag = true;
    VRP *v;
    /* open the file */
    FILE* fp = fopen(argv[1], "r");
    if (fp == NULL) {
        flag = false;
        fprintf(stderr,"The file %s doesn't exist.", argv[1]);
    }else {
        /* little buf more fread, big buff less fread big access time */
        char readBuffer[65536];
        rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
        this->d.ParseStream(is);
        fclose(fp);
        /* check error */
        if (this->d.HasParseError()) {
            flag = false;
            fprintf(stderr, "\nError(offset %u): %s\n",
                (unsigned)this->d.GetErrorOffset(),
                GetParseError_En(this->d.GetParseError()));
        }else {
            int numVertices = this->d["vertices"].Size();
            Customer *customers = new Customer[numVertices];
            for (int i = 0; i < numVertices; i++) {
                auto& item = d["vertices"][i];
                if (item["name"].IsString() && item["x"].IsInt() &&
                    item["y"].IsInt() && item["request"].IsInt() &&
                    item["time"].IsInt()) {
                        std::string n = this->d["vertices"][i]["name"].GetString();
                        int x = this->d["vertices"][i]["x"].GetInt();
                        int y = this->d["vertices"][i]["y"].GetInt();
                        int request = this->d["vertices"][i]["request"].GetInt();
                        int serviceTime = this->d["vertices"][i]["time"].GetInt();
                        customers[i] = Customer(n, x, y, request, serviceTime);
                }else {
                    flag = false;
                    fprintf(stderr, "Invalid file format!\n");
                }
            }
            if (flag && this->d["vehicles"].IsInt() &&
                this->d["capacity"].IsInt() && this->d["worktime"].IsInt()) {
                v = new VRP(
                    *customers,
                    numVertices,
                    this->d["vehicles"].GetInt(),
                    this->d["capacity"].GetInt(),
                    this->d["worktime"].GetInt());
            }
            else
                fprintf(stderr, "Invalid file format!\n");
        }
    }
    return (flag)? v : NULL;
}

FILE* Utils::SaveResult() {
    FILE* fp = fopen("output.json", "wb"); // non-Windows use "w"
    char writeBuffer[65536];
    rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
    rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
    d.Accept(writer);
    fclose(fp);
    return fp;
}