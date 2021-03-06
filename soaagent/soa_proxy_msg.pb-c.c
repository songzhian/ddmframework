/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: soa_proxy_msg.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "soa_proxy_msg.pb-c.h"
void   soa_proxy_request__init
                     (SoaProxyRequest         *message)
{
  static SoaProxyRequest init_value = SOA_PROXY_REQUEST__INIT;
  *message = init_value;
}
size_t soa_proxy_request__get_packed_size
                     (const SoaProxyRequest *message)
{
  assert(message->base.descriptor == &soa_proxy_request__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t soa_proxy_request__pack
                     (const SoaProxyRequest *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &soa_proxy_request__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t soa_proxy_request__pack_to_buffer
                     (const SoaProxyRequest *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &soa_proxy_request__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
SoaProxyRequest *
       soa_proxy_request__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (SoaProxyRequest *)
     protobuf_c_message_unpack (&soa_proxy_request__descriptor,
                                allocator, len, data);
}
void   soa_proxy_request__free_unpacked
                     (SoaProxyRequest *message,
                      ProtobufCAllocator *allocator)
{
  assert(message->base.descriptor == &soa_proxy_request__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   soa_proxy_response__init
                     (SoaProxyResponse         *message)
{
  static SoaProxyResponse init_value = SOA_PROXY_RESPONSE__INIT;
  *message = init_value;
}
size_t soa_proxy_response__get_packed_size
                     (const SoaProxyResponse *message)
{
  assert(message->base.descriptor == &soa_proxy_response__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t soa_proxy_response__pack
                     (const SoaProxyResponse *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &soa_proxy_response__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t soa_proxy_response__pack_to_buffer
                     (const SoaProxyResponse *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &soa_proxy_response__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
SoaProxyResponse *
       soa_proxy_response__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (SoaProxyResponse *)
     protobuf_c_message_unpack (&soa_proxy_response__descriptor,
                                allocator, len, data);
}
void   soa_proxy_response__free_unpacked
                     (SoaProxyResponse *message,
                      ProtobufCAllocator *allocator)
{
  assert(message->base.descriptor == &soa_proxy_response__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor soa_proxy_request__field_descriptors[4] =
{
  {
    "group_name",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(SoaProxyRequest, group_name),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "service_name",
    2,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(SoaProxyRequest, service_name),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "service_version",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(SoaProxyRequest, service_version),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "request",
    4,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_BYTES,
    0,   /* quantifier_offset */
    offsetof(SoaProxyRequest, request),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned soa_proxy_request__field_indices_by_name[] = {
  0,   /* field[0] = group_name */
  3,   /* field[3] = request */
  1,   /* field[1] = service_name */
  2,   /* field[2] = service_version */
};
static const ProtobufCIntRange soa_proxy_request__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor soa_proxy_request__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "soa_proxy_request",
  "SoaProxyRequest",
  "SoaProxyRequest",
  "",
  sizeof(SoaProxyRequest),
  4,
  soa_proxy_request__field_descriptors,
  soa_proxy_request__field_indices_by_name,
  1,  soa_proxy_request__number_ranges,
  (ProtobufCMessageInit) soa_proxy_request__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor soa_proxy_response__field_descriptors[2] =
{
  {
    "status",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(SoaProxyResponse, status),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "response",
    2,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_BYTES,
    0,   /* quantifier_offset */
    offsetof(SoaProxyResponse, response),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned soa_proxy_response__field_indices_by_name[] = {
  1,   /* field[1] = response */
  0,   /* field[0] = status */
};
static const ProtobufCIntRange soa_proxy_response__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 2 }
};
const ProtobufCMessageDescriptor soa_proxy_response__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "soa_proxy_response",
  "SoaProxyResponse",
  "SoaProxyResponse",
  "",
  sizeof(SoaProxyResponse),
  2,
  soa_proxy_response__field_descriptors,
  soa_proxy_response__field_indices_by_name,
  1,  soa_proxy_response__number_ranges,
  (ProtobufCMessageInit) soa_proxy_response__init,
  NULL,NULL,NULL    /* reserved[123] */
};
