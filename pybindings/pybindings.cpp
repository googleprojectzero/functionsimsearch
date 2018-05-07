#include <Python.h>

// The Python bindings, all in one file. Please refer to ./pybindings/README.md
// for details about the provided API and more commentary.

#include "disassembly/flowgraphwithinstructions.hpp"


// PyFlowgraphWithInstructions - the flowgraph class for direct use from Python.

typedef struct PyFlowgraphWithInstructions {
  PyObject_HEAD
  FlowgraphWithInstructions* flowgraph_with_instructions_;
} PyFlowgraphWithInstructions;

static PyObject* PyFlowgraphWithInstructions_new(PyTypeObject *type,
  PyObject* args, PyObject *kwds) {
  PyFlowgraphWithInstructions* self;
  self = (PyFllowgraphWithInstructions*) type->tp_alloc(type, 0);
  return (PyObject*) self;
}

static void PyFlowgraphWithInstructions_dealloc(
  PyFlowgraphWithInstructions* self) {
  delete self->flowgraph_with_instructions_;
  self->ob_type->tp_free((PyObject*)self);
}

static int PyFlowgraphWithInstructions_init(PyFlowgraphWithInstructions* self,
  PyObject* args) {
  // No arguments, so no need to parse any.
  self->flowgraph_with_instructions_ = new FlowgraphWithInstructions();
}

// No externally accessible members.
static PyMemberDef PyFlowgraphWithInstructions_members[] = {
  { NULL },
};

static PyObject* PyFlowgraphWithInstructions__add_node(PyObject* self,
  PyObject* args) {
  uint64_t address;
  if (!PyArg_ParseTuple(args, "L", &address)) {
    return NULL;
  }

  PyFlowgraphWithInstructions* this_ = (PyFlowgrapHWithInstructions*)self;
  this_->flowgraph_with_instructions_->AddNode(address);
  Py_RETURN_NONE;
}

static PyObject* PyFlowgraphWithInstructions__add_edge(PyObject* self,
  PyObject* args) {
  uint64_t source, dest;
  if (!PyArg_ParseTuple(args, "LL", &source, &dest)) {
    return NULL;
  }

  PyFlowgraphWithInstructions* this_ = (PyFlowgrapHWithInstructions*)self;
  this_->flowgraph_with_instructions_->AddEdge(source, dest);

  Py_RETURN_NONE;
}

static PyObject* PyFlowgraphWithInstructions__add_instructions(PyObject* self,
  PyObject* args) {
  // Extract arguments. The arguments are Tuples-of-Tuples of the form:
  // (
  //   (mnemonic_string, (operand1, ..., operandN)),
  //   ...
  // )
  uint64_t address;
  PyObject* block_object = nullptr;
  if (!PyArg_ParseTuple(args, "LO", &address, &block_object)) {
    return NULL;
  }
  // Check if we are dealing with a tuple.
  if (!PyTuple_Check(block_object)) {
    return NULL;
  }
  PyTupleObject* block_tuple = static_cast<PyTupleObject*>(block_object);
  std::vector<Instruction> instructions;
  // Now begin a loop to iterate over all elements in the tuple.
  for (uint64_t instruction_index = 0;
    instruction_index < PyTuple_Size(block_tuple); ++instruction_index) {
    // Get the first instruction tuple.
    PyObject* instruction_object = PyTuple_GetItem(block_tuple, instruction_index);
    if (!PyTuple_Check(instruction_object)) {
      return NULL;
    }
    PyTupleObject* instruction_tuple =
      static_cast<PyTupleObject*>(instruction_object);
    // Parse the contents of the tuple. First object is the mnemonic.
    PyObject* mnenomic_object = PyTuple_GetItem(instruction_tuple, 0);
    PyObject* operands_object = PyTuple_GetItem(instruction_tuple, 1);
    if ((mnemonic_object == NULL) || (operands_object == NULL) || 
      !PyTuple_Check(operands_object)) {
      return NULL;
    }
    PyTupleObject* operands_typle = static_cast<PyTupleObject*>(operands_object);
    std::vector<std::string> operands;
    for (uint64_t operand_index = 0; operand_index < PyTuple_Size(operands_tuple);
      ++ operand_index) {
      PyObject* element = PyTuple_GetItem(operands_tuple, operand_index);
      if (element == NULL) {
        return NULL;
      }
      std::string operand;
      if (!PyObjectToString(element, &operand)) {
        return NULL;
      }
      operands.push_back(operand);
    }
    std::string mnemonic;
    if (!PyObjectToString(mnemonic_object, &mnemonic)) {
      return NULL;
    }
    Instructions.emplace_back(mnemonic, operands);
  }
  // All instructions have been parsed. Add them to the corresponding block.
  this_->flowgraph_with_instructions_->AddInstructions(address, &instructions);
  Py_RETURN_NONE;
}

