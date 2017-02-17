# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: loomcomm.proto

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


DESCRIPTOR = _descriptor.FileDescriptor(
  name='loomcomm.proto',
  package='loomcomm',
  serialized_pb=_b('\n\x0eloomcomm.proto\x12\x08loomcomm\x1a\x0eloomplan.proto\"\xc1\x01\n\x08Register\x12\x18\n\x10protocol_version\x18\x01 \x02(\x05\x12%\n\x04type\x18\x02 \x02(\x0e\x32\x17.loomcomm.Register.Type\x12\x0c\n\x04port\x18\x03 \x01(\x05\x12\x12\n\ntask_types\x18\x04 \x03(\t\x12\x12\n\ndata_types\x18\x05 \x03(\t\x12\x0c\n\x04\x63pus\x18\x06 \x01(\x05\"0\n\x04Type\x12\x13\n\x0fREGISTER_WORKER\x10\x01\x12\x13\n\x0fREGISTER_CLIENT\x10\x02\"&\n\rServerMessage\"\x15\n\x04Type\x12\r\n\tSTART_JOB\x10\x01\"\xa1\x02\n\rWorkerCommand\x12*\n\x04type\x18\x01 \x02(\x0e\x32\x1c.loomcomm.WorkerCommand.Type\x12\n\n\x02id\x18\x02 \x01(\x05\x12\x11\n\ttask_type\x18\x03 \x01(\x05\x12\x13\n\x0btask_config\x18\x04 \x01(\t\x12\x13\n\x0btask_inputs\x18\x05 \x03(\x05\x12\x0e\n\x06n_cpus\x18\x06 \x01(\x05\x12\x0f\n\x07\x61\x64\x64ress\x18\n \x01(\t\x12\x0f\n\x07symbols\x18\x64 \x03(\t\x12\x12\n\ntrace_path\x18x \x01(\t\x12\x11\n\tworker_id\x18y \x01(\x05\"B\n\x04Type\x12\x08\n\x04TASK\x10\x01\x12\x08\n\x04SEND\x10\x02\x12\n\n\x06REMOVE\x10\x03\x12\x0e\n\nDICTIONARY\x10\x08\x12\n\n\x06UPDATE\x10\t\"\xac\x01\n\x0eWorkerResponse\x12+\n\x04type\x18\x01 \x02(\x0e\x32\x1d.loomcomm.WorkerResponse.Type\x12\n\n\x02id\x18\x02 \x02(\x05\x12\x0c\n\x04size\x18\x03 \x01(\x04\x12\x0e\n\x06length\x18\x04 \x01(\x04\x12\x11\n\terror_msg\x18\x64 \x01(\t\"0\n\x04Type\x12\x0c\n\x08\x46INISHED\x10\x01\x12\x0e\n\nTRANSFERED\x10\x02\x12\n\n\x06\x46\x41ILED\x10\x03\"\x18\n\x08\x41nnounce\x12\x0c\n\x04port\x18\x01 \x02(\x05\"=\n\nDataHeader\x12\n\n\x02id\x18\x01 \x02(\x05\x12\x0f\n\x07type_id\x18\x03 \x02(\x05\x12\x12\n\nn_messages\x18\x02 \x02(\x03\"\xc4\x01\n\x05\x45vent\x12\x0c\n\x04time\x18\x01 \x02(\x04\x12\"\n\x04type\x18\x02 \x02(\x0e\x32\x14.loomcomm.Event.Type\x12\n\n\x02id\x18\x03 \x02(\x05\x12\x11\n\tworker_id\x18\x04 \x01(\x05\x12\x0c\n\x04size\x18\x05 \x01(\x04\x12\x18\n\x10target_worker_id\x18\x06 \x01(\x05\"B\n\x04Type\x12\x0e\n\nTASK_START\x10\x01\x12\x0c\n\x08TASK_END\x10\x02\x12\x0e\n\nSEND_START\x10\x03\x12\x0c\n\x08SEND_END\x10\x04\"6\n\x05\x45rror\x12\n\n\x02id\x18\x01 \x02(\x05\x12\x0e\n\x06worker\x18\x02 \x02(\t\x12\x11\n\terror_msg\x18\x03 \x02(\t\"2\n\x05Stats\x12\x11\n\tn_workers\x18\x01 \x01(\x05\x12\x16\n\x0en_data_objects\x18\x02 \x01(\x05\"\x95\x02\n\x0e\x43lientResponse\x12+\n\x04type\x18\x01 \x02(\x0e\x32\x1d.loomcomm.ClientResponse.Type\x12\"\n\x04\x64\x61ta\x18\x02 \x01(\x0b\x32\x14.loomcomm.DataHeader\x12\x1e\n\x05\x65vent\x18\x03 \x01(\x0b\x32\x0f.loomcomm.Event\x12\x1e\n\x05\x65rror\x18\x04 \x01(\x0b\x32\x0f.loomcomm.Error\x12\x0f\n\x07symbols\x18\x05 \x03(\t\x12\x1e\n\x05stats\x18\x06 \x01(\x0b\x32\x0f.loomcomm.Stats\"A\n\x04Type\x12\x08\n\x04\x44\x41TA\x10\x01\x12\t\n\x05\x45VENT\x10\x02\x12\t\n\x05\x45RROR\x10\x03\x12\x0e\n\nDICTIONARY\x10\x04\x12\t\n\x05STATS\x10\x05\"\xbb\x01\n\rClientRequest\x12*\n\x04type\x18\x01 \x02(\x0e\x32\x1c.loomcomm.ClientRequest.Type\x12\x1c\n\x04plan\x18\x02 \x01(\x0b\x32\x0e.loomplan.Plan\x12\x15\n\x06report\x18\x03 \x01(\x08:\x05\x66\x61lse\x12\x12\n\ntrace_path\x18\x06 \x01(\t\"5\n\x04Type\x12\x08\n\x04PLAN\x10\x01\x12\t\n\x05STATS\x10\x02\x12\t\n\x05TRACE\x10\x03\x12\r\n\tTERMINATE\x10\nB\x02H\x03')
  ,
  dependencies=[loomplan_pb2.DESCRIPTOR,])
