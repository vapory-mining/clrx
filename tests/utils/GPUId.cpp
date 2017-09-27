/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
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
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <CLRX/utils/GPUId.h>
#include "../TestUtils.h"

using namespace CLRX;

// table with accepted GPU device names (with mixed lower/upper case)
static const std::pair<const char*, GPUDeviceType>
gpuDeviceEntryTable[] =
{
    { "baffin", GPUDeviceType::BAFFIN },
    { "bonaire", GPUDeviceType::BONAIRE },
    { "capeverde", GPUDeviceType::CAPE_VERDE },
    { "carrizo", GPUDeviceType::CARRIZO },
    { "dummy", GPUDeviceType::DUMMY },
    { "ellesmere", GPUDeviceType::ELLESMERE },
    { "fiji", GPUDeviceType::FIJI },
    { "gfx804", GPUDeviceType::GFX804 },
    { "gfx900", GPUDeviceType::GFX900 },
    { "gfx901", GPUDeviceType::GFX901 },
    { "goose", GPUDeviceType::GOOSE },
    { "hainan", GPUDeviceType::HAINAN },
    { "hawaii", GPUDeviceType::HAWAII },
    { "horse", GPUDeviceType::HORSE },
    { "iceland", GPUDeviceType::ICELAND },
    { "kalindi", GPUDeviceType::KALINDI },
    { "mullins", GPUDeviceType::MULLINS },
    { "oland", GPUDeviceType::OLAND },
    { "pitcairn", GPUDeviceType::PITCAIRN },
    { "polaris10", GPUDeviceType::ELLESMERE },
    { "polaris11", GPUDeviceType::BAFFIN },
    { "polaris12", GPUDeviceType::GFX804 },
    { "polaris20", GPUDeviceType::ELLESMERE },
    { "polaris21", GPUDeviceType::BAFFIN },
    { "polaris22", GPUDeviceType::GFX804 },
    { "raven", GPUDeviceType::GFX901 },
    { "spectre", GPUDeviceType::SPECTRE },
    { "spooky", GPUDeviceType::SPOOKY },
    { "stoney", GPUDeviceType::STONEY },
    { "tahiti", GPUDeviceType::TAHITI },
    { "tonga", GPUDeviceType::TONGA },
    { "topaz", GPUDeviceType::ICELAND },
    { "vega10", GPUDeviceType::GFX900 },
    { "vega11", GPUDeviceType::GFX901 },
    
    { "bAffin", GPUDeviceType::BAFFIN },
    { "boNAIre", GPUDeviceType::BONAIRE },
    { "cAPEverde", GPUDeviceType::CAPE_VERDE },
    { "carRIZo", GPUDeviceType::CARRIZO },
    { "duMMy", GPUDeviceType::DUMMY },
    { "elLESmere", GPUDeviceType::ELLESMERE },
    { "fiJI", GPUDeviceType::FIJI },
    { "gFX804", GPUDeviceType::GFX804 },
    { "gfX900", GPUDeviceType::GFX900 },
    { "GFx901", GPUDeviceType::GFX901 },
    { "goOSe", GPUDeviceType::GOOSE },
    { "hAINAn", GPUDeviceType::HAINAN },
    { "hAWaII", GPUDeviceType::HAWAII },
    { "hoRSe", GPUDeviceType::HORSE },
    { "iCELaNd", GPUDeviceType::ICELAND },
    { "KaliNDi", GPUDeviceType::KALINDI },
    { "mULlINs", GPUDeviceType::MULLINS },
    { "oLAnd", GPUDeviceType::OLAND },
    { "piTCairn", GPUDeviceType::PITCAIRN },
    { "polARis10", GPUDeviceType::ELLESMERE },
    { "pOLaris11", GPUDeviceType::BAFFIN },
    { "poLARis12", GPUDeviceType::GFX804 },
    { "polaRIS20", GPUDeviceType::ELLESMERE },
    { "pOLAris21", GPUDeviceType::BAFFIN },
    { "poLARis22", GPUDeviceType::GFX804 },
    { "raVEn", GPUDeviceType::GFX901 },
    { "spECTre", GPUDeviceType::SPECTRE },
    { "sPOoKy", GPUDeviceType::SPOOKY },
    { "stONeY", GPUDeviceType::STONEY },
    { "TAhITI", GPUDeviceType::TAHITI },
    { "toNGa", GPUDeviceType::TONGA },
    { "tOPAz", GPUDeviceType::ICELAND },
    { "VEga10", GPUDeviceType::GFX900 },
    { "veGA11", GPUDeviceType::GFX901 }
};

