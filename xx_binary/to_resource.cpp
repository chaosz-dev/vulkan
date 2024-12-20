#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <filesystem>

namespace fs = std::filesystem;

std::string buildSafeName(const std::string &inputName) {
    std::string result = "BIN_";

    for (const char &ch : inputName) {
        switch (ch) {
            case '.': result += "_DOT_"; break;
            case '/': result += "_SLASH_"; break;
            case '\\': result+= "_BSLASH_"; break;
            case '+': result+= "_P_"; break;
            case '-': result+= "_M_"; break;
            case ' ': result+= "__"; break;
            default: result += ch; break;
        }
    }

    return result;
}

void addContentHeader(std::ofstream &output, const std::string &name) {
    output << "const uint8_t " << name << "[] = {" << std::endl;
}

void addContentFooter(std::ofstream &output, const std::string &name) {
    output << "}; /* " << name << " */" << std::endl;
}

uint32_t addContents(std::ofstream &output, const std::string &inputFileName) {
    std::ifstream input(inputFileName, std::fstream::binary);

    if (!input.is_open()) {
        return static_cast<uint32_t>(-1);
    }

    const char HEX[] = "0123456789ABCDEF";

    uint32_t writtenBytes = 0;
    uint8_t buffer[1024];
    while (!input.eof()) {
        input.read((char*)buffer, sizeof(buffer));

        int32_t readBytes = input.gcount();

        for (int32_t idx = 0; idx < readBytes; idx++) {
            output << "0x"
                   << HEX[buffer[idx] >> 4]
                   << HEX[buffer[idx] & 0x0F]
                   << ",";

            writtenBytes++;
            if (writtenBytes % 25 == 0) {
                output << std::endl;
            }
        }
    }

    return writtenBytes;
}

int main(int argc, const char **argv) {
    // to_resource.cpp output_file_base input_file_1 input_file_2...
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <output_file_base> input_file [input_file] [...]\n", argv[0]);
        return -1;
    }

    const std::string outputBaseName = std::string(argv[1]);
    const std::string outputFileCpp = outputBaseName + ".cpp";
    const std::string outputFileH = outputBaseName + ".h";

    std::map<std::string, std::string> resourceNames; // path -> name mapping

    std::ofstream outputCpp(outputFileCpp);
    outputCpp << "/* Generated by to_resource.cpp */" << std::endl;
    outputCpp << "#include " << fs::path(outputFileH).filename() << std::endl;
    outputCpp << "#include <cstdint>" << std::endl;
    outputCpp << "#include <cstring>" << std::endl;
    outputCpp << std::endl;

    for (int fileIdx = 2; fileIdx < argc; fileIdx++) {
        const std::string inputName = argv[fileIdx];
        const std::string inputShortName = fs::path(inputName).filename().string();
        const std::string safeName = buildSafeName(inputShortName);

        if (resourceNames.count(inputName) != 0) {
            fprintf(stderr, "Same file specifie multiple times: %s\n", inputName.c_str());
            return -2;
        }

        addContentHeader(outputCpp, safeName);

        const uint32_t bytesCount = addContents(outputCpp, inputName);

        if (bytesCount == static_cast<uint32_t>(-1)) {
            fprintf(stderr, "Incorrect file or file is too big (uint max): '%s'\n", inputName.c_str());
            return -2;
        }


        addContentFooter(outputCpp, inputName);

        outputCpp << "const uint32_t " << safeName << "_SIZE = " << bytesCount << ";" << std::endl;
        outputCpp << std::endl;

        resourceNames[inputShortName] = safeName;
    }

    outputCpp << "struct entry { const uint8_t *data; const uint32_t size; const char *name; } entries[] = {" << std::endl;
    for (const std::pair<std::string, std::string> entry : resourceNames) {
        outputCpp << " { " << entry.second << ", " << entry.second << "_SIZE, \"" << entry.first << "\"}," << std::endl;
    }
    outputCpp << "};" << std::endl;

    outputCpp << R"(
struct Resource GetResource(const char *name) {
    for (size_t idx = 0; idx < sizeof(entries) / sizeof(entries[0]); idx++) {
        const struct entry &item = entries[idx];
        if (strncmp(item.name, name, strlen(item.name)) == 0) {
            return {item.data, item.size};
        }
    }

    return {nullptr, 0};
}
)";


    std::ofstream outputH(outputFileH);

    outputH << R"(#pragma once
/* Generated by to_resource tool */
#include <cstdint>
struct Resource {
    const uint8_t *data;
    const uint32_t size;
};

struct Resource GetResource(const char *name);
)";


    return 0;
}
