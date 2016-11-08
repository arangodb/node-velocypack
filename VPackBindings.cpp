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
/// @author Jan Steeman
/// @author Andreas Streichardt
/// @author Jan Christoph Uhde
////////////////////////////////////////////////////////////////////////////////

// Most code is taken from arangodb/lib/V8/v8-vpack.{cpp,h}

#include <nan.h>

#include <velocypack/Buffer.h>
#include <velocypack/Builder.h>
#include <velocypack/Options.h>
#include <velocypack/Slice.h>
#include <velocypack/Iterator.h>

#include "VPackBindings.h"

static int const MaxLevels = 64;

#define TRI_V8_ASCII_PAIR_STRING(name, length) \
  v8::String::New((name), (int)(length))

#define TRI_V8_PAIR_STRING(name, length) \
  v8::String::New((name), (int)(length))

#define TRI_V8_STD_STRING(name) \
  v8::String::New(name.c_str(), (int)name.length())


namespace arangodb { namespace node {

/// @brief converts a VelocyValueType::String into a V8 object
static inline v8::Local<v8::Value> ObjectVPackString(v8::Isolate* isolate,
                                                      VPackSlice const& slice) {
  ::arangodb::velocypack::ValueLength l;
  char const* val = slice.getString(l);
  if (l == 0) {
    return v8::String::Empty(isolate);
  }
  return TRI_V8_PAIR_STRING(val, l);
}

/// @brief converts a VelocyValueType::Object into a V8 object
static v8::Local<v8::Value> ObjectVPackObject(v8::Isolate* isolate,
                                               VPackSlice const& slice,
                                               VPackOptions const* options,
                                               VPackSlice const* base) {

  assert(slice.isObject());
  v8::Local<v8::Object> object = v8::Object::New();

  if (object.IsEmpty()) {
    return Nan::Undefined();
  }

  VPackObjectIterator it(slice, true);
  while (it.valid()) {
    ::arangodb::velocypack::ValueLength l;
    VPackSlice k = it.key(false);

    if (k.isString()) {
      // regular attribute
      char const* p = k.getString(l);
      object->ForceSet(TRI_V8_PAIR_STRING(p, l),
                       TRI_VPackToV8(isolate, it.value(), options, &slice));
    } else {
      // optimized code path for translated system attributes
      VPackSlice v = VPackSlice(k.begin() + 1);
      v8::Local<v8::Value> sub;
      if (v.isString()) {
        char const* p = v.getString(l);
        sub = TRI_V8_ASCII_PAIR_STRING(p, l);
      } else {
        sub = TRI_VPackToV8(isolate, v, options, &slice);
      }

      uint8_t which =
          static_cast<uint8_t>(k.getUInt()) + AttributeBase;
      switch (which) {
        case KeyAttribute: {
          object->ForceSet(Nan::New("_key").ToLocalChecked(), sub);
          break;
        }
        case RevAttribute: {
          object->ForceSet(Nan::New("_rev").ToLocalChecked(), sub);
          break;
        }
        case IdAttribute: {
          object->ForceSet(Nan::New("_id").ToLocalChecked(), sub);
          break;
        }
        case FromAttribute: {
          object->ForceSet(Nan::New("_from").ToLocalChecked(), sub);
          break;
        }
        case ToAttribute: {
          object->ForceSet(Nan::New("_to").ToLocalChecked(), sub);
          break;
        }
      }
    }

    //check out of memory
    it.next();
  }

  return object;
}

/// @brief converts a VelocyValueType::Array into a V8 object
static inline v8::Local<v8::Value> ObjectVPackArray(v8::Isolate* isolate,
                                              VPackSlice const& slice,
                                              VPackOptions const* options,
                                              VPackSlice const* base) {
  assert(slice.isArray());
  v8::Local<v8::Array> object =
      v8::Array::New(static_cast<int>(slice.length()));

  if (object.IsEmpty()) {
    return Nan::Undefined();
  }

  uint32_t j = 0;
  VPackArrayIterator it(slice);

  while (it.valid()) {
    v8::Local<v8::Value> val =
        TRI_VPackToV8(isolate, it.value(), options, &slice);
    if (!val.IsEmpty()) {
      object->Set(j++, val);
    }

    //checl memory
    it.next();
  }

  return object;
}

/// @brief converts a VPack value into a V8 object
v8::Local<v8::Value> TRI_VPackToV8(v8::Isolate* isolate,
                                    VPackSlice const& slice,
                                    VPackOptions const* options,
                                    VPackSlice const* base) {
  switch (slice.type()) {
    case VPackValueType::Null: {
      return Nan::Null();
    }
    case VPackValueType::Bool: {
      return Nan::New<v8::Boolean>(slice.getBool());
      //return v8::Boolean::New(slice.getBool());
    }
    case VPackValueType::Double: {
      // convert NaN, +inf & -inf to null
      double value = slice.getDouble();
      if (std::isnan(value) || !std::isfinite(value) || value == HUGE_VAL ||
          value == -HUGE_VAL) {
        return Nan::Null();
      }
      return v8::Number::New(slice.getDouble());
    }
    case VPackValueType::Int: {
      int64_t value = slice.getInt();
      if (value >= -2147483648LL && value <= 2147483647LL) {
        // value is within bounds of an int32_t
        return v8::Integer::New(static_cast<int32_t>(value));
      }
      if (value >= 0 && value <= 4294967295LL) {
        // value is within bounds of a uint32_t
        return v8::Integer::NewFromUnsigned(static_cast<uint32_t>(value));
      }
      // must use double to avoid truncation
      return v8::Number::New(static_cast<double>(slice.getInt()));
    }
    case VPackValueType::UInt: {
      uint64_t value = slice.getUInt();
      if (value <= 4294967295ULL) {
        // value is within bounds of a uint32_t
        return v8::Integer::NewFromUnsigned(static_cast<uint32_t>(value));
      }
      // must use double to avoid truncation
      return v8::Number::New(static_cast<double>(slice.getUInt()));
    }
    case VPackValueType::SmallInt: {
      return v8::Integer::New(slice.getNumericValue<int32_t>());
    }
    case VPackValueType::String: {
      return ObjectVPackString(isolate, slice);
    }
    case VPackValueType::Array: {
      return ObjectVPackArray(isolate, slice, options, base);
    }
    case VPackValueType::Object: {
      return ObjectVPackObject(isolate, slice, options, base);
    }
    case VPackValueType::External: {
      // resolve external
      return TRI_VPackToV8(isolate, VPackSlice(slice.getExternal()), options,
                           base);
    }
    case VPackValueType::Custom: {
      if (options == nullptr || options->customTypeHandler == nullptr ||
          base == nullptr) {
        //FIXME THROW ERROR
      }
      std::string id =
          options->customTypeHandler->toString(slice, options, *base);
      return TRI_V8_STD_STRING(id);
    }
    case VPackValueType::None:
    default: { return Nan::Undefined(); }
  }
}

struct BuilderContext {
  BuilderContext(v8::Isolate* isolate, VPackBuilder& builder,
                 bool keepTopLevelOpen)
      : isolate(isolate),
        builder(builder),
        level(0),
        keepTopLevelOpen(keepTopLevelOpen) {}

