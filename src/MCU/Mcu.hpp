#ifndef FREESHOP_MCU_HPP
#define FREESHOP_MCU_HPP

#include <3ds.h>

namespace FreeShop
{

class MCU {

public:
  static MCU& getInstance();

  typedef struct
  {
    u32 ani;
    u8 r[32];
    u8 g[32];
    u8 b[32];
  } RGBLedPattern;

  Result mcuInit();
  Result mcuExit();

  Result ptmSysmInit();
  Result ptmSysmExit();

  bool ledBlinkOnce(u32 col);
  bool ledBlinkThrice(u32 col);
  bool ledStay(u32 col);
  bool ledReset();

  void dimLeds(u8 brightness);

private:
  Handle m_mcuHandle;
  Handle m_ptmSysmHandle;

  Result mcuReadRegister(u8 reg, void* data, u32 size);
  Result mcuWriteRegister(u8 reg, void* data, u32 size);

  Result ptmSysmSetInfoLEDPattern(RGBLedPattern pattern);

  RGBLedPattern m_ledPattern;
  Result ledApply();

};

} // namespace FreeShop

#endif // FREESHOP_MCU_HPP
