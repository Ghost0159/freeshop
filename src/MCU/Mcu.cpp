#include "Mcu.hpp"
#include <cstring>

//Thanks to Sono and his research on MCU

namespace FreeShop
{

MCU &MCU::getInstance()
{
  static MCU mcu;
  return mcu;
}

Result MCU::mcuInit()
{
  return srvGetServiceHandle(&m_mcuHandle, "mcu::HWC");
}

Result MCU::mcuExit()
{
  return svcCloseHandle(m_mcuHandle);
}

Result MCU::mcuWriteRegister(u8 reg, void* data, u32 size)
{
    u32* ipc = getThreadCommandBuffer();
    ipc[0] = 0x20082;
    ipc[1] = reg;
    ipc[2] = size;
    ipc[3] = size << 4 | 0xA;
    ipc[4] = (u32)data;
    Result ret = svcSendSyncRequest(m_mcuHandle);
    if(ret < 0) return ret;
    return ipc[1];
}

Result MCU::mcuReadRegister(u8 reg, void* data, u32 size)
{
    u32* ipc = getThreadCommandBuffer();
    ipc[0] = 0x10082;
    ipc[1] = reg;
    ipc[2] = size;
    ipc[3] = size << 4 | 0xC;
    ipc[4] = (u32)data;
    Result ret = svcSendSyncRequest(m_mcuHandle);
    if(ret < 0) return ret;
    return ipc[1];
}

Result MCU::ptmSysmInit()
{
  return srvGetServiceHandle(&m_ptmSysmHandle, "ptm:sysm");
}

Result MCU::ptmSysmExit()
{
  return svcCloseHandle(m_ptmSysmHandle);
}

Result MCU::ptmSysmSetInfoLEDPattern(RGBLedPattern pattern)
{
    u32* ipc = getThreadCommandBuffer();
    ipc[0] = 0x8010640;
    memcpy(&ipc[1], &pattern, 0x64);
    Result ret = svcSendSyncRequest(m_ptmSysmHandle);
    if(ret < 0) return ret;
    return ipc[1];
}

Result MCU::ledApply()
{
  Result res;

  if (R_SUCCEEDED(res = mcuInit())) {
    mcuWriteRegister(0x2D, &m_ledPattern, sizeof(m_ledPattern));
    mcuExit();
  } else if (R_SUCCEEDED(res = ptmSysmInit())) {
    ptmSysmSetInfoLEDPattern(m_ledPattern);
    ptmSysmExit();
  }

  return res;
}

bool MCU::ledBlinkOnce(u32 col)
{
  memset(&m_ledPattern.r[ 0], 0, 32);
  memset(&m_ledPattern.g[ 0], 0, 32);
  memset(&m_ledPattern.b[ 0], 0, 32);

  memset(&m_ledPattern.r[ 2], (col >>   0) & 0xFF, 16);
  memset(&m_ledPattern.g[ 2], (col >>   8) & 0xFF, 16);
  memset(&m_ledPattern.b[ 2], (col >>  16) & 0xFF, 16);

  m_ledPattern.ani = 0xFF2040;

  return R_SUCCEEDED(ledApply());
}

bool MCU::ledBlinkThrice(u32 col)
{
  memset(&m_ledPattern.r[ 0], 0, 32);
  memset(&m_ledPattern.g[ 0], 0, 32);
  memset(&m_ledPattern.b[ 0], 0, 32);

  memset(&m_ledPattern.r[ 2], (col >>   0) & 0xFF, 5);
  memset(&m_ledPattern.g[ 2], (col >>   8) & 0xFF, 5);
  memset(&m_ledPattern.b[ 2], (col >>  16) & 0xFF, 5);
  memset(&m_ledPattern.r[ 7], 0, 2);
  memset(&m_ledPattern.g[ 7], 0, 2);
  memset(&m_ledPattern.b[ 7], 0, 2);
  memset(&m_ledPattern.r[ 9], (col >>   0) & 0xFF, 5);
  memset(&m_ledPattern.g[ 9], (col >>   8) & 0xFF, 5);
  memset(&m_ledPattern.b[ 9], (col >>  16) & 0xFF, 5);
  memset(&m_ledPattern.r[14], 0, 2);
  memset(&m_ledPattern.g[14], 0, 2);
  memset(&m_ledPattern.b[14], 0, 2);
  memset(&m_ledPattern.r[16], (col >>   0) & 0xFF, 5);
  memset(&m_ledPattern.g[16], (col >>   8) & 0xFF, 5);
  memset(&m_ledPattern.b[16], (col >>  16) & 0xFF, 5);

  m_ledPattern.ani = 0xFF1020;

  return R_SUCCEEDED(ledApply());
}

bool MCU::ledStay(u32 col)
{
  memset(&m_ledPattern.r[0], (col >>  0) & 0xFF, 32);
  memset(&m_ledPattern.g[0], (col >>  8) & 0xFF, 32);
  memset(&m_ledPattern.b[0], (col >> 16) & 0xFF, 32);

  m_ledPattern.ani = 0xFF0201;

  return R_SUCCEEDED(ledApply());
}

bool MCU::ledReset()
{
  memset(&m_ledPattern.r[0], 0, 32);
  memset(&m_ledPattern.g[0], 0, 32);
  memset(&m_ledPattern.b[0], 0, 32);

  m_ledPattern.ani = 0xFF0000;

  return R_SUCCEEDED(ledApply());
}

void MCU::dimLeds(u8 brightness)
{
  mcuWriteRegister(0x28, &brightness, sizeof(brightness));
}

} // namespace FreeShop