static const size_t gpuDeviceEntryTableSize =
    sizeof(gpuDeviceEntryTable) / sizeof(std::pair<const char*, GPUDeviceType>);

static void testGetGPUDeviceFromName()
{
    char descBuf[60];
    for (cxuint i = 0; i < gpuDeviceEntryTableSize; i++)
    {
        snprintf(descBuf, sizeof descBuf, "Test %d %s", i,
                        gpuDeviceEntryTable[i].first);
        const GPUDeviceType result = getGPUDeviceTypeFromName(
                        gpuDeviceEntryTable[i].first);
        assertValue("testGetGPUDeviceFromName", descBuf,
                    cxuint(gpuDeviceEntryTable[i].second), cxuint(result));
    }
    // checking exceptions
    assertCLRXException("testGetGPUDeviceFromName", "TestExc fxdxcd",
            "Unknown GPU device type", getGPUDeviceTypeFromName, "fdxdS");
    assertCLRXException("testGetGPUDeviceFromName", "TestExc fxdxcd2",
            "Unknown GPU device type", getGPUDeviceTypeFromName, "Polrdas");
    assertCLRXException("testGetGPUDeviceFromName", "TestExc empty",
            "Unknown GPU device type", getGPUDeviceTypeFromName, "");
}

// table with accepted GPU device names (with mixed lower/upper case)
static const std::pair<const char*, GPUArchitecture>
gpuArchitectureEntryTable[] =
{
    { "gcn1.0", GPUArchitecture::GCN1_0 },
    { "gcn1.1", GPUArchitecture::GCN1_1 },
    { "gcn1.2", GPUArchitecture::GCN1_2 },
    { "gcn1.4", GPUArchitecture::GCN1_4 },
    { "gfx6", GPUArchitecture::GCN1_0 },
    { "gfx7", GPUArchitecture::GCN1_1 },
    { "gfx8", GPUArchitecture::GCN1_2 },
    { "gfx9", GPUArchitecture::GCN1_4 },
    { "si", GPUArchitecture::GCN1_0 },
    { "ci", GPUArchitecture::GCN1_1 },
    { "vi", GPUArchitecture::GCN1_2 },
    { "vega", GPUArchitecture::GCN1_4 },
    { "gCn1.0", GPUArchitecture::GCN1_0 },
    { "Gcn1.1", GPUArchitecture::GCN1_1 },
    { "gCn1.2", GPUArchitecture::GCN1_2 },
    { "GCn1.4", GPUArchitecture::GCN1_4 },
    { "Gfx6", GPUArchitecture::GCN1_0 },
    { "gFX7", GPUArchitecture::GCN1_1 },
    { "GfX8", GPUArchitecture::GCN1_2 },
    { "gFX9", GPUArchitecture::GCN1_4 },
    { "Si", GPUArchitecture::GCN1_0 },
    { "CI", GPUArchitecture::GCN1_1 },
    { "vI", GPUArchitecture::GCN1_2 },
    { "vEGa", GPUArchitecture::GCN1_4 }
};

static const size_t gpuArchitectureEntryTableSize =
    sizeof(gpuArchitectureEntryTable) / sizeof(std::pair<const char*, GPUArchitecture>);

static void testGetGPUArchitectureFromName()
{
    char descBuf[60];
    for (cxuint i = 0; i < gpuArchitectureEntryTableSize; i++)
    {
        snprintf(descBuf, sizeof descBuf, "Test %d %s", i,
                        gpuArchitectureEntryTable[i].first);
        const GPUArchitecture result = getGPUArchitectureFromName(
                        gpuArchitectureEntryTable[i].first);
        assertValue("testGetGPUArchitectureFromName", descBuf,
                    cxuint(gpuArchitectureEntryTable[i].second), cxuint(result));
    }
    // checking exceptions
    assertCLRXException("testGetGPUArchitectureFromName", "TestExc fxdxcd",
            "Unknown GPU architecture", getGPUArchitectureFromName, "fdxdS");
    assertCLRXException("testGetGPUArchitectureFromName", "TestExc fxdxcd2",
            "Unknown GPU architecture", getGPUArchitectureFromName, "Polrdas");
    assertCLRXException("testGetGPUArchitectureFromName", "TestExc empty",
            "Unknown GPU architecture", getGPUArchitectureFromName, "");
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    retVal |= callTest(testGetGPUDeviceFromName);
    retVal |= callTest(testGetGPUArchitectureFromName);
    return retVal;
}

