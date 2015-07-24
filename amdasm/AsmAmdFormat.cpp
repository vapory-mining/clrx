/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <CLRX/Config.h>
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

static const char* amdPseudoOpNamesTbl[] =
{
    "arg", "boolconsts", "calnote", "cbid",
    "cbmask", "compile_options", "condout", "config",
    "constantbuffers", "cws", "driver_info", "driver_version",
    "earlyexit", "entry", "floatconsts", "floatmode",
    "globalbuffers", "globaldata", "header", "hwlocal",
    "hwregion", "ieeemode", "inputs", "inputsamplers",
    "intconsts", "metadata", "outputs", "persistentbuffers",
    "pgmrsrc2", "printfid", "privateid", "proginfo",
    "sampler", "scratchbuffer", "scratchbuffers", "segment",
    "sgprsnum", "subconstantbuffers", "uav", "uavid",
    "uavmailboxsize", "uavopmask", "uavprivate", "useconstdata",
    "useprintf", "userdata", "vgprsnum"
};

enum
{
    AMDOP_ARG = 0, AMDOP_BOOLCONSTS, AMDOP_CALNOTE, AMDOP_CBID,
    AMDOP_CBMASK, AMDOP_COMPILE_OPTIONS, AMDOP_CONDOUT, AMDOP_CONFIG,
    AMDOP_CONSTANTBUFFERS, AMDOP_CWS, AMDOP_DRIVER_INFO, AMDOP_DRIVER_VERSION,
    AMDOP_EARLYEXIT, AMDOP_ENTRY, AMDOP_FLOATCONSTS, AMDOP_FLOATMODE,
    AMDOP_GLOBALBUFFERS, AMDOP_GLOBALDATA, AMDOP_HEADER, AMDOP_HWLOCAL,
    AMDOP_HWREGION, AMDOP_IEEEMODE, AMDOP_INPUTS, AMDOP_INPUTSAMPLERS,
    AMDOP_INTCONSTS, AMDOP_METADATA, AMDOP_OUTPUTS, AMDOP_PERSISTENTBUFFERS,
    AMDOP_PGMRSRC2, AMDOP_PRINTFID, AMDOP_PRIVATEID, AMDOP_PROGINFO,
    AMDOP_SAMPLER, AMDOP_SCRATCHBUFFER, AMDOP_SCRATCHBUFFERS, AMDOP_SEGMENT,
    AMDOP_SGPRSNUM, AMDOP_SUBCONSTANTBUFFERS, AMDOP_UAV, AMDOP_UAVID,
    AMDOP_UAVMAILBOXSIZE, AMDOP_UAVOPMASK, AMDOP_UAVPRIVATE, AMDOP_USECONSTDATA,
    AMDOP_USEPRINTF, AMDOP_USERDATA, AMDOP_VGPRSNUM
};

/*
 * AmdCatalyst format handler
 */

AsmAmdHandler::AsmAmdHandler(Assembler& assembler) : AsmFormatHandler(assembler),
                output{}, dataSection(0), extraSectionCount(0)
{
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
    sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::DATA, ELFSECTID_UNDEF, nullptr });
    savedSection = 0;
}

void AsmAmdHandler::saveCurrentSection()
{   /// save previous section
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    else
        kernelStates[assembler.currentKernel].savedSection = assembler.currentSection;
}

cxuint AsmAmdHandler::addKernel(const char* kernelName)
{
    cxuint thisKernel = output.kernels.size();
    cxuint thisSection = sections.size();
    output.addEmptyKernel(kernelName);
    Kernel kernel{ ASMSECT_NONE, ASMSECT_NONE, ASMSECT_NONE,
            thisSection, ASMSECT_NONE };
    kernel.extraSectionCount = 0;
    kernelStates.push_back(std::move(kernel));
    sections.push_back({ thisKernel, AsmSectionType::CODE, ELFSECTID_TEXT, ".text" });
    
    saveCurrentSection();
    
    assembler.currentKernel = thisKernel;
    assembler.currentSection = thisSection;
    return thisKernel;
}

