#pragma once
#include <stdint.h>
#include "controller_link.h"

class TouchUi {
 public:
  void begin(ControllerLink& link);
  void update();

 private:
  enum Screen : uint8_t { HOME, PROFILE, SENSORS, ZONES, INFO, START_CONFIRM, CALIBRATION };
  struct ProfileSegment { int16_t targetDeciC; int16_t rampDeciCPerHour; uint32_t holdSeconds; };
  struct ZoneSettings { int16_t hysteresisDeciC; int16_t offsetDeciC; };

  ControllerLink* link_ = nullptr;
  Screen screen_ = HOME;
  bool redraw_ = true, touchDown_ = false, calibrationValid_ = false;
  bool profileUploaded_ = false, wasConnected_ = false;
  uint8_t calibrationStep_ = 0, selectedSegment_ = 0, profileCount_ = 1;
  uint8_t selectedSensor_ = 0, selectedZone_ = 0;
  uint32_t lastDrawMs_ = 0;
  ProfileSegment profile_[4] = {{0,500,0},{0,500,0},{0,500,0},{0,500,0}};
  ZoneSettings zones_[4] = {{20,0},{20,0},{20,0},{20,0}};
  uint8_t thermocoupleTypes_[4] = {0,0,0,0};
  int16_t sensorOffsetsDeciC_[4] = {0,0,0,0};
  float touchMatrix_[6] = {};
  float calibrationRaw_[3][2] = {};
  const char* notice_ = nullptr;

  void render();
  void drawHeader(const char* title);
  void drawNavigation();
  void drawHome();
  void drawProfile();
  void drawSensors();
  void drawZones();
  void drawInfo();
  void drawStartConfirm();
  void drawCalibration();
  void drawButton(int x,int y,int w,int h,const char* label,uint16_t fill,bool enabled=true);
  void handleTap(int x,int y);
  void handleCalibrationTap(int rawX,int rawY);
  bool rawToScreen(int rawX,int rawY,int& x,int& y) const;
  bool solveCalibration();
  void startCalibration();
  bool readyToStart(const char*& reason) const;
  bool sendProfile();
  void setNotice(const char* message);
  void saveSettings();
  void loadSettings();
};