static PyMethodDef PyFlowgraphWithInstructions_methods[] = {
  { "add_node", (PyCFunction)PyFlowgraphWithInstructions__add_node, METH_VARARGS, NULL },
  { "add_edge", (PyCFunction)PyFlowgraphWithInstructions__add_edge, METH_VARARGS, NULL },
  { "add_instructions", (PyCFunction)PyFlowgraphWithInstructions__add_node, METH_VARARGS, NULL },
  { NULL, NULL, 0, NULL },
};

static PyTypeObject PyFlowgraphWithInstructionsType = {
  PyObject_HEAD_INIT(NULL)
  0, /* ob_size */
  "functionsimsearch.FlowgraphWithInstructions",
  sizeof(PyFlowgraphWithInstructions),
  0, /* tp_itemsize */
  (destructor)PyFlowgraphWithInstructions_dealloc,
  0, /* tp_print */
  0, /* tp_getattr */
  0, /* tp_setattr */
  0, /* tp_compare */
  PyFlowgraphWithInstructionsType_repr, /* tp_repr */
  0, /* tp_as_number */
  0, /* tp_as_sequence */
  0, /* tp_as_mapping */
  0, /* tp_hash */
  0, /* tp_call */
  0, /* tp_str */
  0, /* tp_getattro */
  0, /* tp_setattro */
  0, /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
  "FlowgraphWithInstructions is a simple CFG with a sequence of strings associated to each node", /* tp_doc */
  0, /* tp_traverse */
  0, /* tp_clear */
  0, /* tp_richcompare */
  0, /* tp_weaklistoffset */
  0, /* tp_iter */
  0, /* tp_iternext */
  PyFlowgraphWithInstructions_methods, /* tp_methods */
  PyFlowgraphWithInstructions_members, /* tp_members */
  0, /* tp_getset */
  0, /* tp_base */
  0, /* tp_dict */
  0, /* tp_descr_get */
  0, /* tp_descr_set */
  0, /* tp_dictoffset */
  (initproc)PyFlowgraphWithInstructions_init,/* tp_init */
  0, /* tp_alloc */
  PyFlowgraphWithInstructions_new, /* tp_new */
};


// PySimHasher stuff.
//

typedef struct PySimHasher {
  PyObject_HEAD
  FunctionSimHasher* function_simhasher_;
} PySimHasher;

static PyObject* PySimHasher_new(PyTypeObject *type, PyObject* args, PyObject* kwds) {
  PySimHasher* self;
  self = (PySimHasher*) type->tp_alloc(type, 0);
  return (PyObject*) self;
}

static void PySimHasher_dealloc(PySimHasher* self) {
  delete self->function_simhasher_;
  self->ob_type->tp_free((PyObject*)self);
}

static int PySimHasher_init(PyFlowgraphWithInstructions* self,
  PyObject* args) {
  // No arguments, so no need to parse any.
  self->function_simhasher_ = new FunctionSimHasher("weights.txt");
  return 0;
}

// No externally accessible members.
static PyMemberDef PySimHasher_members[] = {
  { NULL },
};

static PyMethodDef PySimHasher_methods[] = {
  { "calculate_hash", (PyCFunction)PySimHasher__calculate_hash, METH_VARARGS, NULL },
  { NULL, NULL, 0, NULL },
};

