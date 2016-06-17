# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: loomplan.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
from google.protobuf import descriptor_pb2
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='loomplan.proto',
  package='loomplan',
  serialized_pb=_b('\n\x0eloomplan.proto\x12\x08loomplan\"<\n\x04Task\x12\x11\n\ttask_type\x18\x01 \x02(\x05\x12\x0e\n\x06\x63onfig\x18\x02 \x02(\x0c\x12\x11\n\tinput_ids\x18\x03 \x03(\x05\"M\n\x04Plan\x12\x12\n\ntask_types\x18\x01 \x03(\t\x12\x1d\n\x05tasks\x18\x02 \x03(\x0b\x32\x0e.loomplan.Task\x12\x12\n\nresult_ids\x18\x03 \x03(\x05\x42\x02H\x03')
)
_sym_db.RegisterFileDescriptor(DESCRIPTOR)




_TASK = _descriptor.Descriptor(
  name='Task',
  full_name='loomplan.Task',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='task_type', full_name='loomplan.Task.task_type', index=0,
      number=1, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='config', full_name='loomplan.Task.config', index=1,
      number=2, type=12, cpp_type=9, label=2,
      has_default_value=False, default_value=_b(""),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='input_ids', full_name='loomplan.Task.input_ids', index=2,
      number=3, type=5, cpp_type=1, label=3,
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
  serialized_start=28,
  serialized_end=88,
)


_PLAN = _descriptor.Descriptor(
  name='Plan',
  full_name='loomplan.Plan',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='task_types', full_name='loomplan.Plan.task_types', index=0,
      number=1, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='tasks', full_name='loomplan.Plan.tasks', index=1,
      number=2, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='result_ids', full_name='loomplan.Plan.result_ids', index=2,
      number=3, type=5, cpp_type=1, label=3,
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
  serialized_start=90,
  serialized_end=167,
)

_PLAN.fields_by_name['tasks'].message_type = _TASK
DESCRIPTOR.message_types_by_name['Task'] = _TASK
DESCRIPTOR.message_types_by_name['Plan'] = _PLAN

Task = _reflection.GeneratedProtocolMessageType('Task', (_message.Message,), dict(
  DESCRIPTOR = _TASK,
  __module__ = 'loomplan_pb2'
  # @@protoc_insertion_point(class_scope:loomplan.Task)
  ))
_sym_db.RegisterMessage(Task)

Plan = _reflection.GeneratedProtocolMessageType('Plan', (_message.Message,), dict(
  DESCRIPTOR = _PLAN,
  __module__ = 'loomplan_pb2'
  # @@protoc_insertion_point(class_scope:loomplan.Plan)
  ))
_sym_db.RegisterMessage(Plan)


DESCRIPTOR.has_options = True
DESCRIPTOR._options = _descriptor._ParseOptions(descriptor_pb2.FileOptions(), _b('H\003'))
# @@protoc_insertion_point(module_scope)
