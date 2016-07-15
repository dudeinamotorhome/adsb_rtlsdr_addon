
#include <pthread.h>
#include <string.h>

// node-gyp headers
#include <v8.h>
#include <node.h>

// RTL-SDR USB Radio header
#include <rtl-sdr.h>

using namespace node;
using namespace v8;


// Isolate* isolate = Isolate::GetCurrent();

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

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "getRadioList", GetRadioList);
  //NODE_SET_METHOD(exports, "initRadioByID", InitRadioByID);
}

extern "C" {
  NODE_MODULE(adsb_rtlsdr_addon, init)
}
