#include "Utils.h"

bool Utils::ParseInt(std::string a) {
    std::regex integer("\\+?[[:digit:]]+");
    return (regex_match(a, integer))? true : false;
}

VRP Utils::InitParameters(char **argv) {
    std::string s;
    VRP v;
    /* open the file */
    FILE* fp = fopen(argv[1], "r");
    if (fp == NULL) {
        s = "The file " + std::string(argv[1]) +  " doesn't exist.";
        throw s;
    }else {
        /* little buf more fread, big buff less fread big access time */
        char readBuffer[65536];
        rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
        this->d.ParseStream(is);
        fclose(fp);
        /* check error */
        if (this->d.HasParseError()) {
            s = "Error(offset" + std::string((char*)this->d.GetErrorOffset()) +
                "): " + GetParseError_En(this->d.GetParseError());
            throw s;
        }else {
            s = "Invalid file format!";
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
                    throw s;
                }
            }
            if (this->d["vehicles"].IsInt() &&
                this->d["capacity"].IsInt() && this->d["worktime"].IsInt()) {
                VRP v(
                    *customers,
                    numVertices,
                    this->d["vehicles"].GetInt(),
                    this->d["capacity"].GetInt(),
                    this->d["worktime"].GetInt());
            }else {
                throw s;
            }
        }
    }
    return v;
}

FILE* Utils::SaveResult() {
    FILE* fp = fopen("output.json", "w");
    if (fp != NULL) {
        char writeBuffer[65536];
        rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
        rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
        rapidjson::Document::AllocatorType& allocator = this->d.GetAllocator();
        rapidjson::Value route(rapidjson::kArrayType);
        for (int i = 0; i < 3; i++) {
            rapidjson::Value r(rapidjson::kArrayType);
            for (int k = 0; k < 10; k++) {
                int c = 1 + (rand() % (int)(8));
                r.PushBack(c, allocator);
            }
            route.PushBack(r, allocator);
        }
        d.AddMember("routes", route, allocator);
        d.Accept(writer);
        fclose(fp);
    }else {
        throw "Error writing file! (Bad permissions)\n";
    }
    return fp;
}

void Utils::logger(std::string s, int c) {
    switch(c) {
        case SUCCESS:
            std::cout << ANSI_GREEN << s << ANSI_RESET << std::endl;
        break;
        case WARNING:
            std::cout << ANSI_YELLOW << s << ANSI_RESET << std::endl;
        break;
        case ERROR:
            std::cout << ANSI_RED << s << ANSI_RESET << std::endl;
        break;
    }
}