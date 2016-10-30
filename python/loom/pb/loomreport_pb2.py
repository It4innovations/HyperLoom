# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: loomreport.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
from google.protobuf import descriptor_pb2
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


from . import loomplan_pb2
from . import loomcomm_pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='loomreport.proto',
  package='loomcomm',
  serialized_pb=_b('\n\x10loomreport.proto\x12\x08loomcomm\x1a\x0eloomplan.proto\x1a\x0eloomcomm.proto\"X\n\x06Report\x12\x0f\n\x07symbols\x18\x01 \x03(\t\x12\x1c\n\x04plan\x18\x02 \x02(\x0b\x32\x0e.loomplan.Plan\x12\x1f\n\x06\x65vents\x18\x03 \x03(\x0b\x32\x0f.loomcomm.EventB\x02H\x03')
  ,
  dependencies=[loomplan_pb2.DESCRIPTOR,loomcomm_pb2.DESCRIPTOR,])
_sym_db.RegisterFileDescriptor(DESCRIPTOR)




_REPORT = _descriptor.Descriptor(
  name='Report',
  full_name='loomcomm.Report',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='symbols', full_name='loomcomm.Report.symbols', index=0,
      number=1, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='plan', full_name='loomcomm.Report.plan', index=1,
      number=2, type=11, cpp_type=10, label=2,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='events', full_name='loomcomm.Report.events', index=2,
      number=3, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  options=None,
  is_extendable=False,
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=62,
  serialized_end=150,
)

_REPORT.fields_by_name['plan'].message_type = loomplan_pb2._PLAN
_REPORT.fields_by_name['events'].message_type = loomcomm_pb2._EVENT
DESCRIPTOR.message_types_by_name['Report'] = _REPORT

Report = _reflection.GeneratedProtocolMessageType('Report', (_message.Message,), dict(
  DESCRIPTOR = _REPORT,
  __module__ = 'loomreport_pb2'
  # @@protoc_insertion_point(class_scope:loomcomm.Report)
  ))
_sym_db.RegisterMessage(Report)


DESCRIPTOR.has_options = True
DESCRIPTOR._options = _descriptor._ParseOptions(descriptor_pb2.FileOptions(), _b('H\003'))
# @@protoc_insertion_point(module_scope)
