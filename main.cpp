#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using Byte = unsigned char;
using Word = unsigned short;

using u32 = unsigned int;

struct Mem {
    static constexpr u32 MAX_MEM = 1024 * 64;
    Byte Data[MAX_MEM];

    void Initialize() {
        for (u32 i = 0; i < MAX_MEM; i++) {
            Data[i] = 0;
        }
    }

    // read 1 byte at a time
    Byte operator[](u32 Address) const {
        return Data[Address]; // assert address is less than max mem
    }

    // write 1 byte at a time
    Byte& operator[](u32 Address) {
        return Data[Address]; // assert address is less than max mem
    }

    // write 2 bytes
    void WriteWord(Word value, u32 address, u32& cycles) {
        Data[address] = value & 0xFF;
        Data[address + 1] = (value >> 8);
        cycles -= 2;
    }
};

struct CPU {
    Word PC; // program counter
    Word SP; // stack pointer, should be byte

    Byte A, X, Y; // registers

    // status flags
    Byte C : 1;
    Byte Z : 1;
    Byte I : 1;
    Byte D : 1;
    Byte B : 1;
    Byte V : 1;
    Byte N : 1;

    // boot up function
    void Reset(Mem& memory) {
        PC = 0xFFFC;
        SP = 0x0100;
        C = Z = I = D = B = V = N = A = X = Y = 0;
        memory.Initialize();
    }

    Byte FetchByte(u32& cycles, Mem& memory) {
        Byte Data = memory[PC];
        PC++;
        cycles--;
        return Data;
    }

    Word FetchWord(u32& cycles, Mem& memory) {
        Word Data = memory[PC];
        PC++;

        Data |= (memory[PC] << 8);
        PC++;

        cycles-=2;

        return Data;
    }

    Byte ReadByte(u32& cycles, Byte address, Mem& memory) {
        Byte Data = memory[address];
        cycles--;
        return Data;
    }


    static constexpr Byte 
        INS_LDA_IM = 0xA9, 
        INS_LDA_ZP = 0xA5, 
        INS_LDA_ZPX = 0xB5, 
        INS_JSR = 0x20;

    void LDASetStatus() {
        Z = (A == 0);
        N = (A & 0b10000000) > 0;
    }

    // execute instruction for number of cycles
    void Execute(u32 cycles, Mem& memory) {
        while (cycles > 0) {
            Byte Instructions = FetchByte(cycles, memory);
            switch (Instructions) {
                case INS_LDA_IM: {
                    Byte Value = FetchByte(cycles, memory);
                    A = Value;
                    LDASetStatus();
                } break;
                case INS_LDA_ZP: {
                    Byte zeroPageAddress = FetchByte(cycles, memory);
                    A = ReadByte(cycles, zeroPageAddress, memory);
                    LDASetStatus();
                } break;
                case INS_LDA_ZPX: {
                    Byte zeroPageAddress = FetchByte(cycles, memory);
                    zeroPageAddress += X;
                    cycles--;
                    A = ReadByte(cycles, zeroPageAddress, memory);
                    LDASetStatus();
                }
                case INS_JSR: {
                    Word subAddr = FetchWord(cycles, memory);
                    memory.WriteWord(PC - 1, SP, cycles);
                    SP++; // increment stack pointer
                    PC = subAddr;
                    cycles--;
                }
                default: {
                    std::cout << "Instruction not handled %d\n" << Instructions;
                } break;
            }
        }
    }
};

int main() {
    Mem mem;
    CPU cpu;
    cpu.Reset(mem);

    mem[0xFFFC] = CPU::INS_JSR;
    mem[0xFFFD] = 0x42;
    mem[0xFFFE] = 0x42;
    mem[0x4242] = CPU::INS_LDA_IM;
    mem[0x4243] = 0x84;

    cpu.Execute(9, mem);

    return 0;
}