static PyObject* PySimHasher__calculate_hash(PyObject* self,
  PyObject* args) {
  PyFlowgraphWithInstructionsObject* flowgraph;
  if (!PyArg_ParseTuple(args, "O!", &PyFlowgraphWithInstructionsType,
    &flowgraph)) {
    return NULL;
  }
  FlowgraphWithInstructionsFeatureGenerator feature_gen(
    *(flowgraph->flowgraph_with_instructions_));
  std::vector<uint64_t> output_hashes;
  CalculateFunctionSimHash(&feature_gen, 2, &output_hashes);

  Py_RETURN_NONE;
}

static PyTypeObject PySimHasherType = {
  PyObject_HEAD_INIT(NULL)
  0, /* ob_size */
  "functionsimsearch.SimHasher",
  sizeof(PySimHasher),
  0, /* tp_itemsize */
  (destructor)PySimHasher_dealloc,
  0, /* tp_print */
  0, /* tp_getattr */
  0, /* tp_setattr */
  0, /* tp_compare */
  PySimHasher_repr, /* tp_repr */
  0, /* tp_as_number */
  0, /* tp_as_sequence */
  0, /* tp_as_mapping */
  0, /* tp_hash */
  0, /* tp_call */
  0, /* tp_str */
  0, /* tp_getattro */
  0, /* tp_setattro */
  0, /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
  "SimHasher calculates a similarity-preserving hash for a CFG.", /* tp_doc */
  0, /* tp_traverse */
  0, /* tp_clear */
  0, /* tp_richcompare */
  0, /* tp_weaklistoffset */
  0, /* tp_iter */
  0, /* tp_iternext */
  PySimHasher_methods, /* tp_methods */
  PySimHasher_members, /* tp_members */
  0, /* tp_getset */
  0, /* tp_base */
  0, /* tp_dict */
  0, /* tp_descr_get */
  0, /* tp_descr_set */
  0, /* tp_dictoffset */
  (initproc)PySimHasher_init,/* tp_init */
  0, /* tp_alloc */
  PySimHasher_new, /* tp_new */
};

// PySimHashSearchIndex stuff.
typedef struct PySimHashSearchIndex {
  PyObject_HEAD
  SimHashSearchIndex* search_index_;
} PySimHashSearchIndex;

static PyObject* PySimHashSearchIndex_new(PyTypeObject *type, PyObject* args, 
  PyObject* kwds) {
  PySimHasgSearchIndex* self;
  self = (PySimHashSearchIndex*) type->tp_alloc(type, 0);
  return (PyObject*) self;
}

static void PySimHashSearchIndex_dealloc(PySimHashSearchIndex* self) {
  delete self->search_index_;
  self->ob_type->tp_free((PyObject*)self);
}

static int PySimHashSearchIndex_init(PySimHashSearchIndex* self,
  PyObject* args) {
  // No arguments, so no need to parse any.
  self->search_index_ = new PySimHashSearchIndex("search.index", false);
  return 0;
}

// No externally accessible members.
static PyMemberDef PySimHasher_members[] = {
  { NULL },
};

static PyMethodDef PySimHasher_methods[] = {
  { "get_free_size", (PyCFunction)PySimHashSearchIndex__get_free_size, METH_VARARGS, NULL },
  { "get_used_size", (PyCFunction)PySimHashSearchIndex__get_used_size, METH_VARARGS, NULL },
  { "add_function", (PyCFunction)PySimHashSearchIndex__add_function, METH_VARARGS, NULL },
  { "query_top_N", (PyCFunction)PySimHashSearchIndex__query_top_N, METH_VARARGS, NULL },
  { NULL, NULL, 0, NULL },
};

static PyObject* PySimHashSearchIndex__get_free_size(PyObject* self,
  PyObject* args) {
  PySimHashSearchIndex* index = static_cast<PySimHashSearchIndex*>(self);
  return PyLong_FromLong(index->search_index_->GetIndexFileFreeSpace());
}

