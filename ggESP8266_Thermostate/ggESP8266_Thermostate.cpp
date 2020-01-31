#include <Arduino.h>
#include <ESP8266WebServer.h> // https://github.com/esp8266/Arduino
#include <WebSocketsServer.h> // https://github.com/Links2004/arduinoWebSockets (by Markus Sattler)
#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager (by Tzapu)
#include <BME280I2C.h>        // https://github.com/finitespace/BME280 (by Tyler Glenn)
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>

// PCB version definition (ggPeriphery.h)
// #define M_PCB_VERSION_V1
// #define M_PCB_VERSION_V2
// #define M_PCB_VERSION_V3
// #define M_PCB_VERSION_V4 // actual fabricated PCB (May 2019, installed in Vella)
#define M_PCB_VERSION_V5 // doesn't use RX/TX-pins for SSR-control (serial port still usable for debugging)

#include "ggWebServer.h"
#include "ggWebSockets.h"
#include "ggWiFiConnection.h"
#include "ggPeriphery.h"
#include "ggController.h"
#include "ggValueEEPromString.h"
#include "ggStreams.h"
#include "ggCircularFileT.h"
#include "ggTimerNTP.h"
#include "ggAveragesT.h"


/*
todo:
- "wifimanager" should not block controller operation:
  - option A) wifiManager.setAPCallback(configModeCallback);
  - option B) own wifimanager ...
- in AP-mode also run http-server with controller settings
- scan LAN for connected devices (app for smart-phone)
- which web-interface belongs to which device? "ping" flashing status LED
- data-logging on ESP (multi-time-resolution)
- PID auto-tuning algorithm
- adjustable PWM cycle time
- serial stream to rx/tx AND html client console
- SW/HW version
- use "littleFS" instead of "SPIFFS" (sketch data upload: https://github.com/earlephilhower/arduino-esp8266littlefs-plugin)
- VS-Code: SPIFFS/LittleFS upload, OTA
- NTP server in eeprom
- pin-assignment in eeprom
- debugging: print number of connected web socket clients
- live-log discard old samples, 1h-view, no super-sample
*/


const String mHostName = "ESP-SSR-" + String(ESP.getChipId(), HEX);

FS* mFileSystem = &SPIFFS; // &LittleFS or &SPIFFS;

// runs AP, if no wifi connection
WiFiManager mWifiManager;
ggWiFiConnection mWiFiConnection;
  
// create web server on port 80
const int mWebServerPort = 80;
ggWebServer mWebServer(mWebServerPort, mFileSystem);

// create web sockets server on port 81
const int mWebSocketsPort = 81;
ggWebSockets mWebSockets(mWebSocketsPort);

// input and ouput configuration
ggPeriphery mPeriphery;

// controls an output based on input and reference value
ggController mTemperatureController;

// device name
ggValueEEPromString<> mName(mHostName);


