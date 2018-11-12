# author: Michal Melewski <carstein@google.com>
# FSS similarity search plugin - metadata class

import base64
import os

import binaryninja as bn


class Metadata:
  def __init__(self, meta_location):
    self.data = self.read_metadata_dict(meta_location)
    self.sim_hash_meta_location = meta_location

  def read_metadata_dict(self, meta_location):
    data = {}
    if os.path.isfile(meta_location):
      with open(meta_location, "r") as fh:
        for line in fh.xreadlines():
          tokens = line.split()
          if tokens:
            try:
              decoded_token = base64.b64decode(tokens[3])
            except:
              bn.log_info("[-] Failure to b64decode '%s'!" % tokens[3])
              decoded_token = tokens[3]
            exe_id = long(tokens[0], 16)
            address = long(tokens[2], 16)
            file_name = tokens[1]
            function_name = decoded_token
            data[(exe_id, address)] = (file_name, function_name)
    bn.log_info("[+] Loaded metadata for %d functions." % len(data.keys()))
    return data

  def add(self, exe_id, address, file_name, function_name):
    bn.log_info("[*] Adding metadata. exe_id:{:x}, address:{:x}, file_name:{}, function_name:{}".format(exe_id, address, file_name, function_name))
    self.data[(exe_id, address)] = (file_name, function_name)

  def get(self, exe_id, address):
    if self.data.has_key((exe_id, address)):
      return self.data[(exe_id, address)]
    else:
      return None

  def __save__(self):
    """
    When saving, we should be conservative -- we do not want to delete existing
    metadata, instead we want to add any extra metadata we have collected.
    """
    exists = os.path.isfile(self.sim_hash_meta_location)
    if exists:
      data = self.read_metadata_dict(self.sim_hash_meta_location)
    else:
      data = {}
    for key, value in self.data.iteritems():
      data[key] = value
    # Now write the file again.
    with open(self.sim_hash_meta_location, "w+") as fh:
      for key, value in data.iteritems():
        try:
          fh.write("{exe_id:x} {file_name} {address:x} {function_name} false\n"
            .format(
              exe_id = key[0],
              file_name = value[0],
              address = key[1],
              function_name =  base64.b64encode(value[1])
              ))
        except:
          # Keep on writing even if there was an exception.
          pass