cxuint AsmAmdHandler::addSection(const char* sectionName, cxuint kernelId)
{
    const cxuint thisSection = sections.size();
    Section section;
    section.kernelId = kernelId;
    if (::strcmp(sectionName, ".data") == 0) // data
    {
        if (kernelId == ASMKERN_GLOBAL)
            throw AsmFormatException("Section '.data' permitted only inside kernels");
        kernelStates[kernelId].dataSection = thisSection;
        section.type = AsmSectionType::DATA;
        section.elfBinSectId = ELFSECTID_DATA;
        section.name = ".data"; // set static name (available by whole lifecycle)*/
    }
    else if (::strcmp(sectionName, ".text") == 0) // code
    {
        if (kernelId == ASMKERN_GLOBAL)
            throw AsmFormatException("Section '.text' permitted only inside kernels");
        kernelStates[kernelId].codeSection = thisSection;
        section.type = AsmSectionType::CODE;
        section.elfBinSectId = ELFSECTID_TEXT;
        section.name = ".text"; // set static name (available by whole lifecycle)*/
    }
    else if (kernelId == ASMKERN_GLOBAL)
    {
        auto out = extraSectionMap.insert(std::make_pair(std::string(sectionName),
                    thisSection));
        if (!out.second)
            throw AsmFormatException("Section is already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = extraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    else
    {   /* inside kernel binary */
        if (kernelId >= kernelStates.size())
            throw AsmFormatException("KernelId out of range");
        Kernel& kernelState = kernelStates[kernelId];
        auto out = kernelState.extraSectionMap.insert(std::make_pair(
                    std::string(sectionName), thisSection));
        if (!out.second)
            throw AsmFormatException("Section is already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = kernelState.extraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    sections.push_back(section);
    
    saveCurrentSection();
    
    assembler.currentKernel = kernelId;
    assembler.currentSection = thisSection;
    return thisSection;
}

cxuint AsmAmdHandler::getSectionId(const char* sectionName) const
{
    if (assembler.currentKernel == ASMKERN_GLOBAL)
    {
        SectionMap::const_iterator it = extraSectionMap.find(sectionName);
        if (it != extraSectionMap.end())
            return it->second;
        return ASMSECT_NONE;
    }
    else
    {
        const Kernel& kernelState = kernelStates[assembler.currentKernel];
        if (::strcmp(sectionName, ".text") == 0)
            return kernelState.codeSection;
        else if (::strcmp(sectionName, ".data") == 0)
            return kernelState.dataSection;
        else
        {
            SectionMap::const_iterator it = kernelState.extraSectionMap.find(sectionName);
            if (it != extraSectionMap.end())
                return it->second;
        }
        return ASMSECT_NONE;
    }
}

void AsmAmdHandler::setCurrentKernel(cxuint kernel)
{
    if (kernel != ASMKERN_GLOBAL && kernel >= kernelStates.size())
        throw AsmFormatException("KernelId out of range");
        
    saveCurrentSection();
    assembler.currentKernel = kernel;
    if (kernel != ASMKERN_GLOBAL)
        assembler.currentSection = kernelStates[kernel].savedSection;
    else
        assembler.currentSection = savedSection;
}

void AsmAmdHandler::setCurrentSection(cxuint sectionId)
{
    if (sectionId >= sections.size())
        throw AsmFormatException("SectionId out of range");
    
    saveCurrentSection();
    assembler.currentKernel = sections[sectionId].kernelId;
    assembler.currentSection = sectionId;
}

AsmFormatHandler::SectionInfo AsmAmdHandler::getSectionInfo(cxuint sectionId) const
{   /* find section */
    if (sectionId >= sections.size())
        throw AsmFormatException("Section doesn't exists");
    AsmFormatHandler::SectionInfo info;
    info.type = sections[sectionId].type;
    info.flags = 0;
    if (info.type == AsmSectionType::CODE)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE;
    else if (info.type != AsmSectionType::CONFIG)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
    info.name = sections[sectionId].name;
    return info;
}

namespace CLRX
{

void AsmAmdPseudoOps::setCompileOptions(AsmAmdHandler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    std::string out;
    if (!asmr.parseString(out, linePtr))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.compileOptions = out;
}

void AsmAmdPseudoOps::setDriverInfo(AsmAmdHandler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    std::string out;
    if (!asmr.parseString(out, linePtr))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.driverInfo = out;
}

void AsmAmdPseudoOps::setDriverVersion(AsmAmdHandler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t value;
    if (!getAbsoluteValueArg(asmr, value, linePtr, true))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.driverVersion = value;
}

void AsmAmdPseudoOps::doGlobalData(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (handler.dataSection==ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ ASMKERN_GLOBAL,  AsmSectionType::DATA,
            ELFSECTID_UNDEF, nullptr });
        handler.dataSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, handler.dataSection);
}

void AsmAmdPseudoOps::addMetadata(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL)
    {
        asmr.printError(pseudoOpPlace, "Metadata can be defined only inside kernel");
        return;
    }
    if (handler.kernelStates[asmr.currentKernel].configSection!=ASMSECT_NONE)
    {
        asmr.printError(pseudoOpPlace,
                    "Metadata can't be defined if configuration defined");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint& metadataSection = handler.kernelStates[asmr.currentKernel].metadataSection;
    if (metadataSection == ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel, AsmSectionType::AMD_METADATA,
            ELFSECTID_UNDEF, nullptr });
        metadataSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, metadataSection);
}

void AsmAmdPseudoOps::doConfig(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL)
    {
        asmr.printError(pseudoOpPlace, "Kernel config can be defined only inside kernel");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    AsmAmdHandler::Kernel& kernel = handler.kernelStates[asmr.currentKernel];
    if (kernel.metadataSection!=ASMSECT_NONE || kernel.headerSection!=ASMSECT_NONE ||
        !kernel.calNoteSections.empty())
    {
        asmr.printError(pseudoOpPlace, "Config can't be defined if metadata,header and/or"
                        " CALnotes section exists");
        return;
    }

    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
        
    if (kernel.configSection == ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel, AsmSectionType::CONFIG,
            ELFSECTID_UNDEF, nullptr });
        kernel.configSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, kernel.configSection);
    handler.output.kernels[asmr.currentKernel].useConfig = true;
}