  v8::Isolate* isolate;
  v8::Local<v8::Value> toJsonKey;
  VPackBuilder& builder;
  int level;
  bool keepTopLevelOpen;
};

/// @brief adds a VPackValue to either an array or an object
template <typename T, bool inObject>
static inline void AddValue(BuilderContext& context,
                            arangodb::StringRef const& attributeName,
                            T const& value) {
  if (inObject) {
    context.builder.add(attributeName.begin(), attributeName.size(), value);
  } else {
    context.builder.add(value);
  }
}

/// @brief convert a V8 value to a VPack value
template <bool performAllChecks, bool inObject>
int V8ToVPack(BuilderContext& context,
                     v8::Local<v8::Value> const parameter,
                     arangodb::StringRef const& attributeName) {

  if (parameter->IsNull() || parameter->IsUndefined()) {
    AddValue<VPackValue, inObject>(context, attributeName,
                                   VPackValue(VPackValueType::Null));
    return TRI_ERROR_NO_ERROR;
  }

  if (parameter->IsBoolean()) {
    AddValue<VPackValue, inObject>(context, attributeName,
                                   VPackValue(parameter->ToBoolean()->Value()));
    return TRI_ERROR_NO_ERROR;
  }

  if (parameter->IsNumber()) {
    if (parameter->IsInt32()) {
      AddValue<VPackValue, inObject>(context, attributeName,
                                     VPackValue(parameter->ToInt32()->Value()));
      return TRI_ERROR_NO_ERROR;
    }

    if (parameter->IsUint32()) {
      AddValue<VPackValue, inObject>(
          context, attributeName, VPackValue(parameter->ToUint32()->Value()));
      return TRI_ERROR_NO_ERROR;
    }

    AddValue<VPackValue, inObject>(context, attributeName,
                                   VPackValue(parameter->ToNumber()->Value()));
    return TRI_ERROR_NO_ERROR;
  }

  if (parameter->IsString()) {
    v8::String::Utf8Value str(parameter->ToString());

    if (*str == nullptr) {
      return TRI_ERROR_OUT_OF_MEMORY;
    }

    AddValue<VPackValuePair, inObject>(
        context, attributeName,
        VPackValuePair(*str, str.length(), VPackValueType::String));
    return TRI_ERROR_NO_ERROR;
  }

  if (parameter->IsArray()) {
    v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(parameter);

    AddValue<VPackValue, inObject>(context, attributeName,
                                   VPackValue(VPackValueType::Array));
    uint32_t const n = array->Length();

    for (uint32_t i = 0; i < n; ++i) {
      v8::Local<v8::Value> value = array->Get(i);
      if (value->IsUndefined()) {
        // ignore array values which are undefined
        continue;
      }

      if (++context.level > MaxLevels) {
        // too much recursion
        return TRI_ERROR_BAD_PARAMETER;
      }

      int res = V8ToVPack<performAllChecks, false>(context, value,
                                                   arangodb::StringRef());

      --context.level;

      if (res != TRI_ERROR_NO_ERROR) {
        return res;
      }
    }

    if (!context.keepTopLevelOpen || context.level > 0) {
      context.builder.close();
    }
    return TRI_ERROR_NO_ERROR;
  }

  if (parameter->IsObject()) {
    if (performAllChecks) {
      if (parameter->IsBooleanObject()) {
        AddValue<VPackValue, inObject>(
            context, attributeName,
            VPackValue(v8::Local<v8::BooleanObject>::Cast(parameter)
                           ->BooleanValue()));
        return TRI_ERROR_NO_ERROR;
      }

      if (parameter->IsNumberObject()) {
        AddValue<VPackValue, inObject>(
            context, attributeName,
            VPackValue(
                v8::Local<v8::NumberObject>::Cast(parameter)->NumberValue()));
        return TRI_ERROR_NO_ERROR;
      }

      if (parameter->IsStringObject()) {
        v8::String::Utf8Value str(parameter->ToString());

        if (*str == nullptr) {
          return TRI_ERROR_OUT_OF_MEMORY;
        }

        AddValue<VPackValuePair, inObject>(
            context, attributeName,
            VPackValuePair(*str, str.length(), VPackValueType::String));
        return TRI_ERROR_NO_ERROR;
      }

      if (parameter->IsRegExp() || parameter->IsFunction() ||
          parameter->IsExternal()) {
        return TRI_ERROR_BAD_PARAMETER;
      }
    }

    v8::Local<v8::Object> o = parameter->ToObject();

    if (performAllChecks) {
      // first check if the object has a "toJSON" function
      if (o->Has(Nan::To<v8::String>(context.toJsonKey).ToLocalChecked())) {
        // call it if yes
        v8::Local<v8::Value> func = o->Get(context.toJsonKey);
        if (func->IsFunction()) {
          v8::Local<v8::Function> toJson =
              v8::Local<v8::Function>::Cast(func);

          v8::Local<v8::Value> args;
          v8::Local<v8::Value> converted = toJson->Call(o, 0, &args);

          if (!converted.IsEmpty()) {
            // return whatever toJSON returned
            v8::String::Utf8Value str(converted->ToString());

            if (*str == nullptr) {
              return TRI_ERROR_OUT_OF_MEMORY;
            }

            // this passes ownership for the utf8 string to the JSON object
            AddValue<VPackValuePair, inObject>(
                context, attributeName,
                VPackValuePair(*str, str.length(), VPackValueType::String));
            return TRI_ERROR_NO_ERROR;
          }
        }

        // fall-through intentional
      }
    }

    v8::Local<v8::Array> names = o->GetOwnPropertyNames();
    uint32_t const n = names->Length();

    AddValue<VPackValue, inObject>(context, attributeName,
                                   VPackValue(VPackValueType::Object));

    for (uint32_t i = 0; i < n; ++i) {
      // process attribute name
      v8::Local<v8::Value> key = names->Get(i);
      v8::String::Utf8Value str(key);

      if (*str == nullptr) {
        return TRI_ERROR_OUT_OF_MEMORY;
      }

      v8::Local<v8::Value> value = o->Get(key);
      if (value->IsUndefined()) {
        // ignore object values which are undefined
        continue;
      }

      if (++context.level > MaxLevels) {
        // too much recursion
        return TRI_ERROR_BAD_PARAMETER;
      }

      int res = V8ToVPack<performAllChecks, true>(
          context, value, arangodb::StringRef(*str, str.length()));

      --context.level;

      if (res != TRI_ERROR_NO_ERROR) {
        return res;
      }
    }

    if (!context.keepTopLevelOpen || context.level > 0) {
      context.builder.close();
    }
    return TRI_ERROR_NO_ERROR;
  }

  return TRI_ERROR_BAD_PARAMETER;
}

/// @brief convert a V8 value to VPack value
// node helper ////////////////////////////////////////////////////////////////////////////////
int TRI_V8ToVPack(v8::Isolate* isolate, VPackBuilder& builder,
                  v8::Local<v8::Value> const value, bool keepTopLevelOpen) {
  Nan::HandleScope scope;
  BuilderContext context(isolate, builder, keepTopLevelOpen);
  context.toJsonKey = Nan::New("toJSON").ToLocalChecked();
  return V8ToVPack<true, false>(context, value, arangodb::StringRef());
}

// node interface ////////////////////////////////////////////////////////////////////////////////
NAN_METHOD(decode) {
    if (info.Length() < 1) {
        Nan::ThrowError("Wrong number of arguments");
        return;
    }
    char* buf = ::node::Buffer::Data(info[0]);
    VPackSlice slice(buf);
    info.GetReturnValue().Set(TRI_VPackToV8(info.GetIsolate(), slice, &::arangodb::velocypack::Options::Defaults));
}

NAN_METHOD(encode) {
    if (info.Length() < 1) {
        Nan::ThrowError("Wrong number of arguments");
        return;
    }

    VPackBuilder builder;
    if (TRI_V8ToVPack(info.GetIsolate(), builder, info[0], false) != TRI_ERROR_NO_ERROR) {
        Nan::ThrowError("Failed transforming to vpack");
        return;
    }

    auto buffer = builder.buffer();
    info.GetReturnValue().Set(
        Nan::CopyBuffer(
            (char*)buffer->data(),
            buffer->size()
        ).ToLocalChecked()
    );
}

NAN_MODULE_INIT(Init){
    NAN_EXPORT(target, encode);
    NAN_EXPORT(target, decode);
}

}}

NODE_MODULE(vpack, arangodb::node::Init);
