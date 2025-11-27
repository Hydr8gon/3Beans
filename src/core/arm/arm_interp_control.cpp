/*
    Copyright 2023-2025 Hydr8gon

    This file is part of 3Beans.

    3Beans is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    3Beans is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with 3Beans. If not, see <https://www.gnu.org/licenses/>.
*/

#include "arm_interp.h"
#include "../core.h"

#if LOG_LEVEL > 3
// Define SVC names and parameter counts for the 3DS OS
static const struct {
    const char *name;
    uint8_t count;
} svcInfo[0x80] = {
    { "", 0 }, { "ControlMemory", 5 }, { "QueryMemory", 3 }, { "ExitProcess", 0 }, // 0x00-0x03
    { "GetProcessAffinityMask", 3 }, { "SetProcessAffinityMask", 3 }, // 0x04-0x05
    { "GetProcessIdealProcessor", 2 }, { "SetProcessIdealProcessor", 2 }, // 0x06-0x07
    { "CreateThread", 5 }, { "ExitThread", 0 }, { "SleepThread", 2 }, { "GetThreadPriority", 2 }, // 0x08-0x0B
    { "SetThreadPriority", 2 }, { "GetThreadAffinityMask", 3 }, // 0x0C-0x0D
    { "SetThreadAffinityMask", 3 }, { "GetThreadIdealProcessor", 2 }, // 0x0E-0x0F
    { "SetThreadIdealProcessor", 2 }, { "GetProcessorID", 0 }, { "Run", 2 }, { "CreateMutex", 2 }, // 0x10-0x13
    { "ReleaseMutex", 1 }, { "CreateSemaphore", 3 }, { "ReleaseSemaphore", 3 }, { "CreateEvent", 2 }, // 0x14-0x17
    { "SignalEvent", 1 }, { "ClearEvent", 1 }, { "CreateTimer", 2 }, { "SetTimer", 5 }, // 0x18-0x1B
    { "CancelTimer", 1 }, { "ClearTimer", 1 }, { "CreateMemoryBlock", 5 }, { "MapMemoryBlock", 4 }, // 0x1C-0x1F
    { "UnmapMemoryBlock", 2 }, { "CreateAddressArbiter", 1 }, // 0x20-0x21
    { "ArbitrateAddress", 6 }, { "CloseHandle", 1 }, // 0x22-0x23
    { "WaitSynchronization1", 4 }, { "WaitSynchronizationN", 6 }, // 0x24-0x25
    { "SignalAndWait", 8 }, { "DuplicateHandle", 2 }, // 0x26-0x27
    { "GetSystemTick", 0 }, { "GetHandleInfo", 3 }, { "GetSystemInfo", 3 }, { "GetProcessInfo", 3 }, // 0x28-0x2B
    { "GetThreadInfo", 3 }, { "ConnectToPort", 2 }, { "SendSyncRequest1", 1 }, { "SendSyncRequest2", 1 }, // 0x2C-0x2F
    { "SendSyncRequest3", 1 }, { "SendSyncRequest4", 1 }, { "SendSyncRequest", 1 }, { "OpenProcess", 2 }, // 0x30-0x33
    { "OpenThread", 3 }, { "GetProcessId", 2 }, { "GetProcessIdOfThread", 2 }, { "GetThreadId", 2 }, // 0x34-0x37
    { "GetResourceLimit", 2 }, { "GetResourceLimitLimitValues", 4 }, // 0x38-0x39
    { "GetResourceLimitCurrentValues", 4 }, { "GetThreadContext", 2 }, // 0x3A-0x3B
    { "Break", 1 }, { "OutputDebugString", 2 }, { "ControlPerformanceCounter", 6 }, { "", 0 }, // 0x3C-0x3F
    { "", 0 }, { "", 0 }, { "", 0 }, { "", 0 }, // 0x40-0x43
    { "", 0 }, { "", 0 }, { "", 0 }, { "CreatePort", 4 }, // 0x44-0x47
    { "CreateSessionToPort", 2 }, { "CreateSession", 2 }, // 0x48-0x49
    { "AcceptSession", 2 }, { "ReplyAndReceive1", 4 }, // 0x4A-0x4B
    { "ReplyAndReceive2", 4 }, { "ReplyAndReceive3", 4 }, // 0x4C-0x4D
    { "ReplyAndReceive4", 4 }, { "ReplyAndReceive", 4 }, // 0x4E-0x4F
    { "BindInterrupt", 4 }, { "UnbindInterrupt", 2 }, // 0x50-0x51
    { "InvalidateProcessDataCache", 3 }, { "StoreProcessDataCache", 3 }, // 0x52-0x53
    { "FlushProcessDataCache", 3 }, { "StartInterProcessDma", 7 }, { "StopDma", 1 }, { "GetDmaState", 2 }, // 0x54-0x57
    { "RestartDma", 5 }, { "SetGpuProt", 1 }, { "SetWifiEnabled", 1 }, { "", 0 }, // 0x58-0x5B
    { "", 0 }, { "", 0 }, { "", 0 }, { "", 0 }, // 0x5C-0x5F
    { "DebugActiveProcess", 2 }, { "BreakDebugProcess", 1 }, // 0x60-0x61
    { "TerminateDebugProcess", 1 }, { "GetProcessDebugEvent", 2 }, // 0x62-0x63
    { "ContinueDebugEvent", 2 }, { "GetProcessList", 3 }, // 0x64-0x65
    { "GetThreadList", 4 }, { "GetDebugThreadContext", 4 }, // 0x66-0x67
    { "SetDebugThreadContext", 4 }, { "QueryDebugProcessMemory", 4 }, // 0x68-0x69
    { "ReadProcessMemory", 4 }, { "WriteProcessMemory", 4 }, // 0x6A-0x6B
    { "SetHardwareBreakPoint", 3 }, { "GetDebugThreadParam", 5 }, { "", 0 }, { "", 0 }, // 0x6C-0x6F
    { "ControlProcessMemory", 6 }, { "MapProcessMemory", 3 }, // 0x70-0x71
    { "UnmapProcessMemory", 3 }, { "CreateCodeSet", 5 }, // 0x72-0x73
    { "RandomStub", 0 }, { "CreateProcess", 4 }, // 0x74-0x75
    { "TerminateProcess", 1 }, { "SetProcessResourceLimits", 2 }, // 0x76-0x77
    { "CreateResourceLimit", 1 }, { "SetResourceLimitLimitValues", 4 }, // 0x78-0x79
    { "AddCodeSegment", 2 }, { "Backdoor", 1 }, // 0x7A-0x7B
    { "KernelSetState", 1 }, { "QueryProcessMemory", 4 }, { "", 0 }, { "", 0 } // 0x7C-0x7F
};

