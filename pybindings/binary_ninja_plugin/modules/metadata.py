# author: Michal Melewski <carstein@google.com>
# FSS similarity search plugin - metadata class

import base64
import os

import binaryninja as bn


class Metadata:
  def __init__(self, meta_location):
    self.data = {}
    self.sim_hash_meta_location = meta_location

    if os.path.isfile(self.sim_hash_meta_location):
      with open(self.sim_hash_meta_location, "r") as fh:
        for line in fh.xreadlines():
          tokens = line.split()
          if tokens:
            self.add(long(tokens[0], 16), long(tokens[2], 16), base64.b64decode(tokens[3]), tokens[1])

  def add(self, exe_id, address, file_name, function_name):
    bn.log_info("[*] Adding metadata. exe_id:{:x}, address:{:x}, file_name:{}, function_name:{}".format(exe_id, address, file_name, function_name))
    self.data[(exe_id, address)] = (file_name, function_name)
    self.__save__()

  def get(self, exe_id, address):
    if self.data.has_key((exe_id, address)):
      return self.data[(exe_id, address)]
    else:
      return None

  def __save__(self):
    with open(self.sim_hash_meta_location, "w+") as fh:
      for key, value in self.data.iteritems():
        fh.write("{exe_id:x} {function_name} {address:x} {file_name}\n".format(
            exe_id = key[0],
            function_name =value[1],
            address = key[1],
            file_name =  base64.b64encode(value[0])
            ))
