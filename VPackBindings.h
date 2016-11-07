////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2016 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Jan Christoph Uhde
////////////////////////////////////////////////////////////////////////////////
#include <nan.h>
#include <velocypack/Buffer.h>
#include <velocypack/Slice.h>
#include <velocypack/Builder.h>
#include <velocypack/velocypack-aliases.h>

//replaces arangodb string ref
#include <experimental/string_view>
namespace arangodb {
  using StringRef = std::experimental::string_view;
}

namespace arangodb { namespace node {

struct BuilderContext;
using VPBuffer = ::arangodb::velocypack::Buffer<uint8_t>;

v8::Local<v8::Value> TRI_VPackToV8(v8::Isolate* isolate,
                                    VPackSlice const& slice,
                                    VPackOptions const* options,
                                    VPackSlice const* base = nullptr);

template <bool performAllChecks, bool inObject>
static int V8ToVPack(BuilderContext& context,
                     v8::Local<v8::Value> const parameter,
                     arangodb::StringRef const& attributeName);





class VPackBuffer : public Nan::ObjectWrap {
 public:

  static NAN_MODULE_INIT(Init) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("VPackBuffer").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    //Nan::SetPrototypeMethod(tpl, "create", Collection::create);
    constructor().Reset(tpl->GetFunction());
    target->Set( Nan::New("VPackBuffer").ToLocalChecked() , tpl->GetFunction());
  }

  static NAN_METHOD(New);
  static NAN_METHOD(create);
  VPBuffer& cppClass() {
    return _cppBuffer;
  }
 private:
   VPBuffer _cppBuffer;

   VPackBuffer()
     : _cppBuffer()
     {}

  static Nan::Persistent<v8::Function>& constructor(){
    static Nan::Persistent<v8::Function> ctor;
    return ctor;
  }

};

}}