_sym_db.RegisterFileDescriptor(DESCRIPTOR)



_REGISTER_TYPE = _descriptor.EnumDescriptor(
  name='Type',
  full_name='loomcomm.Register.Type',
  filename=None,
  file=DESCRIPTOR,
  values=[
    _descriptor.EnumValueDescriptor(
      name='REGISTER_WORKER', index=0, number=1,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='REGISTER_CLIENT', index=1, number=2,
      options=None,
      type=None),
  ],
  containing_type=None,
  options=None,
  serialized_start=190,
  serialized_end=238,
)
_sym_db.RegisterEnumDescriptor(_REGISTER_TYPE)

_SERVERMESSAGE_TYPE = _descriptor.EnumDescriptor(
  name='Type',
  full_name='loomcomm.ServerMessage.Type',
  filename=None,
  file=DESCRIPTOR,
  values=[
    _descriptor.EnumValueDescriptor(
      name='START_JOB', index=0, number=1,
      options=None,
      type=None),
  ],
  containing_type=None,
  options=None,
  serialized_start=257,
  serialized_end=278,
)
_sym_db.RegisterEnumDescriptor(_SERVERMESSAGE_TYPE)

_WORKERCOMMAND_TYPE = _descriptor.EnumDescriptor(
  name='Type',
  full_name='loomcomm.WorkerCommand.Type',
  filename=None,
  file=DESCRIPTOR,
  values=[
    _descriptor.EnumValueDescriptor(
      name='TASK', index=0, number=1,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='SEND', index=1, number=2,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='REMOVE', index=2, number=3,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='DICTIONARY', index=3, number=8,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='UPDATE', index=4, number=9,
      options=None,
      type=None),
  ],
  containing_type=None,
  options=None,
  serialized_start=504,
  serialized_end=570,
)
_sym_db.RegisterEnumDescriptor(_WORKERCOMMAND_TYPE)