static const uint32_t singleValueCALNotesMask =
        (1U<<CALNOTE_ATI_EARLYEXIT) | (1U<<CALNOTE_ATI_CONDOUT) |
        (1U<<CALNOTE_ATI_UAV_OP_MASK) | (1U<<CALNOTE_ATI_UAV_MAILBOX_SIZE);

void AsmAmdPseudoOps::addCALNote(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, uint32_t calNoteId)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL)
    {
        asmr.printError(pseudoOpPlace, "CALNote can be defined only inside kernel");
        return;
    }
    AsmAmdHandler::Kernel& kernel = handler.kernelStates[asmr.currentKernel];
    if (kernel.configSection!=ASMSECT_NONE)
    {
        asmr.printError(pseudoOpPlace, "CALNote can't be defined if configuration defined");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    uint64_t value = 0;
    const char* valuePlace = linePtr;
    const bool singleValue = calNoteId < 32 &&
            (singleValueCALNotesMask & (1U<<calNoteId)) && linePtr != end;
    if (singleValue)
    {
        if (!getAbsoluteValueArg(asmr, value, linePtr, false))
            return; // error
        asmr.printWarningForRange(32, value, asmr.getSourcePos(valuePlace));
    }
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // always add new CALnote
    const cxuint thisSection = handler.sections.size();
    handler.sections.push_back({ asmr.currentKernel, AsmSectionType::AMD_CALNOTE,
            ELFSECTID_UNDEF, nullptr, calNoteId });
    kernel.calNoteSections.push_back({thisSection});
    asmr.goToSection(pseudoOpPlace, thisSection);
    if (singleValue)
    {   // with single value
        uint32_t outValue = LEV(uint32_t(value));
        asmr.putData(4, (const cxbyte*)&outValue);
    }
}

void AsmAmdPseudoOps::addHeader(AsmAmdHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL)
    {
        asmr.printError(pseudoOpPlace, "Header can be defined only inside kernel");
        return;
    }
    if (handler.kernelStates[asmr.currentKernel].configSection!=ASMSECT_NONE)
    {
        asmr.printError(pseudoOpPlace, "Header can't be defined if configuration defined");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint& headerSection = handler.kernelStates[asmr.currentKernel].headerSection;
    if (headerSection == ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel, AsmSectionType::AMD_HEADER,
            ELFSECTID_UNDEF, nullptr });
        headerSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, headerSection);
}