// Define a macro to log SVC information when called
#define LOG_SVC(mask) \
    uint32_t svc = (opcode & mask); \
    std::string log = (id == ARM9) ? "ARM9" : ("ARM11 core " + std::to_string(id)); \
    if (svc < 0x80 && svcInfo[svc].name[0]) { \
        log = log + " calling svc" + svcInfo[svc].name + "("; \
        for (int i = 0; i < svcInfo[svc].count; i++) { \
            char buf[11]; \
            sprintf(buf, "0x%X", *registers[i]); \
            log = log + buf + ((i == svcInfo[svc].count - 1) ? "" : ", "); \
        } \
        LOG_OS("%s)\n", log.c_str()); \
    } \
    else { \
        LOG_OS("%s calling unknown SVC: 0x%X\n", log.c_str(), svc); \
    }
#else
#define LOG_SVC(mask)
#endif

int ArmInterp::bx(uint32_t opcode) { // BX Rn
    // Branch to address and switch to THUMB if bit 0 is set
    uint32_t op0 = *registers[opcode & 0xF];
    cpsr |= (op0 & BIT(0)) << 5;
    *registers[15] = op0;
    flushPipeline();
    return 3;
}

int ArmInterp::blxReg(uint32_t opcode) { // BLX Rn
    // Branch to address with link and switch to THUMB if bit 0 is set
    uint32_t op0 = *registers[opcode & 0xF];
    cpsr |= (op0 & BIT(0)) << 5;
    *registers[14] = *registers[15] - 4;
    *registers[15] = op0;
    flushPipeline();
    return 3;
}