_WORKERRESPONSE_TYPE = _descriptor.EnumDescriptor(
  name='Type',
  full_name='loomcomm.WorkerResponse.Type',
  filename=None,
  file=DESCRIPTOR,
  values=[
    _descriptor.EnumValueDescriptor(
      name='FINISHED', index=0, number=1,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='TRANSFERED', index=1, number=2,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='FAILED', index=2, number=3,
      options=None,
      type=None),
  ],
  containing_type=None,
  options=None,
  serialized_start=697,
  serialized_end=745,
)
_sym_db.RegisterEnumDescriptor(_WORKERRESPONSE_TYPE)

_EVENT_TYPE = _descriptor.EnumDescriptor(
  name='Type',
  full_name='loomcomm.Event.Type',
  filename=None,
  file=DESCRIPTOR,
  values=[
    _descriptor.EnumValueDescriptor(
      name='TASK_START', index=0, number=1,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='TASK_END', index=1, number=2,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='SEND_START', index=2, number=3,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='SEND_END', index=3, number=4,
      options=None,
      type=None),
  ],
  containing_type=None,
  options=None,
  serialized_start=967,
  serialized_end=1033,
)
_sym_db.RegisterEnumDescriptor(_EVENT_TYPE)

_CLIENTRESPONSE_TYPE = _descriptor.EnumDescriptor(
  name='Type',
  full_name='loomcomm.ClientResponse.Type',
  filename=None,
  file=DESCRIPTOR,
  values=[
    _descriptor.EnumValueDescriptor(
      name='DATA', index=0, number=1,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='EVENT', index=1, number=2,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='ERROR', index=2, number=3,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='DICTIONARY', index=3, number=4,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='STATS', index=4, number=5,
      options=None,
      type=None),
  ],
  containing_type=None,
  options=None,
  serialized_start=1356,
  serialized_end=1421,
)
_sym_db.RegisterEnumDescriptor(_CLIENTRESPONSE_TYPE)

_CLIENTREQUEST_TYPE = _descriptor.EnumDescriptor(
  name='Type',
  full_name='loomcomm.ClientRequest.Type',
  filename=None,
  file=DESCRIPTOR,
  values=[
    _descriptor.EnumValueDescriptor(
      name='PLAN', index=0, number=1,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='STATS', index=1, number=2,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='TRACE', index=2, number=3,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='TERMINATE', index=3, number=10,
      options=None,
      type=None),
  ],
  containing_type=None,
  options=None,
  serialized_start=1558,
  serialized_end=1611,
)
_sym_db.RegisterEnumDescriptor(_CLIENTREQUEST_TYPE)


_REGISTER = _descriptor.Descriptor(
  name='Register',
  full_name='loomcomm.Register',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='protocol_version', full_name='loomcomm.Register.protocol_version', index=0,
      number=1, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='type', full_name='loomcomm.Register.type', index=1,
      number=2, type=14, cpp_type=8, label=2,
      has_default_value=False, default_value=1,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='port', full_name='loomcomm.Register.port', index=2,
      number=3, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='task_types', full_name='loomcomm.Register.task_types', index=3,
      number=4, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='data_types', full_name='loomcomm.Register.data_types', index=4,
      number=5, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='cpus', full_name='loomcomm.Register.cpus', index=5,
      number=6, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
    _REGISTER_TYPE,
  ],
  options=None,
  is_extendable=False,
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=45,
  serialized_end=238,
)


_SERVERMESSAGE = _descriptor.Descriptor(
  name='ServerMessage',
  full_name='loomcomm.ServerMessage',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
    _SERVERMESSAGE_TYPE,
  ],
  options=None,
  is_extendable=False,
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=240,
  serialized_end=278,
)