void AsmAmdPseudoOps::doEntry(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, uint32_t requiredCalNoteIdMask)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    /* check place where is entry */
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        handler.sections[asmr.currentSection].type != AsmSectionType::AMD_CALNOTE ||
        (handler.sections[asmr.currentSection].extraId >= 32 ||
        ((1U<<handler.sections[asmr.currentSection].extraId) & requiredCalNoteIdMask))==0)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of entry");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    const char* value1Place = linePtr;
    uint64_t value1, value2;
    bool good = getAbsoluteValueArg(asmr, value1, linePtr, true);
    
    asmr.printWarningForRange(32, value1, asmr.getSourcePos(value1Place));
    
    if (!skipRequiredComma(asmr, linePtr, "second value"))
        return;
    const char* value2Place = linePtr;
    good &= getAbsoluteValueArg(asmr, value2, linePtr, true);
    asmr.printWarningForRange(32, value2, asmr.getSourcePos(value2Place));
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    uint32_t outVals[2];
    SLEV(outVals[0], value1);
    SLEV(outVals[1], value2);
    asmr.putData(8, (const cxbyte*)outVals);
}

void AsmAmdPseudoOps::setConfigValue(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, AmdConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    const char* valuePlace = linePtr;
    uint64_t value;
    bool good = getAbsoluteValueArg(asmr, value, linePtr, true);
    asmr.printWarningForRange(32, value, asmr.getSourcePos(valuePlace));
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AmdKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    // set value
    switch(target)
    {
        case AMDCVAL_SAMPLER:
            config.samplers.push_back(value);
            break;
        case AMDCVAL_SGPRSNUM:
            config.usedSGPRsNum = value;
            break;
        case AMDCVAL_VGPRSNUM:
            config.usedVGPRsNum = value;
            break;
        case AMDCVAL_PGMRSRC2:
            config.pgmRSRC2 = value;
            break;
        case AMDCVAL_IEEEMODE:
            config.ieeeMode = value;
            break;
        case AMDCVAL_FLOATMODE:
            config.floatMode = value;
            break;
        case AMDCVAL_HWLOCAL:
            config.hwLocalSize = value;
            break;
        case AMDCVAL_HWREGION:
            config.hwRegion = value;
            break;
        case AMDCVAL_PRIVATEID:
            config.privateId = value;
            break;
        case AMDCVAL_SCRATCHBUFFER:
            config.scratchBufferSize = value;
            break;
        case AMDCVAL_UAVPRIVATE:
            config.uavPrivate = value;
            break;
        case AMDCVAL_UAVID:
            config.uavId = value;
            break;
        case AMDCVAL_CBID:
            config.constBufferId = value;
            break;
        case AMDCVAL_PRINTFID:
            config.printfId = value;
            break;
        case AMDCVAL_EARLYEXIT:
            config.earlyExit = value;
            break;
        case AMDCVAL_CONDOUT:
            config.condOut = value;
            break;
        default:
            break;
    }
}

void AsmAmdPseudoOps::setConfigBoolValue(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, AmdConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AmdKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    if (target == AMDCVAL_USECONSTDATA)
        config.useConstantData = true;
    else if (target == AMDCVAL_USEPRINTF)
        config.usePrintf = true;
}

void AsmAmdPseudoOps::setCWS(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration value");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    uint64_t value1 = 1;
    uint64_t value2 = 0;
    uint64_t value3 = 0;
    const char* valuePlace = linePtr;
    bool good = getAbsoluteValueArg(asmr, value1, linePtr, true);
    asmr.printWarningForRange(32, value1, asmr.getSourcePos(valuePlace));
    bool haveComma;
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    if (haveComma)
    {
        skipSpacesToEnd(linePtr, end);
        valuePlace = linePtr;
        good &= getAbsoluteValueArg(asmr, value2, linePtr, false);
        asmr.printWarningForRange(32, value2, asmr.getSourcePos(valuePlace));
        
        if (!skipComma(asmr, haveComma, linePtr))
            return;
        if (haveComma)
        {
            valuePlace = linePtr;
            good &= getAbsoluteValueArg(asmr, value3, linePtr, false);
            asmr.printWarningForRange(32, value3, asmr.getSourcePos(valuePlace));
        }   
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AmdKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    config.reqdWorkGroupSize[0] = value1;
    config.reqdWorkGroupSize[1] = value2;
    config.reqdWorkGroupSize[2] = value3;
}

}

