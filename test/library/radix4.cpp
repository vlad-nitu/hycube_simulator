#include <string.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <iostream>
#include "CGRA.h"

#define SIZE 512 // page table size
#define PHYS_MEM_SIZE 1024

uint64_t PML4[SIZE], PDPT[SIZE], PD[SIZE], PT[SIZE];
uint64_t physical_memory[PHYS_MEM_SIZE];
uint64_t va, pa;

int main() {
    // ---- 1. Set up test data as in 4radix.c ----
    va = 0x123456789ABCDEF0;
    pa = 0;

    int pml4_idx = (va >> (12 + 9 * 3)) & 0x1FF;
    int pdpt_idx = (va >> (12 + 9 * 2)) & 0x1FF;
    int pd_idx   = (va >> (12 + 9 * 1)) & 0x1FF;
    int pt_idx   = (va >> (12 + 9 * 0)) & 0x1FF;

    PML4[pml4_idx] = (uint64_t)&PDPT[0];
    PDPT[pdpt_idx] = (uint64_t)&PD[0];
    PD[pd_idx]     = (uint64_t)&PT[0];
    PT[pt_idx]     = (uint64_t)&physical_memory[0];

    // ---- 2. Load memory allocation map ----
    std::ifstream file("page_table_walk_mem_alloc.txt");
    std::map<std::string, int> base_addresses;
    std::string line;
    std::getline(file, line); // skip header

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string var;
        int addr;
        std::getline(iss, var, ',');
        iss >> addr;
        base_addresses[var] = addr;
    }

    // ---- 3. Setup simulator ----
    HyCUBESim::CGRA cgra(4, 4, 1, 4096);
    cgra.configCGRA("page_table_walk_PartPredDFG.xml_DP1_XDim=4_YDim=4_II=4_MTP=1_binary.bin", 4, 4);

    // ---- 4. Write data to CGRA DMEM ----
    for (const auto& [name, addr] : base_addresses) {
        std::vector<uint8_t> data;

        if (name == "va") {
            for (int i = 0; i < 8; ++i) data.push_back((va >> (8 * i)) & 0xFF);
            cgra.writeDMEM(cgra, addr, data.data(), 8);
        }
        if (name == "pa") {
            for (int i = 0; i < 8; ++i) data.push_back((pa >> (8 * i)) & 0xFF);
            cgra.writeDMEM(cgra, addr, data.data(), 8);
        }

        if (name == "PML4" || name == "PDPT" || name == "PD" || name == "PT") {
            uint64_t* table = nullptr;
            if (name == "PML4") table = PML4;
            if (name == "PDPT") table = PDPT;
            if (name == "PD") table = PD;
            if (name == "PT") table = PT;

            for (int i = 0; i < SIZE; ++i) {
                for (int j = 0; j < 8; ++j) {
                    data.push_back((table[i] >> (j * 8)) & 0xFF);
                }
            }
            cgra.writeDMEM(cgra, addr, data.data(), data.size());
        }

        if (name == "physical_memory") {
            for (int i = 0; i < PHYS_MEM_SIZE; ++i) {
                for (int j = 0; j < 8; ++j) {
                    data.push_back((physical_memory[i] >> (j * 8)) & 0xFF);
                }
            }
            cgra.writeDMEM(cgra, addr, data.data(), data.size());
        }

        if (name == "loopstart") {
            uint8_t d = 1;
            cgra.writeDMEM(cgra, addr, &d, 1);
        }

        if (name == "loopend") {
            uint8_t d = 0;
            cgra.writeDMEM(cgra, addr, &d, 1);
        }
    }

    // ---- 5. Run CGRA kernel ----
    cgra.invokeCGRA(cgra);

    // ---- 6. Read back physical address ----
    for (const auto& [name, addr] : base_addresses) {
        if (name == "pa") {
            uint8_t buf[8] = {};
            cgra.readDMEM(cgra, addr, buf, 8);
            uint64_t out = 0;
            for (int i = 0; i < 8; ++i) out |= (uint64_t(buf[i]) << (8 * i));
            std::cout << "Translated PA = 0x" << std::hex << out << std::dec << "\n";
        }
    }

    return 0;
}