_WORKERCOMMAND = _descriptor.Descriptor(
  name='WorkerCommand',
  full_name='loomcomm.WorkerCommand',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='type', full_name='loomcomm.WorkerCommand.type', index=0,
      number=1, type=14, cpp_type=8, label=2,
      has_default_value=False, default_value=1,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='id', full_name='loomcomm.WorkerCommand.id', index=1,
      number=2, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='task_type', full_name='loomcomm.WorkerCommand.task_type', index=2,
      number=3, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='task_config', full_name='loomcomm.WorkerCommand.task_config', index=3,
      number=4, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='task_inputs', full_name='loomcomm.WorkerCommand.task_inputs', index=4,
      number=5, type=5, cpp_type=1, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='n_cpus', full_name='loomcomm.WorkerCommand.n_cpus', index=5,
      number=6, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='address', full_name='loomcomm.WorkerCommand.address', index=6,
      number=10, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='symbols', full_name='loomcomm.WorkerCommand.symbols', index=7,
      number=100, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='trace_path', full_name='loomcomm.WorkerCommand.trace_path', index=8,
      number=120, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='worker_id', full_name='loomcomm.WorkerCommand.worker_id', index=9,
      number=121, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
    _WORKERCOMMAND_TYPE,
  ],
  options=None,
  is_extendable=False,
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=281,
  serialized_end=570,
)


_WORKERRESPONSE = _descriptor.Descriptor(
  name='WorkerResponse',
  full_name='loomcomm.WorkerResponse',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='type', full_name='loomcomm.WorkerResponse.type', index=0,
      number=1, type=14, cpp_type=8, label=2,
      has_default_value=False, default_value=1,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='id', full_name='loomcomm.WorkerResponse.id', index=1,
      number=2, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='size', full_name='loomcomm.WorkerResponse.size', index=2,
      number=3, type=4, cpp_type=4, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='length', full_name='loomcomm.WorkerResponse.length', index=3,
      number=4, type=4, cpp_type=4, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='error_msg', full_name='loomcomm.WorkerResponse.error_msg', index=4,
      number=100, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
    _WORKERRESPONSE_TYPE,
  ],
  options=None,
  is_extendable=False,
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=573,
  serialized_end=745,
)


_ANNOUNCE = _descriptor.Descriptor(
  name='Announce',
  full_name='loomcomm.Announce',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='port', full_name='loomcomm.Announce.port', index=0,
      number=1, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
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
  serialized_start=747,
  serialized_end=771,
)


_DATAHEADER = _descriptor.Descriptor(
  name='DataHeader',
  full_name='loomcomm.DataHeader',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='id', full_name='loomcomm.DataHeader.id', index=0,
      number=1, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='type_id', full_name='loomcomm.DataHeader.type_id', index=1,
      number=3, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='n_messages', full_name='loomcomm.DataHeader.n_messages', index=2,
      number=2, type=3, cpp_type=2, label=2,
      has_default_value=False, default_value=0,
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
  serialized_start=773,
  serialized_end=834,
)


_EVENT = _descriptor.Descriptor(
  name='Event',
  full_name='loomcomm.Event',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='time', full_name='loomcomm.Event.time', index=0,
      number=1, type=4, cpp_type=4, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='type', full_name='loomcomm.Event.type', index=1,
      number=2, type=14, cpp_type=8, label=2,
      has_default_value=False, default_value=1,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='id', full_name='loomcomm.Event.id', index=2,
      number=3, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='worker_id', full_name='loomcomm.Event.worker_id', index=3,
      number=4, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='size', full_name='loomcomm.Event.size', index=4,
      number=5, type=4, cpp_type=4, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='target_worker_id', full_name='loomcomm.Event.target_worker_id', index=5,
      number=6, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
    _EVENT_TYPE,
  ],
  options=None,
  is_extendable=False,
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=837,
  serialized_end=1033,
)


_ERROR = _descriptor.Descriptor(
  name='Error',
  full_name='loomcomm.Error',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='id', full_name='loomcomm.Error.id', index=0,
      number=1, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='worker', full_name='loomcomm.Error.worker', index=1,
      number=2, type=9, cpp_type=9, label=2,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='error_msg', full_name='loomcomm.Error.error_msg', index=2,
      number=3, type=9, cpp_type=9, label=2,
      has_default_value=False, default_value=_b("").decode('utf-8'),
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
  serialized_start=1035,
  serialized_end=1089,
)


_STATS = _descriptor.Descriptor(
  name='Stats',
  full_name='loomcomm.Stats',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='n_workers', full_name='loomcomm.Stats.n_workers', index=0,
      number=1, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='n_data_objects', full_name='loomcomm.Stats.n_data_objects', index=1,
      number=2, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
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
  serialized_start=1091,
  serialized_end=1141,
)


