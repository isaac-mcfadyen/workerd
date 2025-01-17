// Copyright (c) 2017-2022 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
//     https://opensource.org/licenses/Apache-2.0

#pragma once

#include "readable.h"
#include "writable.h"

namespace workerd::api {

class TransformStream: public jsg::Object {
  // A TransformStream is a readable, writable pair in which whatever is written to the writable
  // side can be read from the readable side, possibly transformed into a different type of value.
  //
  // The original version of TransformStream in Workers was nothing more than an identity
  // passthrough that only handled byte data. No actual transformation of the value was performed.
  // The original version did not conform to the streams standard. That original version has been
  // migrated into the IdentityTransformStream class. If the transformstream_enable_standard_constructor
  // feature flag is not enabled, then TransformStream is just an alias for IdentityTransformStream
  // and continues to implement the non-standard behavior. With the transformstream_enable_standard_constructor
  // flag set, however, the TransformStream implements standardized behavior.

public:
  explicit TransformStream(
      jsg::Ref<ReadableStream> readable,
      jsg::Ref<WritableStream> writable)
      : readable(kj::mv(readable)),
        writable(kj::mv(writable)) {}

  static jsg::Ref<TransformStream> constructor(
      jsg::Lock& js,
      jsg::Optional<Transformer> maybeTransformer,
      jsg::Optional<StreamQueuingStrategy> maybeWritableStrategy,
      jsg::Optional<StreamQueuingStrategy> maybeReadableStrategy,
      CompatibilityFlags::Reader flags);

  jsg::Ref<ReadableStream> getReadable() {
    return readable.addRef();
  }
  jsg::Ref<WritableStream> getWritable() {
    return writable.addRef();
  }

  JSG_RESOURCE_TYPE(TransformStream, CompatibilityFlags::Reader flags) {
    if (flags.getJsgPropertyOnPrototypeTemplate()) {
      JSG_READONLY_PROTOTYPE_PROPERTY(readable, getReadable);
      JSG_READONLY_PROTOTYPE_PROPERTY(writable, getWritable);
    } else {
      JSG_READONLY_INSTANCE_PROPERTY(readable, getReadable);
      JSG_READONLY_INSTANCE_PROPERTY(writable, getWritable);
    }
  }

private:
  jsg::Ref<ReadableStream> readable;
  jsg::Ref<WritableStream> writable;

  void visitForGc(jsg::GcVisitor& visitor) {
    visitor.visit(readable, writable);
  }
};

class IdentityTransformStream: public TransformStream {
  // The IdentityTransformStream is a non-standard TransformStream implementation that passes
  // the exact bytes written to the writable side on to the readable side without modification.
  // Unlike standard the TransformStream, the readable side of an IdentityTransformStream
  // supports BYOB reads.
public:
  using TransformStream::TransformStream;

  static jsg::Ref<IdentityTransformStream> constructor(jsg::Lock& js);

  JSG_RESOURCE_TYPE(IdentityTransformStream) {
    JSG_INHERIT(TransformStream);
  }
};

class FixedLengthStream: public IdentityTransformStream {
  // Same as an IdentityTransformStream, except with a known length in bytes on the readable side.
  // We don't currently enforce this limit -- it just convinces the kj-http layer to
  // emit a Content-Length (assuming it doesn't get gzipped or anything).

public:
  using IdentityTransformStream::IdentityTransformStream;

  static jsg::Ref<FixedLengthStream> constructor(jsg::Lock& js, uint64_t expectedLength);

  JSG_RESOURCE_TYPE(FixedLengthStream) {
    JSG_INHERIT(IdentityTransformStream);
  }
};

}  // namespace workerd::api