// data logging
ggTimerNTP mTimerNTP("ch.pool.ntp.org", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00");

class ggDataLog {
public:
  ggDataLog(uint32_t aPeriod, // time ins seconds from sample to sample
            uint32_t aDuration, // overall recording time in seconds until circular file rolls over
            const String& aFileName, // filename for the circular file (ring-buffer)
            FS* aFileSystem) // file-system ro use
  : mPressureAVG(),
    mTemperatureAVG(),
    mHumidityAVG(),
    mOutputAVG(),
    mPeriod(aPeriod),
    mCircularFile(aFileName, aDuration / aPeriod, aFileSystem) {
    GG_DEBUG();
    vDebug.PrintF("aPeriod = %d\n", aPeriod);
    vDebug.PrintF("aDuration = %d\n", aDuration);
    vDebug.PrintF("number of data blocks = %d\n", mCircularFile.GetNumberOfDataBlocks());
    vDebug.PrintF("aFileName = \"%s\"\n", aFileName.c_str());
  }
  uint32_t GetPeriod() const {
    return mPeriod;
  }
  const String& GetFileName() const {
    return mCircularFile.GetFileName();
  }
  void AddPressureSample(float aPressure) {
    mPressureAVG.AddSample(aPressure);
  }
  void AddTemperatureSample(float aTemperature) {
    mTemperatureAVG.AddSample(aTemperature);
  }
  void AddHumiditySample(float aHumidity) {
    mHumidityAVG.AddSample(aHumidity);
  }
  void AddOutputSample(float aOutput) {
    mOutputAVG.AddSample(aOutput);
  }
  void AddSamples(const ggDataLog& aDataLogSrc) {
    mPressureAVG.AddSamples(aDataLogSrc.mPressureAVG);
    mTemperatureAVG.AddSamples(aDataLogSrc.mTemperatureAVG);
    mHumidityAVG.AddSamples(aDataLogSrc.mHumidityAVG);
    mOutputAVG.AddSamples(aDataLogSrc.mOutputAVG);
  }
  void ResetOnNextAddSample() {
    mPressureAVG.ResetOnNextAddSample();
    mTemperatureAVG.ResetOnNextAddSample();
    mHumidityAVG.ResetOnNextAddSample();
    mOutputAVG.ResetOnNextAddSample();
  }
  void Write(time_t aTime) {
    // BME280 ranges: 300..1100hPa, -40..85°C, 0..100%
    // https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf
    cMeasurements vMeasurements;
    mPressureAVG.AssignValues(vMeasurements.mPressure, 100.0f, -800.0f); // common range: 600..1000hPa (https://de.wikipedia.org/wiki/Luftdruck)
    mTemperatureAVG.AssignValues(vMeasurements.mTemperature, 100.0f, 0.0f); // common range: -20..40°C
    mHumidityAVG.AssignValues(vMeasurements.mHumidity, 100.0f, 0.0f); // common range: 0..100%
    mOutputAVG.AssignValues(vMeasurements.mOutput, 10000.0f, 0.0f); // common range: 0..1
    mCircularFile.Write(aTime, vMeasurements);
  }
private:
  typedef struct cValue {
    int16_t mMean;
    int16_t mMin;
    int16_t mMax;
    int16_t mStdDev;
  };
  typedef struct cMeasurements {
    cValue mPressure;
    cValue mTemperature;
    cValue mHumidity;
    cValue mOutput;
  };
  class cAverages {
  public:
    typedef ggAveragesT<float, float> tBaseClass;
    cAverages()
    : mAverages(),
      mResetOnNextAddSample(false) {
    }
    void AddSample(float aValue) {
      ResetIfNeeded();
      mAverages.AddSample(aValue);
    }
    void AddSamples(const cAverages& aAveragesSrc) {
      ResetIfNeeded();
      mAverages.AddSamples(aAveragesSrc.mAverages);
    }
    void ResetOnNextAddSample() {
      mResetOnNextAddSample = true;
    }
    void AssignValues(cValue& aValue, float aScale = 1.0f, float aOffset = 0.0f) {
      aValue.mMean = ggRound<int16_t>(aScale * (aOffset + mAverages.GetMean()));
      aValue.mMin = ggRound<int16_t>(aScale * (aOffset + mAverages.GetMin()));
      aValue.mMax = ggRound<int16_t>(aScale * (aOffset + mAverages.GetMax()));
      aValue.mStdDev = ggRound<int16_t>(aScale * (aOffset + mAverages.GetStdDev()));
    }
  private:
    void ResetIfNeeded() {
      if (mResetOnNextAddSample) {
        mAverages.Reset();
        mResetOnNextAddSample = false;
      }
    }
    ggAveragesT<float, float> mAverages;
    bool mResetOnNextAddSample;
  };
  typedef ggCircularFileT<time_t, cMeasurements> tCircularFile;
  cAverages mPressureAVG;
  cAverages mTemperatureAVG;
  cAverages mHumidityAVG;
  cAverages mOutputAVG;
  uint32_t mPeriod;
  tCircularFile mCircularFile;
};


ggDataLog* mDataLog1D = nullptr;
ggDataLog* mDataLog1W = nullptr;
ggDataLog* mDataLog1M = nullptr;
ggDataLog* mDataLog1Y = nullptr;
ggDataLog* mDataLogMax = nullptr;


void CreateComponents()
{
  // create data loggers with various sampling rates
  /*
  mDataLog1D = new ggDataLog(30, 24*60*60, "/ggData1D.dat", mFileSystem);
  mDataLog1W = new ggDataLog(5*60, 7*24*60*60, "/ggData1W.dat", mFileSystem);
  mDataLog1M = new ggDataLog(15*60, 30*24*60*60, "/ggData1M.dat", mFileSystem);
  mDataLog1Y = new ggDataLog(3*60*60, 365*24*60*60, "/ggData1Y.dat", mFileSystem);
  mDataLog1Max = new ggDataLog(24*60*60, 10*365*24*60*60, "/ggDataMax.dat", mFileSystem);
  */
  mDataLog1D = new ggDataLog(5, 500, "/ggData1D.dat", mFileSystem);
  mDataLog1W = new ggDataLog(10, 1000, "/ggData1W.dat", mFileSystem);
  mDataLog1M = new ggDataLog(30, 3000, "/ggData1M.dat", mFileSystem);
  mDataLog1Y = new ggDataLog(60, 6000, "/ggData1Y.dat", mFileSystem);
  mDataLogMax = new ggDataLog(120, 12000, "/ggDataMax.dat", mFileSystem);
}


void ConnectComponents()
{
  // when a new client is conneted, it needs a complete update
  mWebSockets.OnClientConnect([&] (int aClientID) {
    ggDebug vDebug("mWebSockets.OnClientConnect(...)");
    vDebug.PrintF("aClientID = %d\n", aClientID);
    mWebSockets.UpdateName(mName.Get(), aClientID);
    mWebSockets.UpdateSensorStatus(mPeriphery.mSensor.GetStatus(), aClientID);
    mWebSockets.UpdatePressure(mPeriphery.mSensor.GetPressure(), aClientID);
    mWebSockets.UpdateTemperature(mPeriphery.mSensor.GetTemperature(), aClientID);
    mWebSockets.UpdateHumidity(mPeriphery.mSensor.GetHumidity(), aClientID);
    mWebSockets.UpdateControlMode(mTemperatureController.GetMode(), aClientID);
    mWebSockets.UpdateTemperatureSetPoint(mTemperatureController.GetSetPoint(), aClientID);
    mWebSockets.UpdateHysteresis(mTemperatureController.GetHysteresis(), aClientID);
    mWebSockets.UpdateControlPID(mTemperatureController.GetP(), mTemperatureController.GetI(), mTemperatureController.GetD(), aClientID);
    mWebSockets.UpdateOutputAnalog(mTemperatureController.GetOutputAnalog(), aClientID);
    mWebSockets.UpdateOutput(mTemperatureController.GetOutput(), aClientID);
    mWebSockets.UpdateKey(mPeriphery.mKey.GetPressed(), aClientID);
  });
  mWebSockets.OnClientDisconnect([&] (int aClientID) {
    ggDebug vDebug("mWebSockets.OnClientDisonnect(...)");
    vDebug.PrintF("aClientID = %d\n", aClientID);
  });

  // controller event: when output changes, the SSR needs to be switched
  mTemperatureController.OnOutputChanged([&] (float aOutputValue) {
    mPeriphery.mOutputPWM.Set(aOutputValue);
    mWebSockets.UpdateOutput(aOutputValue);
    mDataLog1D->AddOutputSample(aOutputValue);
  });

  // when button "key" is pressed we switch the SSR manually
  mPeriphery.mKey.OnChanged([&] (bool aPressed) {
    mWebSockets.UpdateKey(aPressed);
  });
  mPeriphery.mKey.OnPressed([&] () {
    mTemperatureController.SetMode(mTemperatureController.GetMode() != ggController::eModeOff ? ggController::eModeOff : ggController::eModeOn);
    mWebSockets.UpdateControlMode(mTemperatureController.GetMode());
  });
  mPeriphery.mKey.OnPressedFor(5000, [&] () {
    mPeriphery.mStatusLED.SetWarning(true);
  });
  mPeriphery.mKey.OnReleased([&] () {
    if (mPeriphery.mKey.GetMillisDelta() > 5000) {
      mTemperatureController.ResetSettings();
      mWifiManager.resetSettings();
      ESP.restart();
    }
  });

  // connect sensor events
  mPeriphery.mSensor.OnStatusChanged([&] (const char* aStatus) {
    ggDebug vDebug("mPeriphery.mSensor.OnStatusChanged");
    vDebug.PrintF("aStatus = %s\n", aStatus);
    mTemperatureController.SetInputValid(mPeriphery.mSensor.StatusOK());
    mPeriphery.mStatusLED.SetError(!mPeriphery.mSensor.StatusOK());
    mWebSockets.UpdateSensorStatus(aStatus);
  });
  mPeriphery.mSensor.OnPressureChanged([&] (float aPressure) {
    mWebSockets.UpdatePressure(aPressure);
    mDataLog1D->AddPressureSample(aPressure);
  });
  mPeriphery.mSensor.OnTemperatureChanged([&] (float aTemperature) {
    mTemperatureController.SetInput(aTemperature);
    mWebSockets.UpdateTemperature(aTemperature);
    mDataLog1D->AddTemperatureSample(aTemperature);
  });
  mPeriphery.mSensor.OnHumidityChanged([&] (float aHumidity) {
    mWebSockets.UpdateHumidity(aHumidity);
    mDataLog1D->AddHumiditySample(aHumidity);
  });

  // wifi events
  mWiFiConnection.OnConnect([&] () {
    ggDebug vDebug("mWiFiConnection.OnConnect");
    mPeriphery.mStatusLED.SetWarning(false);
  });
  mWiFiConnection.OnDisconnect([&] () {
    ggDebug vDebug("mWiFiConnection.OnDisconnect");
    mPeriphery.mStatusLED.SetWarning(true);
  });

  // OTA events
  ArduinoOTA.onStart([] () {
    mPeriphery.mStatusLED.SetOTA(true); // indicate "upload"
    mPeriphery.mOutputPWM.Set(false); // switch off output (in case OTA fails)
  });
  ArduinoOTA.onEnd([] () {
    mPeriphery.mStatusLED.SetOTA(false);
  });

  // events from client: control mode, reference temperature, ...
  mWebSockets.OnSetName([&] (const String& aName) {
    mName.Set(aName);
    mWebSockets.UpdateName(mName.Get());
  });
  mWebSockets.OnSetControlMode([&] (int aControlMode) {
    mTemperatureController.SetMode(static_cast<ggController::tMode>(aControlMode));
    mWebSockets.UpdateControlMode(mTemperatureController.GetMode());
  });
  mWebSockets.OnSetTemperatureSetPoint([&] (float aTemperatureSetPoint) {
    mTemperatureController.SetSetPoint(aTemperatureSetPoint);
    mWebSockets.UpdateTemperatureSetPoint(mTemperatureController.GetSetPoint());
  });
  mWebSockets.OnSetHysteresis([&] (float aHysteresis) {
    mTemperatureController.SetHysteresis(aHysteresis);
    mWebSockets.UpdateHysteresis(mTemperatureController.GetHysteresis());
  });
  mWebSockets.OnSetControlPID([&] (float aP, float aI, float aD) {
    mTemperatureController.SetPID(aP, aI, aD);
    mWebSockets.UpdateControlPID(aP, aI, aD);
  });
  mWebSockets.OnSetOutputAnalog([&] (bool aOutputAnalog) {
    mTemperatureController.SetOutputAnalog(aOutputAnalog);
    mWebSockets.UpdateOutputAnalog(mTemperatureController.GetOutputAnalog());
  });
  mWebSockets.OnSetOutput([&] (float aOutputValue) {
    mPeriphery.mOutputPWM.Set(aOutputValue);
    mWebSockets.UpdateOutput(aOutputValue);
    mDataLog1D->AddOutputSample(aOutputValue);
  });

  // web server
  mWebServer.OnDebugStream([] (Stream& aStream) {
    ggStreams vStreams;
    vStreams.push_back(&aStream);
    vStreams.push_back(&Serial);
    ggDebug::SetStream(vStreams);
    ggDebug vDebug("mWebServer.OnDebugStream(...)");
    mPeriphery.PrintDebug("mPeriphery");
    mTemperatureController.PrintDebug("mTemperatureController");
    ggDebug::SetStream(Serial);
  });
  mWebServer.OnReset([] () {
    mTemperatureController.ResetSettings();
    mWifiManager.resetSettings();
  });
  mWebServer.OnReboot([] () {
    ESP.restart();
  });

  // timer and logging
  mTimerNTP.AddTimer(mDataLog1D->GetPeriod(), [] (uint32_t aPeriod) {
    mDataLog1D->Write(mTimerNTP.GetTime());
    mDataLog1W->AddSamples(*mDataLog1D);
    mDataLog1D->ResetOnNextAddSample();
  });
  mTimerNTP.AddTimer(mDataLog1W->GetPeriod(), [] (uint32_t aPeriod) {
    mDataLog1W->Write(mTimerNTP.GetTime());
    mDataLog1M->AddSamples(*mDataLog1W);
    mDataLog1W->ResetOnNextAddSample();
  });
  mTimerNTP.AddTimer(mDataLog1M->GetPeriod(), [] (uint32_t aPeriod) {
    mDataLog1M->Write(mTimerNTP.GetTime());
    mDataLog1Y->AddSamples(*mDataLog1M);
    mDataLog1M->ResetOnNextAddSample();
  });
  mTimerNTP.AddTimer(mDataLog1Y->GetPeriod(), [] (uint32_t aPeriod) {
    mDataLog1Y->Write(mTimerNTP.GetTime());
    mDataLogMax->AddSamples(*mDataLog1Y);
    mDataLog1Y->ResetOnNextAddSample();
  });
  mTimerNTP.AddTimer(mDataLogMax->GetPeriod(), [] (uint32_t aPeriod) {
    mDataLogMax->Write(mTimerNTP.GetTime());
    mDataLogMax->ResetOnNextAddSample();
  });
}


void Run()
{
  mPeriphery.Run();
  mTemperatureController.Run();
  mWebServer.Run();
  mWebSockets.Run();
  mWiFiConnection.Run();
  mTimerNTP.Run();
  MDNS.update();
  ArduinoOTA.handle();
  yield();
}


void WifiManagerConfigPortalStart(WiFiManager* aWiFiManager)
{
  mPeriphery.mStatusLED.Begin();
  mPeriphery.mStatusLED.SetWarning(true);
}


void WifiManagerConfigPortalEnd()
{
  mPeriphery.mStatusLED.SetWarning(false);
}


void setup()
{
  // serial communication (for debugging)
  Serial.begin(115200);
  Serial.println("");
  
  GG_DEBUG();

  // startup eeprom utility class
  ggValueEEProm::Begin();
  Serial.printf("Device Name: %s\n", mName.Get().c_str());

  // start the file system
  mFileSystem->begin();

  // connect to wifi
  mWifiManager.setDebugOutput(true);
  mWifiManager.setAPCallback(WifiManagerConfigPortalStart);
  mWifiManager.setSaveConfigCallback(WifiManagerConfigPortalEnd);
  mWifiManager.setConfigPortalTimeout(60); // 1 minute
  mWifiManager.autoConnect(mHostName.c_str());
  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  mWiFiConnection.Begin();

  // create and connect inputs, outputs, socket-events, ...
  CreateComponents();
  ConnectComponents();

  // NTP timer
  mTimerNTP.Begin();

  // configure and start web-server
  mWebServer.Begin();
  Serial.println("Web server started");

  // configure and start web-sockets
  mWebSockets.Begin();
  Serial.println("Web sockets started");

  // start mdns
  MDNS.begin(mHostName.c_str());
  MDNS.addService("http", "tcp", mWebServerPort);
  MDNS.addService("ws", "tcp", mWebSocketsPort);
  Serial.println("MDNS responder started");

  // over the air update
  ArduinoOTA.setHostname(mHostName.c_str());
  ArduinoOTA.begin();
  Serial.println("OTA service started");

  // make sure all status and debug messages are sent before communication gets
  // interrupted, just in case hardware pins are needed for some different use.
  Serial.flush();

  // setup connected hardware
  mPeriphery.Begin();
  mTemperatureController.Begin();
}


void loop()
{
  Run();
}