_CLIENTRESPONSE = _descriptor.Descriptor(
  name='ClientResponse',
  full_name='loomcomm.ClientResponse',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='type', full_name='loomcomm.ClientResponse.type', index=0,
      number=1, type=14, cpp_type=8, label=2,
      has_default_value=False, default_value=1,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='data', full_name='loomcomm.ClientResponse.data', index=1,
      number=2, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='event', full_name='loomcomm.ClientResponse.event', index=2,
      number=3, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='error', full_name='loomcomm.ClientResponse.error', index=3,
      number=4, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='symbols', full_name='loomcomm.ClientResponse.symbols', index=4,
      number=5, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='stats', full_name='loomcomm.ClientResponse.stats', index=5,
      number=6, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
    _CLIENTRESPONSE_TYPE,
  ],
  options=None,
  is_extendable=False,
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=1144,
  serialized_end=1421,
)


_CLIENTREQUEST = _descriptor.Descriptor(
  name='ClientRequest',
  full_name='loomcomm.ClientRequest',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='type', full_name='loomcomm.ClientRequest.type', index=0,
      number=1, type=14, cpp_type=8, label=2,
      has_default_value=False, default_value=1,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='plan', full_name='loomcomm.ClientRequest.plan', index=1,
      number=2, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='report', full_name='loomcomm.ClientRequest.report', index=2,
      number=3, type=8, cpp_type=7, label=1,
      has_default_value=True, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='trace_path', full_name='loomcomm.ClientRequest.trace_path', index=3,
      number=6, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
    _CLIENTREQUEST_TYPE,
  ],
  options=None,
  is_extendable=False,
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=1424,
  serialized_end=1611,
)

_REGISTER.fields_by_name['type'].enum_type = _REGISTER_TYPE
_REGISTER_TYPE.containing_type = _REGISTER
_SERVERMESSAGE_TYPE.containing_type = _SERVERMESSAGE
_WORKERCOMMAND.fields_by_name['type'].enum_type = _WORKERCOMMAND_TYPE
_WORKERCOMMAND_TYPE.containing_type = _WORKERCOMMAND
_WORKERRESPONSE.fields_by_name['type'].enum_type = _WORKERRESPONSE_TYPE
_WORKERRESPONSE_TYPE.containing_type = _WORKERRESPONSE
_EVENT.fields_by_name['type'].enum_type = _EVENT_TYPE
_EVENT_TYPE.containing_type = _EVENT
_CLIENTRESPONSE.fields_by_name['type'].enum_type = _CLIENTRESPONSE_TYPE
_CLIENTRESPONSE.fields_by_name['data'].message_type = _DATAHEADER
_CLIENTRESPONSE.fields_by_name['event'].message_type = _EVENT
_CLIENTRESPONSE.fields_by_name['error'].message_type = _ERROR
_CLIENTRESPONSE.fields_by_name['stats'].message_type = _STATS
_CLIENTRESPONSE_TYPE.containing_type = _CLIENTRESPONSE
_CLIENTREQUEST.fields_by_name['type'].enum_type = _CLIENTREQUEST_TYPE
_CLIENTREQUEST.fields_by_name['plan'].message_type = loomplan_pb2._PLAN
_CLIENTREQUEST_TYPE.containing_type = _CLIENTREQUEST
DESCRIPTOR.message_types_by_name['Register'] = _REGISTER
DESCRIPTOR.message_types_by_name['ServerMessage'] = _SERVERMESSAGE
DESCRIPTOR.message_types_by_name['WorkerCommand'] = _WORKERCOMMAND
DESCRIPTOR.message_types_by_name['WorkerResponse'] = _WORKERRESPONSE
DESCRIPTOR.message_types_by_name['Announce'] = _ANNOUNCE
DESCRIPTOR.message_types_by_name['DataHeader'] = _DATAHEADER
DESCRIPTOR.message_types_by_name['Event'] = _EVENT
DESCRIPTOR.message_types_by_name['Error'] = _ERROR
DESCRIPTOR.message_types_by_name['Stats'] = _STATS
DESCRIPTOR.message_types_by_name['ClientResponse'] = _CLIENTRESPONSE
DESCRIPTOR.message_types_by_name['ClientRequest'] = _CLIENTREQUEST

