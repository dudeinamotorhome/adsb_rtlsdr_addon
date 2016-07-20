/**
  @file adsb_rtlsdr_addon.cpp
  @author Jeff Powers, powerjm@gmail.com
  @date 20160715
  
  @brief Node.js native addon that decodes aircraft ADS-B transmissions using a
  RTL-SDR USB receiver.
  
  - revision history (date, name, brief summary)
   - 20160715, Jeff Powers, Initial version.
   
  @attention LICENSE TBD
*/

// system headers
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <iostream>

// node-gyp headers
#include <v8.h>
#include <node.h>

// RTL-SDR USB Radio header
#include <rtl-sdr.h>

using namespace node;
using namespace v8;

// Global Application Data
typedef struct {
  rtlsdr_dev_t *_dev;
  int _devIndex;
  bool _autoGainEnabled;
  int _gain;
  int _freq;
} AppData_t;

AppData_t AppData;

/**
  void initAppData()
  
  @brief Initializes the AppData struct.  If the device is open this will
  close it.
*/
void initAppData() {
  // std::cout << "initAppData()" << std::endl;
  if(AppData._dev) {
    // std::cout << "rtlsdr_close()" << std::endl;
    rtlsdr_close(AppData._dev);
  }
  AppData._dev = NULL;
  AppData._devIndex = 0;
  AppData._autoGainEnabled = false;
  AppData._gain = 0;
  AppData._freq = 0;
}

/**
  void GetRadioList(const FunctionCallbackInfo<Value>& args)
  
  @brief Returns a list of all available RTL-SDR radios connected to the
  computer.
  
  @param[in] args None
  @return JSON
*/
void GetRadioList(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  
  int rtlDeviceCount = rtlsdr_get_device_count();
  Local<Array> radios = Array::New(isolate, rtlDeviceCount);
  
  for(int i = 0; i < rtlDeviceCount; i++) {
    char vendor[256];
    char product[256];
    char serial[256];
    rtlsdr_get_device_usb_strings(i, vendor, product, serial);
    Local<Object> radioData = Object::New(isolate);
    radioData->Set(String::NewFromUtf8(isolate, "id"),
                   Integer::New(isolate, i));
    radioData->Set(String::NewFromUtf8(isolate, "vendor"),
                   String::NewFromUtf8(isolate, vendor));
    radioData->Set(String::NewFromUtf8(isolate, "product"),
                   String::NewFromUtf8(isolate, product));
    radioData->Set(String::NewFromUtf8(isolate, "serial"),
                   String::NewFromUtf8(isolate, serial));
    radios->Set(i, radioData);
  }
  args.GetReturnValue().Set(radios);
}


/**
  void InitRadioByID(const FunctionCallbackInfo<Value>& args)
  
  @brief Initializes the USB radio.
  
  @param[in] args[0] Int32 The Radio Device id enumerated by the RTLSDR API.
  @param[in] args[1] Boolean True for autogain, or false for max gain
  @return Boolean True if successful, false if it fails.
*/
void InitRadioByID(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  int r = 0;
  initAppData();
  
  // Check for 2 arguments
  if(args.Length() != 2) {
    isolate->ThrowException(
      Exception::TypeError(
        String::NewFromUtf8(isolate, "Invalid argument count")));
    return; 
  }
  
  // Check args[0] is integer
  if(!args[0]->IsInt32()) {
    isolate->ThrowException(
      Exception::TypeError(
        String::NewFromUtf8(isolate, "Invalid arg[0] type (not an integer")));
    return; 
  }
  
  // Check args[1] is boolean
  if(!args[1]->IsBoolean()) {
    isolate->ThrowException(
      Exception::TypeError(
        String::NewFromUtf8(isolate, "Invalid arg[1] type (not a bool")));
    return;
  }
  
  AppData._devIndex = args[0]->Int32Value();
  AppData._autoGainEnabled = args[1]->BooleanValue();
  
  // std::cout << "AppData._devIndex = " << AppData._devIndex << std::endl;
  
  // Open the device
  r = rtlsdr_open(&AppData._dev, (uint32_t)AppData._devIndex); 
  if(r >= 0) {
    // Set Gain Level
    if(AppData._autoGainEnabled) {
      rtlsdr_set_tuner_gain_mode(AppData._dev, 0);
      AppData._gain = -100;
    }
    else { // Set Max Gain
      int numGains;
      int gains[100];
      numGains = rtlsdr_get_tuner_gains(AppData._dev, gains);
      AppData._gain = gains[numGains - 1];
      rtlsdr_set_tuner_gain(AppData._dev, AppData._gain);
    }
    
    // set 0 frequency correction
    rtlsdr_set_freq_correction(AppData._dev, 0);
    
    rtlsdr_set_center_freq(AppData._dev, 1090000000); // 1090 MHz
    rtlsdr_set_sample_rate(AppData._dev, 2000000); // 2 MHz
    rtlsdr_reset_buffer(AppData._dev);
    args.GetReturnValue().Set(true);
  }
  else {
    fprintf(stderr, "Error opening the RTLSDR device: %s\n", strerror(errno));
    args.GetReturnValue().Set(false);
  }
}

/**
  void GetGainSettings(const FunctionCallbackInfo<Value>& args)
  
  @brief Returns all the Gain settings for the radio.
  
  @return JSON 
*/
void GetGainSettings(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  
  Local<Object> gainSettings = Object::New(isolate);
  gainSettings->Set(String::NewFromUtf8(isolate, "autoGainEnabled"),
                    Boolean::New(isolate, AppData._autoGainEnabled));
  gainSettings->Set(String::NewFromUtf8(isolate, "gainSetting"),
                    Int32::New(isolate, AppData._gain));
  gainSettings->Set(String::NewFromUtf8(isolate, "deviceGain"),
                    Number::New(isolate, rtlsdr_get_tuner_gain(AppData._dev)/10.0));
  
  args.GetReturnValue().Set(gainSettings);
}

/**
  void CloseRadio(const FunctionCallbackInfo<Value>& args)
  
  @brief closes the Radio and re-initializes the AppData structure.
*/
void CloseRadio(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  
  initAppData();
}


/**
  void init(Local<Object> exports)
  
  @brief Entry point to the addon.  It defines all functions.
  
  @param[in] exports
*/
void init(Local<Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  
  initAppData();
  
  NODE_SET_METHOD(exports, "getRadioList", GetRadioList);
  NODE_SET_METHOD(exports, "initRadioByID", InitRadioByID);
  NODE_SET_METHOD(exports, "getGainSettings", GetGainSettings);
  NODE_SET_METHOD(exports, "closeRadio", CloseRadio);
}

// define node-gyp entry point
extern "C" {
  NODE_MODULE(adsb_rtlsdr_addon, init)
}
