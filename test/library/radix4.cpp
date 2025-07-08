#include <stdint.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include "CGRA.h"

#define LVLS        4
#define ENTRIES     512

/* -------- host-side test data ---------- */
static uint32_t PTBL[LVLS][ENTRIES];
static int      idx [LVLS];

/* ---------- helper: pack uint32_t → bytes little-endian ---------- */
static void push_u32(std::vector<uint8_t>& v, uint32_t w)
{
    for (int i = 0; i < 4; ++i) v.push_back(uint8_t(w >> (8 * i)));
}

int main()
{
    /* 1. dummy init – identical to the software kernel test */
    idx[3] = 0;  idx[2] = 1;  idx[1] = 2;  idx[0] = 3;
    PTBL[3][0] = 0;  PTBL[2][1] = 1;  PTBL[1][2] = 2;  PTBL[0][3] = 3;

    /* 2. load the memory-allocation map (word addresses) */
    std::ifstream f("page_table_walk_mem_alloc.txt");
    if (!f) { std::perror("map file"); return 1; }

    std::map<std::string, int> base;        // now stores BYTE addresses
    std::string line;  std::getline(f, line);               // skip header

    while (std::getline(f, line)) {
        std::istringstream iss(line);
        std::string var;  int addr_words;
        std::getline(iss, var, ',');  iss >> addr_words;

        base[var] = addr_words; // convert words → bytes
    }

    /* 3. configure the simulator */
    HyCUBESim::CGRA cgra(4, 4, 1, 16384);
    cgra.configCGRA(
        "page_table_walk_PartPredDFG.xml_DP1_XDim=4_YDim=4_II=5_MTP=1_binary.bin",
        4, 4);

    /* 4. write all required objects to DMEM */
    for (const auto& [name, addr] : base) {
        std::vector<uint8_t> bytes;

        if (name == "PTBL") {
            for (int r = 0; r < LVLS; ++r)
                for (int c = 0; c < ENTRIES; ++c)
                    push_u32(bytes, PTBL[r][c]);

            cgra.writeDMEM(cgra, addr, bytes.data(), bytes.size());
        }
        else if (name == "idx")  {
            for (int r = 0; r < LVLS; ++r) push_u32(bytes, idx[r]);
                cgra.writeDMEM(cgra, addr, bytes.data(), bytes.size());
        }
        else if (name == "loopstart") {
            uint8_t one = 1;
            cgra.writeDMEM(cgra, addr, &one, 1);
        }
        else if (name == "loopend") {
            uint8_t zero = 0;
            cgra.writeDMEM(cgra, addr, &zero, 1);
        }
    }

    /* 5. launch the kernel */
    cgra.invokeCGRA(cgra);

    // Read and print data for each variable
    for (const auto& pair : base_addresses) {
        if(pair.first == "loopstart" || pair.first == "loopend") continue;
        std::vector<uint8_t> byteData(SIZE * 4);
        cgraInstance.readDMEM(cgraInstance, pair.second, byteData.data(), byteData.size());

        std::cout << "Data for variable " << pair.first << ": ";
        for (int i = 0; i < SIZE; ++i) {
            int value = 0;
            for (int j = 0; j < 4; ++j) {
                value |= (byteData[i * 4 + j] << (j * 8));
            }
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