int ArmInterp::b(uint32_t opcode) { // B label
    // Branch to offset
    int32_t op0 = (int32_t)(opcode << 8) >> 6;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bl(uint32_t opcode) { // BL label
    // Branch to offset with link
    int32_t op0 = (int32_t)(opcode << 8) >> 6;
    *registers[14] = *registers[15] - 4;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::blx(uint32_t opcode) { // BLX label
    // Branch to offset with link and switch to THUMB
    int32_t op0 = ((int32_t)(opcode << 8) >> 6) | ((opcode & BIT(24)) >> 23);
    cpsr |= BIT(5);
    *registers[14] = *registers[15] - 4;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::swi(uint32_t opcode) { // SWI #i
    // Software interrupt
    LOG_SVC(0xFFFFFF)
    *registers[15] -= 4;
    return exception(0x08);
}

int ArmInterp::bkpt(uint32_t opcode) { // BKPT
    // Software breakpoint
    if (id == ARM9)
        LOG_INFO("Triggering ARM9 software breakpoint\n");
    else
        LOG_INFO("Triggering ARM11 core %d software breakpoint\n", id);
    *registers[15] -= 4;
    return exception(0x0C);
}

int ArmInterp::pld(uint32_t opcode) { // PLD address
    // Ignore hints that an address will be accessed soon
    return 1;
}

int ArmInterp::cps(uint32_t opcode) { // CPS[IE/ID] AIF,#mode
    // Optionally enable or disable interrupt flags
    if (opcode & BIT(19)) {
        if (opcode & BIT(18)) // ID
            cpsr |= (opcode & 0xE0);
        else // IE
            cpsr &= ~(opcode & 0xE0);
    }

    // Optionally change the CPU mode
    if (opcode & BIT(17))
        setCpsr((cpsr & ~0x1F) | (opcode & 0x1F));
    return 1;
}

int ArmInterp::srs(uint32_t opcode) { // SRS[DA/IA/DB/IB] sp!,#mode
    // Get the stack pointer for the specified CPU mode
    uint32_t *op0;
    switch (opcode & 0x1F) {
        case 0x11: op0 = &registersFiq[5]; break; // FIQ
        case 0x12: op0 = &registersIrq[0]; break; // IRQ
        case 0x13: op0 = &registersSvc[0]; break; // Supervisor
        case 0x17: op0 = &registersAbt[0]; break; // Abort
        case 0x1B: op0 = &registersUnd[0]; break; // Undefined
        default: op0 = &registersUsr[13]; break; // User/System
    }

    // Store return state based on the addressing mode
    switch ((opcode >> 23) & 0x3) {
    case 0x0: // DA
        core->cp15.write<uint32_t>(id, *op0 - 4, *registers[14]);
        core->cp15.write<uint32_t>(id, *op0 - 0, spsr ? *spsr : 0);
        if (opcode & BIT(21)) *op0 -= 8; // Writeback
        return 1;

    case 0x1: // IA
        core->cp15.write<uint32_t>(id, *op0 + 0, *registers[14]);
        core->cp15.write<uint32_t>(id, *op0 + 4, spsr ? *spsr : 0);
        if (opcode & BIT(21)) *op0 += 8; // Writeback
        return 1;

    case 0x2: // DB
        core->cp15.write<uint32_t>(id, *op0 - 8, *registers[14]);
        core->cp15.write<uint32_t>(id, *op0 - 4, spsr ? *spsr : 0);
        if (opcode & BIT(21)) *op0 -= 8; // Writeback
        return 1;

    case 0x3: // IB
        core->cp15.write<uint32_t>(id, *op0 + 4, *registers[14]);
        core->cp15.write<uint32_t>(id, *op0 + 8, spsr ? *spsr : 0);
        if (opcode & BIT(21)) *op0 += 8; // Writeback
        return 1;
    }
}

int ArmInterp::rfe(uint32_t opcode) { // RFE[DA/IA/DB/IB] Rn!
    // Return from exception based on the addressing mode
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    switch ((opcode >> 23) & 0x3) {
    case 0x0: // DA
        *registers[15] = core->cp15.read<uint32_t>(id, *op0 - 4);
        setCpsr(core->cp15.read<uint32_t>(id, *op0 - 0));
        if (opcode & BIT(21)) *op0 -= 8; // Writeback
        break;

    case 0x1: // IA
        *registers[15] = core->cp15.read<uint32_t>(id, *op0 + 0);
        setCpsr(core->cp15.read<uint32_t>(id, *op0 + 4));
        if (opcode & BIT(21)) *op0 += 8; // Writeback
        break;

    case 0x2: // DB
        *registers[15] = core->cp15.read<uint32_t>(id, *op0 - 8);
        setCpsr(core->cp15.read<uint32_t>(id, *op0 - 4));
        if (opcode & BIT(21)) *op0 -= 8; // Writeback
        break;

    case 0x3: // IB
        *registers[15] = core->cp15.read<uint32_t>(id, *op0 + 4);
        setCpsr(core->cp15.read<uint32_t>(id, *op0 + 8));
        if (opcode & BIT(21)) *op0 += 8; // Writeback
        break;
    }

    // Handle pipelining
    flushPipeline();
    return 1;
}

int ArmInterp::wfi(uint32_t opcode) {
    // Halt the CPU until an interrupt occurs
    core->interrupts.halt(id);
    return 1;
}

int ArmInterp::wfe(uint32_t opcode) {
    // Halt the CPU until the event flag is set or an interrupt occurs
    if (!event)
        core->interrupts.halt(id, 1);
    event = false;
    return 1;
}

int ArmInterp::sev(uint32_t opcode) {
    // Set the event flag for other CPUs or unhalt them if already waiting
    uint8_t cores = ~BIT(id) & 0xF;
    for (int i = 0; cores >> i; i++) {
        if (~cores & BIT(i)) continue;
        if (core->arms[i].halted & BIT(1))
            core->arms[i].unhalt(BIT(1));
        else
            core->arms[i].event = true;
    }
    return 1;
}

int ArmInterp::bxRegT(uint16_t opcode) { // BX Rs
    // Branch to address and switch to ARM mode if bit 0 is cleared (THUMB)
    uint32_t op0 = *registers[(opcode >> 3) & 0xF];
    cpsr &= ~((~op0 & BIT(0)) << 5);
    *registers[15] = op0;
    flushPipeline();
    return 3;
}

int ArmInterp::blxRegT(uint16_t opcode) { // BLX Rs
    // Branch to address with link and switch to ARM mode if bit 0 is cleared (THUMB)
    uint32_t op0 = *registers[(opcode >> 3) & 0xF];
    cpsr &= ~((~op0 & BIT(0)) << 5);
    *registers[14] = *registers[15] - 1;
    *registers[15] = op0;
    flushPipeline();
    return 3;
}

int ArmInterp::beqT(uint16_t opcode) { // BEQ label
    // Branch to offset if equal (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (~cpsr & BIT(30)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bneT(uint16_t opcode) { // BNE label
    // Branch to offset if not equal (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (cpsr & BIT(30)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bcsT(uint16_t opcode) { // BCS label
    // Branch to offset if carry set (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (~cpsr & BIT(29)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bccT(uint16_t opcode) { // BCC label
    // Branch to offset if carry clear (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (cpsr & BIT(29)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bmiT(uint16_t opcode) { // BMI label
    // Branch to offset if negative (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (~cpsr & BIT(31)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bplT(uint16_t opcode) { // BPL label
    // Branch to offset if positive (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (cpsr & BIT(31)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bvsT(uint16_t opcode) { // BVS label
    // Branch to offset if overflow set (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (~cpsr & BIT(28)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bvcT(uint16_t opcode) { // BVC label
    // Branch to offset if overflow clear (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (cpsr & BIT(28)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bhiT(uint16_t opcode) { // BHI label
    // Branch to offset if higher (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if ((cpsr & 0x60000000) != 0x20000000) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::blsT(uint16_t opcode) { // BLS label
    // Branch to offset if lower or same (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if ((cpsr & 0x60000000) == 0x20000000) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bgeT(uint16_t opcode) { // BGE label
    // Branch to offset if signed greater or equal (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if ((cpsr ^ (cpsr << 3)) & BIT(31)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bltT(uint16_t opcode) { // BLT label
    // Branch to offset if signed less than (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (~(cpsr ^ (cpsr << 3)) & BIT(31)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bgtT(uint16_t opcode) { // BGT label
    // Branch to offset if signed greater than (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (((cpsr ^ (cpsr << 3)) | (cpsr << 1)) & BIT(31)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bleT(uint16_t opcode) { // BLE label
    // Branch to offset if signed less or equal (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (~((cpsr ^ (cpsr << 3)) | (cpsr << 1)) & BIT(31)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bT(uint16_t opcode) { // B label
    // Branch to offset (THUMB)
    int32_t op0 = (int16_t)(opcode << 5) >> 4;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::blSetupT(uint16_t opcode) { // BL/BLX label
    // Set the upper 11 bits of the target address for a long BL/BLX (THUMB)
    int32_t op0 = (int16_t)(opcode << 5) >> 4;
    *registers[14] = *registers[15] + (op0 << 11);
    return 1;
}

int ArmInterp::blOffT(uint16_t opcode) { // BL label
    // Long branch to offset with link (THUMB)
    uint32_t op0 = (opcode & 0x7FF) << 1;
    uint32_t ret = *registers[15] - 1;
    *registers[15] = *registers[14] + op0;
    *registers[14] = ret;
    flushPipeline();
    return 3;
}

int ArmInterp::blxOffT(uint16_t opcode) { // BLX label
    // Long branch to offset with link and switch to ARM mode (THUMB)
    uint32_t op0 = (opcode & 0x7FF) << 1;
    cpsr &= ~BIT(5);
    uint32_t ret = *registers[15] - 1;
    *registers[15] = *registers[14] + op0;
    *registers[14] = ret;
    flushPipeline();
    return 3;
}

int ArmInterp::swiT(uint16_t opcode) { // SWI #i
    // Software interrupt (THUMB)
    LOG_SVC(0xFF)
    *registers[15] -= 4;
    return exception(0x08);
}

int ArmInterp::bkptT(uint16_t opcode) { // BKPT
    // Software breakpoint (THUMB)
    if (id == ARM9)
        LOG_INFO("Triggering ARM9 software breakpoint\n");
    else
        LOG_INFO("Triggering ARM11 core %d software breakpoint\n", id);
    *registers[15] -= 4;
    return exception(0x0C);
}