Register = _reflection.GeneratedProtocolMessageType('Register', (_message.Message,), dict(
  DESCRIPTOR = _REGISTER,
  __module__ = 'loomcomm_pb2'
  # @@protoc_insertion_point(class_scope:loomcomm.Register)
  ))
_sym_db.RegisterMessage(Register)

ServerMessage = _reflection.GeneratedProtocolMessageType('ServerMessage', (_message.Message,), dict(
  DESCRIPTOR = _SERVERMESSAGE,
  __module__ = 'loomcomm_pb2'
  # @@protoc_insertion_point(class_scope:loomcomm.ServerMessage)
  ))
_sym_db.RegisterMessage(ServerMessage)

WorkerCommand = _reflection.GeneratedProtocolMessageType('WorkerCommand', (_message.Message,), dict(
  DESCRIPTOR = _WORKERCOMMAND,
  __module__ = 'loomcomm_pb2'
  # @@protoc_insertion_point(class_scope:loomcomm.WorkerCommand)
  ))
_sym_db.RegisterMessage(WorkerCommand)

WorkerResponse = _reflection.GeneratedProtocolMessageType('WorkerResponse', (_message.Message,), dict(
  DESCRIPTOR = _WORKERRESPONSE,
  __module__ = 'loomcomm_pb2'
  # @@protoc_insertion_point(class_scope:loomcomm.WorkerResponse)
  ))
_sym_db.RegisterMessage(WorkerResponse)

Announce = _reflection.GeneratedProtocolMessageType('Announce', (_message.Message,), dict(
  DESCRIPTOR = _ANNOUNCE,
  __module__ = 'loomcomm_pb2'
  # @@protoc_insertion_point(class_scope:loomcomm.Announce)
  ))
_sym_db.RegisterMessage(Announce)

DataHeader = _reflection.GeneratedProtocolMessageType('DataHeader', (_message.Message,), dict(
  DESCRIPTOR = _DATAHEADER,
  __module__ = 'loomcomm_pb2'
  # @@protoc_insertion_point(class_scope:loomcomm.DataHeader)
  ))
_sym_db.RegisterMessage(DataHeader)

Event = _reflection.GeneratedProtocolMessageType('Event', (_message.Message,), dict(
  DESCRIPTOR = _EVENT,
  __module__ = 'loomcomm_pb2'
  # @@protoc_insertion_point(class_scope:loomcomm.Event)
  ))
_sym_db.RegisterMessage(Event)

Error = _reflection.GeneratedProtocolMessageType('Error', (_message.Message,), dict(
  DESCRIPTOR = _ERROR,
  __module__ = 'loomcomm_pb2'
  # @@protoc_insertion_point(class_scope:loomcomm.Error)
  ))
_sym_db.RegisterMessage(Error)

Stats = _reflection.GeneratedProtocolMessageType('Stats', (_message.Message,), dict(
  DESCRIPTOR = _STATS,
  __module__ = 'loomcomm_pb2'
  # @@protoc_insertion_point(class_scope:loomcomm.Stats)
  ))
_sym_db.RegisterMessage(Stats)

ClientResponse = _reflection.GeneratedProtocolMessageType('ClientResponse', (_message.Message,), dict(
  DESCRIPTOR = _CLIENTRESPONSE,
  __module__ = 'loomcomm_pb2'
  # @@protoc_insertion_point(class_scope:loomcomm.ClientResponse)
  ))
_sym_db.RegisterMessage(ClientResponse)

ClientRequest = _reflection.GeneratedProtocolMessageType('ClientRequest', (_message.Message,), dict(
  DESCRIPTOR = _CLIENTREQUEST,
  __module__ = 'loomcomm_pb2'
  # @@protoc_insertion_point(class_scope:loomcomm.ClientRequest)
  ))
_sym_db.RegisterMessage(ClientRequest)


DESCRIPTOR.has_options = True
DESCRIPTOR._options = _descriptor._ParseOptions(descriptor_pb2.FileOptions(), _b('H\003'))
# @@protoc_insertion_point(module_scope)
