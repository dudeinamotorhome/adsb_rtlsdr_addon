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
#include <errno.h>
#include <string.h>
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
  bool _radioInitialized;
  
  // Radio data
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
  std::cout << "initAppData()" << std::endl;
  if(AppData._dev) {
    std::cout << "rtlsdr_close()" << std::endl;
    rtlsdr_close(AppData._dev);
  }
  AppData._dev = NULL;
  AppData._devIndex = 0;
  AppData._autoGainEnabled = false;
  AppData._gain = 0;
  AppData._freq = 0;
  
  AppData._radioInitialized = false;
}

static void atExit(void *arg) {
  initAppData();
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
  if(r != 0) {
    std::cerr << "Error opening the RTLSDR device: " 
              << strerror(errno) << std::endl;
    args.GetReturnValue().Set(false);
    return;
  }
  
  // Set Gain Level
  if(AppData._autoGainEnabled) {
    r = rtlsdr_set_tuner_gain_mode(AppData._dev, 0);
    if(r != 0) {
      std::cerr << "Error setting automatic gain mode: "
                << strerror(errno) << std::endl;
      args.GetReturnValue().Set(false);
      initAppData();
      return;
    }
    AppData._gain = -100;
  }
  else { // Set Max Gain
    int numGains;
    int gains[100];
      
    // Get available gain values
    numGains = rtlsdr_get_tuner_gains(AppData._dev, gains);
    if(numGains <= 0) {
      std::cerr << "Error getting list of supported gains:"
                << strerror(errno) << std::endl;
        
      args.GetReturnValue().Set(false);
      initAppData();
      return;     
    }
      
    // Choose the Maximum gain level
    AppData._gain = gains[numGains - 1];
      
    // Set manual gain mode
    r = rtlsdr_set_tuner_gain_mode(AppData._dev, 1);
    if(r != 0) {
      std::cerr << "Error setting manual gain mode: "
                << strerror(errno) << std::endl;
      args.GetReturnValue().Set(false);
      initAppData();
      return;
    }
      
    // Set manual gain value
    r = rtlsdr_set_tuner_gain(AppData._dev, AppData._gain);
    if(r != 0) {
      std::cerr << "Error setting tuner gain value: "
                << strerror(errno) << std::endl;
      args.GetReturnValue().Set(false);
      initAppData();
      return;
    }
  }
    
  // set 0 frequency correction
  r = rtlsdr_set_freq_correction(AppData._dev, 0);
  /// @todo why does this fail? Is it because it is already 0?
  /*
  if(r != 0) {
    std::cerr << "Error setting frequency correction: "
              << strerror(errno) << std::endl;
    args.GetReturnValue().Set(false);
    initAppData();
    return;
  }
  */
  
  // set frequency to 1090 MHz for ADS-B
  AppData._freq = 1090000000; // 1090000000Hz = 1090MHz
  r = rtlsdr_set_center_freq(AppData._dev, AppData._freq); 
  if(r != 0) {
    std::cerr << "Error setting frequency value: "
              << strerror(errno) << std::endl;
    args.GetReturnValue().Set(false);
    initAppData();
    return;
  }  
  
  // set sample rate. We'll use 2 MHz as a nice even number, since RTLSDR_API
  // says we'll start getting sample loss if we go over 2.4 MHz
  r = rtlsdr_set_sample_rate(AppData._dev, 2000000); // 2000000Hz = 2MHz
  if(r != 0) {
    std::cerr << "Error setting sample rate: "
              << strerror(errno) << std::endl;
    args.GetReturnValue().Set(false);
    initAppData();
    return;
  }
  
  // Reset the streaming buffer
  r = rtlsdr_reset_buffer(AppData._dev);
  if(r != 0) {
    std::cerr << "Error resetting buffer: "
              << strerror(errno) << std::endl;
    args.GetReturnValue().Set(false);
    initAppData();
    return;
  }
  
  // Everything is configured, return TRUE
  AppData._radioInitialized = true;
  args.GetReturnValue().Set(true);
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
  gainSettings->Set(String::NewFromUtf8(isolate, "gainSettingInTenths_dB"),
                    Int32::New(isolate, AppData._gain));
  gainSettings->Set(String::NewFromUtf8(isolate, "deviceGainIn_dB"),
                    Number::New(
                      isolate,
                      rtlsdr_get_tuner_gain(AppData._dev)/10.0));
  
  args.GetReturnValue().Set(gainSettings);
}

/**
  void GetFreqSettings(const FunctionCallbackInfo<Value>& args)
  
  @brief Returns all the Frequency settings for the radio.
  
  @return JSON 
*/
void GetFreqSettings(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  
  Local<Object> freqSettings = Object::New(isolate);
  freqSettings->Set(String::NewFromUtf8(isolate, "freqSettingsInHz"),
                    Int32::New(isolate, AppData._freq));
  freqSettings->Set(String::NewFromUtf8(isolate, "deviceFreqInHz"),
                    Int32::New(isolate, rtlsdr_get_center_freq(AppData._dev)));
  freqSettings->Set(String::NewFromUtf8(isolate, "deviceFreqCorrectionInPPM"),
                    Int32::New(
                      isolate,
                      rtlsdr_get_freq_correction(AppData._dev)));
  
  args.GetReturnValue().Set(freqSettings);                  
}

void RegisterCallback(const FunctionCallbackInfo<Value>& args) {
  
}

void StartRadio(const FunctionCallbackInfo<Value>& args) {
  
}

void GetAircraftData(const FunctionCallbackInfo<Value>& args) {
  
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
  NODE_SET_METHOD(exports, "getFreqSettings", GetFreqSettings);
  NODE_SET_METHOD(exports, "registerCallback", RegisterCallback);
  NODE_SET_METHOD(exports, "startRadio", StartRadio);
  NODE_SET_METHOD(exports, "getAircraftData", GetAircraftData);
  NODE_SET_METHOD(exports, "closeRadio", CloseRadio);
  
  AtExit(atExit);
}

// define node-gyp entry point
extern "C" {
  NODE_MODULE(adsb_rtlsdr_addon, init)
}