static PyObject* PySimHashSearchIndex__get_used_size(PyObject* self,
  PyObject* args) {
  PySimHashSearchIndex* index = static_cast<PySimHashSearchIndex*>(self);
  return PyLong_FromLong(index->search_index_->GetIndexSetSize());
}

static PyObject* PySimHashSearchIndex__add_function(PyObject* self,
  PyObject* args) {
  // Arguments are hash_A, hash_B (both uint64_t), a 128-bit FileID, and an
  // address of the function in question.
  uint64_t hash_A, hash_B, file_id_A, address;
  if (!PyArg_ParseTuple(args, "kkkkk", &hash_A, &hash_B, &file_id, &address)) {
    return NULL;
  }
  PySimHashSearchIndex* index = static_cast<PySimHashSearchIndex*>(self);
  index->search_index_->AddFunction(hash_A, hash_B, file_id, address);
  return Py_RETURN_NONE;
}

static PyObject* PySimHashSearchIndex__query_top_N(PyObject* self, 
  PyObject* args) {
  uint32_t how_many;
  uint64_t hash_A, hash_B;
  std::vector<std::Pair<float, FileAndAddress>> results;
  if (!PyArg_ParseTuple(args, "kkI", &hash_A, &hash_B, &how_many)) {
    return NULL;
  }
  PySimHashSearchIndex* index = static_cast<PySimHashSearchIndex*>(self);
  uint64_t findings = index->search_index_->QueryTopN(hash_A, hash_B, how_many,
    &results);
  // Convert the vector to a list of python floats & strings.
  PyObject* result_list = PyList_New(results.size());
  if (!result_list) {
    return NULL;
  }

  for (uint32_t index = 0; index < results.size(); ++index) {
    // Similarity score, file id, address.
    PyObject* result_tuple = PyTuple_New(3);
    if (!result_tuple) {
      Py_DECREF(result_list);
      return NULL;
    }
    // Place it into the list. This steals the reference, destruction of the
    // list will now trigger desctruction of the tuple.
    PyList_SetItem(result_list, index, result_tuple);
    // Set the elements of the tuple.
    PyTuple_SetItem(result_tuple, 0, 
      PyFloat_FromDouble(results[index].first)); // Similarity.
    PyTuple_SetItem(result_tuple, 1,
      PyLong_FromUnsignedLong(results[index].second.first)); // FileID.
    PyTuple_SetItem(result_tuple, 2, 
      PyLong_FromUnsignedLong(results[index].second.second)); // Address.
  }
  return result_list;
}

static PyTypeObject PySimHashSearchIndex = {
  PyObject_HEAD_INIT(NULL)
  0, /* ob_size */
  "functionsimsearch.SimHashSearchIndex",
  sizeof(PySimHashSearchIndex),
  0, /* tp_itemsize */
  (destructor)PySimHashSearchIndex_dealloc,
  0, /* tp_print */
  0, /* tp_getattr */
  0, /* tp_setattr */
  0, /* tp_compare */
  PySimHashSearchIndex_repr, /* tp_repr */
  0, /* tp_as_number */
  0, /* tp_as_sequence */
  0, /* tp_as_mapping */
  0, /* tp_hash */
  0, /* tp_call */
  0, /* tp_str */
  0, /* tp_getattro */
  0, /* tp_setattro */
  0, /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
  "SimHashSearchIndex allows ANN search for similarity-preserving hashes.", /* tp_doc */
  0, /* tp_traverse */
  0, /* tp_clear */
  0, /* tp_richcompare */
  0, /* tp_weaklistoffset */
  0, /* tp_iter */
  0, /* tp_iternext */
  PySimHashSearchIndex_methods, /* tp_methods */
  PySimHashSearchIndex_members, /* tp_members */
  0, /* tp_getset */
  0, /* tp_base */
  0, /* tp_dict */
  0, /* tp_descr_get */
  0, /* tp_descr_set */
  0, /* tp_dictoffset */
  (initproc)PySimHashSearchIndex_init,/* tp_init */
  0, /* tp_alloc */
  PySimHashSearchIndex_new, /* tp_new */
};