bool AsmAmdHandler::parsePseudoOp(const std::string& firstName,
       const char* stmtPlace, const char* linePtr)
{
    const size_t pseudoOp = binaryFind(amdPseudoOpNamesTbl, amdPseudoOpNamesTbl +
                    sizeof(amdPseudoOpNamesTbl)/sizeof(char*), firstName.c_str()+1,
                   CStringLess()) - amdPseudoOpNamesTbl;
    
    switch(pseudoOp)
    {
        case AMDOP_ARG:
            break;
        case AMDOP_BOOLCONSTS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_BOOL32CONSTS);
            break;
        case AMDOP_CALNOTE:
            break;
        case AMDOP_CBID:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_CBID);
            break;
        case AMDOP_CBMASK:
            AsmAmdPseudoOps::doEntry(*this, stmtPlace, linePtr,
                         1U<<CALNOTE_ATI_CONSTANT_BUFFERS);
            break;
        case AMDOP_COMPILE_OPTIONS:
            AsmAmdPseudoOps::setCompileOptions(*this, linePtr);
            break;
        case AMDOP_CONDOUT:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_CONDOUT);
            break;
        case AMDOP_CONFIG:
            AsmAmdPseudoOps::doConfig(*this, stmtPlace, linePtr);
            break;
        case AMDOP_CONSTANTBUFFERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_CONSTANT_BUFFERS);
            break;
        case AMDOP_CWS:
            AsmAmdPseudoOps::setCWS(*this, stmtPlace, linePtr);
            break;
        case AMDOP_DRIVER_INFO:
            AsmAmdPseudoOps::setDriverInfo(*this, linePtr);
            break;
        case AMDOP_DRIVER_VERSION:
            AsmAmdPseudoOps::setDriverVersion(*this, linePtr);
            break;
        case AMDOP_EARLYEXIT:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_EARLYEXIT);
            break;
        case AMDOP_ENTRY:
            AsmAmdPseudoOps::doEntry(*this, stmtPlace, linePtr,
                         1U<<CALNOTE_ATI_PROGINFO);
            break;
        case AMDOP_FLOATCONSTS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_FLOAT32CONSTS);
            break;
        case AMDOP_FLOATMODE:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_FLOATMODE);
            break;
        case AMDOP_GLOBALBUFFERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_GLOBAL_BUFFERS);
            break;
        case AMDOP_GLOBALDATA:
            AsmAmdPseudoOps::doGlobalData(*this, stmtPlace, linePtr);
            break;
        case AMDOP_HEADER:
            AsmAmdPseudoOps::addHeader(*this, stmtPlace, linePtr);
            break;
        case AMDOP_HWLOCAL:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_HWLOCAL);
            break;
        case AMDOP_HWREGION:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_HWREGION);
            break;
        case AMDOP_IEEEMODE:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_IEEEMODE);
            break;
        case AMDOP_INPUTS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_INPUTS);
            break;
        case AMDOP_INPUTSAMPLERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_INPUT_SAMPLERS);
            break;
        case AMDOP_INTCONSTS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_INT32CONSTS);
            break;
        case AMDOP_METADATA:
            AsmAmdPseudoOps::addMetadata(*this, stmtPlace, linePtr);
            break;
        case AMDOP_OUTPUTS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_OUTPUTS);
            break;
        case AMDOP_PERSISTENTBUFFERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_PERSISTENT_BUFFERS);
            break;
        case AMDOP_PGMRSRC2:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_PGMRSRC2);
            break;
        case AMDOP_PRINTFID:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_PRINTFID);
            break;
        case AMDOP_PRIVATEID:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_PRIVATEID);
            break;
        case AMDOP_PROGINFO:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_PROGINFO);
            break;
        case AMDOP_SAMPLER:
            break;
        case AMDOP_SCRATCHBUFFER:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                AMDCVAL_SCRATCHBUFFER);
            break;
        case AMDOP_SCRATCHBUFFERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_SCRATCH_BUFFERS);
            break;
        case AMDOP_SEGMENT:
            AsmAmdPseudoOps::doEntry(*this, stmtPlace, linePtr,
                    (1U<<CALNOTE_ATI_INT32CONSTS) | (1U<<CALNOTE_ATI_FLOAT32CONSTS) |
                    (1U<<CALNOTE_ATI_BOOL32CONSTS));
            break;
        case AMDOP_SGPRSNUM:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_SGPRSNUM);
            break;
        case AMDOP_SUBCONSTANTBUFFERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_SUB_CONSTANT_BUFFERS);
            break;
        case AMDOP_UAV:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_UAV);
            break;
        case AMDOP_UAVID:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_UAVID);
            break;
        case AMDOP_UAVMAILBOXSIZE:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_UAV_MAILBOX_SIZE);
            break;
        case AMDOP_UAVOPMASK:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_UAV_OP_MASK);
            break;
        case AMDOP_UAVPRIVATE:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_UAVPRIVATE);
            break;
        case AMDOP_USECONSTDATA:
            AsmAmdPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                            AMDCVAL_USECONSTDATA);
            break;
        case AMDOP_USEPRINTF:
            AsmAmdPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                            AMDCVAL_USEPRINTF);
            break;
        case AMDOP_USERDATA:
            break;
        case AMDOP_VGPRSNUM:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_VGPRSNUM);
            break;
        default:
            return false;
    }
    return true;
}

