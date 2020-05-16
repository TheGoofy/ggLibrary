#pragma once

#include "ggNumerics.h"

namespace ggColor {


  struct cRGB {

    union {
      struct {
        uint8_t mR;
        uint8_t mG;
        uint8_t mB;
        uint8_t mW;
      };
      uint8_t mChannels[4];
      uint32_t mData;
    };

    inline cRGB()
    : mData(0) {
    }

    inline cRGB(uint32_t aData)
    : mData(aData) {
    }

    inline cRGB(uint8_t aR, uint8_t aG, uint8_t aB)
    : mR(aR), mG(aG), mB(aB), mW(0) {
    }

    inline cRGB(const cRGB& aOther)
    : mData(aOther.mData) {
    }

    inline operator uint32_t& () {
      return mData;
    }

    inline operator const uint32_t& () const {
      return mData;
    }

    inline bool operator == (const cRGB& aOther) const {
      return mData == aOther.mData;
    }

    inline bool operator != (const cRGB& aOther) const {
      return mData != aOther.mData;
    }

    // html colors (see https://www.w3schools.com/colors/colors_names.asp)
    inline static cRGB Black() { return cRGB(0,0,0); }
    inline static cRGB Red() { return cRGB(255,0,0); }
    inline static cRGB Green() { return cRGB(0,255,0); }
    inline static cRGB Blue() { return cRGB(0,0,255); }
    inline static cRGB Yellow() { return cRGB(255,255,0); }
    inline static cRGB Cyan() { return cRGB(0,255,255); }
    inline static cRGB Magenta() { return cRGB(255,0,255); }
    inline static cRGB White() { return cRGB(255,255,255); }
    inline static cRGB DarkGray() { return cRGB(169,169,169); }
    inline static cRGB Gray() { return cRGB(128,128,128); }
    inline static cRGB LightGray() { return cRGB(211,211,211); }
    inline static cRGB Orange() { return cRGB(255,165,0); }
    inline static cRGB GreenYellow() { return cRGB(173,255,47); }
    inline static cRGB Turquoise() { return cRGB(64,224,207); }
    inline static cRGB DeepSkyBlue() { return cRGB(0,191,255); }
    inline static cRGB DeepPink() { return cRGB(255,20,147); }
    inline static cRGB DarkViolet() { return cRGB(148,0,211); }
    inline static cRGB Purple() { return cRGB(128,0,128); }

  };


  struct cHSV {

    union {
      struct {
        uint8_t mH;
        uint8_t mS;
        uint8_t mV;
        uint8_t mX;
      };
      uint8_t mChannels[4];
      uint32_t mData;
    };

    inline cHSV()
    : mData(0) {
    }

    inline cHSV(uint8_t aH, uint8_t aS, uint8_t aV)
    : mH(aH), mS(aS), mV(aV), mX(0) {
    }

    inline cHSV(const cHSV& aOther)
    : mData(aOther.mData) {
    }

    inline bool operator == (const cHSV& aOther) const {
      return mData == aOther.mData;
    }

    inline bool operator != (const cHSV& aOther) const {
      return mData != aOther.mData;
    }

  };


  inline cRGB ToRGB(const cHSV& aHSV) {
    cRGB vRGB(aHSV.mV, aHSV.mV, aHSV.mV);
    if (aHSV.mS != 0) {
      uint8_t vRegion = aHSV.mH / 43;
      uint8_t vReminder = 6 * (aHSV.mH - (43 * vRegion));
      uint8_t vP = (aHSV.mV * (255 - ((aHSV.mS                    )     ))) >> 8;
      uint8_t vQ = (aHSV.mV * (255 - ((aHSV.mS * (      vReminder)) >> 8))) >> 8;
      uint8_t vT = (aHSV.mV * (255 - ((aHSV.mS * (255 - vReminder)) >> 8))) >> 8;
      switch (vRegion) {
        case 0:               vRGB.mG = vT; vRGB.mB = vP; break;
        case 1: vRGB.mR = vQ;               vRGB.mB = vP; break;
        case 2: vRGB.mR = vP;               vRGB.mB = vT; break;
        case 3: vRGB.mR = vP; vRGB.mG = vQ;               break;
        case 4: vRGB.mR = vT; vRGB.mG = vP;               break;
        case 5:               vRGB.mG = vP; vRGB.mB = vQ; break;
      }
    }
    // Serial.printf("ggColor::ToRGB(...) - HSV=%d/%d/%d ==> RGB=%d/%d/%d\n",
    //               aHSV.mH, aHSV.mS, aHSV.mV, vRGB.mR, vRGB.mG, vRGB.mB);
    return vRGB;
  }


  cHSV ToHSV(const cRGB& aRGB) {
    cHSV vHSV;
    uint8_t vMinRGB = ggMin(aRGB.mR, aRGB.mG, aRGB.mB);
    uint8_t vMaxRGB = ggMax(aRGB.mR, aRGB.mG, aRGB.mB);
    vHSV.mV = vMaxRGB;
    if (vMaxRGB != 0) {
      uint16_t vDeltaRGB = vMaxRGB - vMinRGB;
      vHSV.mS = 255 * vDeltaRGB / vMaxRGB;
      if (vDeltaRGB != 0) {
        if (vMaxRGB == aRGB.mR) vHSV.mH =   0 + 43 * (aRGB.mG - aRGB.mB) / vDeltaRGB;
        if (vMaxRGB == aRGB.mG) vHSV.mH =  85 + 43 * (aRGB.mB - aRGB.mR) / vDeltaRGB;
        if (vMaxRGB == aRGB.mB) vHSV.mH = 171 + 43 * (aRGB.mR - aRGB.mG) / vDeltaRGB;
      }
    }
    // Serial.printf("ggColor::ToHSV(...) - RGB=%d/%d/%d ==> HSV=%d/%d/%d\n",
    //               aRGB.mR, aRGB.mG, aRGB.mB, vHSV.mH, vHSV.mS, vHSV.mV);
    return vHSV;
  }


  cRGB HTMLToRGB(const String& aColorHTML) {
    // skip first char "#", convert with base 16 (hex)
    ggColor::cRGB vRGB(strtoul(aColorHTML.c_str() + 1, nullptr, 16));
    // Serial.printf("ggWebSockets::HTMLToRGB(...) - aColorHTML=%s ==> vRGB=%d/%d/%d\n",
    //               aColorHTML.c_str(), vRGB.mR, vRGB.mG, vRGB.mB);
    return vRGB;
  }


  String RGBToHTML(const ggColor::cRGB& aRGB) {
    char vColorHTML[8]; // 8 chars: 1 prefix, 6 digits, 1 zero terminator (#123456\0)
    sprintf(vColorHTML, "#%06x", aRGB.mData); // 6 hex digits with leading zeroes
    // Serial.printf("ggWebSockets::RGBToHTML(...) - aRGB=%d/%d/%d ==> vColorHTML=%s\n",
    //               aRGB.mR, aRGB.mG, aRGB.mB, vColorHTML);
    return vColorHTML;
  }

};
