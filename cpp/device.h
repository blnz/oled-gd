#ifndef DEVICE_H
#define DEVICE_H

#include <typeinfo>
#include <string>
#include <node.h>

class Device : public node::ObjectWrap {
public:
    static void Init(v8::Handle<v8::Object> exports);

private:
    Device();
    ~Device();

    static v8::Handle<v8::Value> New(const v8::Arguments& args);
    static v8::Handle<v8::Value> Demo(const v8::Arguments& args);
    static v8::Handle<v8::Value> Display(const v8::Arguments& args);
    static v8::Handle<v8::Value> BlitString(const v8::Arguments& args);
    static v8::Handle<v8::Value> ShowBitmap(const v8::Arguments& args);
    static v8::Handle<v8::Value> InvertDisplay(const v8::Arguments& args);
    static v8::Handle<v8::Value> ClearDisplay(const v8::Arguments& args);

    int _counter;
    int _fd;
};

#endif