bool AsmAmdHandler::prepareBinary()
{
    output.is64Bit = assembler.is64Bit();
    output.deviceType = assembler.getDeviceType();
    /* initialize sections */
    size_t sectionsNum = sections.size();
    for (size_t i = 0; i < sectionsNum; i++)
    {
        const AsmSection& asmSection = assembler.sections[i];
        const Section& section = sections[i];
        const size_t sectionSize = asmSection.content.size();
        const cxbyte* sectionData = (!asmSection.content.empty()) ?
                asmSection.content.data() : (const cxbyte*)"";
        AmdKernelInput* kernel = (section.kernelId!=ASMKERN_GLOBAL) ?
                    &output.kernels[section.kernelId] : nullptr;
                
        switch(asmSection.type)
        {
            case AsmSectionType::CODE:
                kernel->codeSize = sectionSize;
                kernel->code = sectionData;
                break;
            case AsmSectionType::AMD_HEADER:
                kernel->headerSize = sectionSize;
                kernel->header = sectionData;
                break;
            case AsmSectionType::AMD_METADATA:
                kernel->metadataSize = sectionSize;
                kernel->metadata = (const char*)sectionData;
                break;
            case AsmSectionType::DATA:
                if (section.kernelId == ASMKERN_GLOBAL)
                {   // this is global data
                    output.globalDataSize = sectionSize;
                    output.globalData = sectionData;
                }
                else
                {   // this is kernel data
                    kernel->dataSize = sectionSize;
                    kernel->data = sectionData;
                }
                break;
            case AsmSectionType::AMD_CALNOTE:
            {
                CALNoteInput calNote;
                calNote.header.type = section.extraId;
                calNote.header.descSize = sectionSize;
                calNote.header.nameSize = 8;
                ::memcpy(calNote.header.name, "ATI CAL", 8);
                calNote.data = sectionData;
                kernel->calNotes.push_back(calNote);
                break;
            }
            case AsmSectionType::EXTRA_SECTION:
                if (section.kernelId == ASMKERN_GLOBAL)
                    output.extraSections.push_back({section.name,
                            asmSection.content.size(), asmSection.content.data(),
                            1, SHT_PROGBITS, 0, ELFSECTID_NULL, 0, 0 });
                else
                    kernel->extraSections.push_back({section.name,
                            asmSection.content.size(), asmSection.content.data(),
                            1, SHT_PROGBITS, 0, ELFSECTID_NULL, 0, 0 });
                break;
            default: // ignore other sections
                break;
        }
    }
    /* put extra symbols */
    if (assembler.flags & ASM_FORCE_ADD_SYMBOLS)
        for (const AsmSymbolEntry& symEntry: assembler.symbolMap)
        {
            if (!symEntry.second.hasValue ||
                ELF32_ST_BIND(symEntry.second.info) == STB_LOCAL)
                continue; // unresolved or local
            cxuint binSectId = (symEntry.second.sectionId != ASMSECT_ABS) ?
                    sections[symEntry.second.sectionId].elfBinSectId : ELFSECTID_ABS;
            if (binSectId==ELFSECTID_UNDEF)
                continue; // no section
            
            const BinSymbol binSym = { symEntry.first, symEntry.second.value,
                        symEntry.second.size, binSectId, false, symEntry.second.info,
                        symEntry.second.other };
            
            if (symEntry.second.sectionId == ASMSECT_ABS ||
                sections[symEntry.second.sectionId].kernelId == ASMKERN_GLOBAL)
                output.extraSymbols.push_back(std::move(binSym));
            else // to kernel extra symbols
                output.kernels[sections[symEntry.second.sectionId].kernelId].extraSymbols
                            .push_back(std::move(binSym));
        }
    return true;
}

void AsmAmdHandler::writeBinary(std::ostream& os) const
{
    AmdGPUBinGenerator binGenerator(&output);
    binGenerator.generate(os);
}

void AsmAmdHandler::writeBinary(Array<cxbyte>& array) const
{
    AmdGPUBinGenerator binGenerator(&output);
    binGenerator.generate(array);
}
