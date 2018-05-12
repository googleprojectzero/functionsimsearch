#include <Python.h>
#include "structmember.h"
#include "patchlevel.h" // To get version of current Python.

// The Python bindings, all in one file. Please refer to ./pybindings/README.md
// for details about the provided API and more commentary.

#include "disassembly/flowgraphutil.hpp"
#include "disassembly/flowgraphwithinstructions.hpp"
#include "searchbackend/functionsimhash.hpp"
#include "searchbackend/simhashsearchindex.hpp"

static PyObject* functionsimsearch_error;

// Helper function to convert a Python object into a std::string. Used to min-
// imize changes required to build the bindings for Python 2 and Python 3.
bool PyObjectToString(PyObject* object, std::string* result) {
  char* temp = nullptr;
#if PY_MAJOR_VERSION == 2
  if (!PyString_Check(object)) {
    return false;
  }
  temp = PyString_AsString(object);
#elif PY_MAJOR_VERSION == 3
  if (!PyUnicode_Check(object)) {
    return false;
  }
  temp = PyUnicode_AsUTF8(object);
#else
  #error "Unsupported Python version"
#endif 
  if (temp == NULL) {
    return false;
  }
  result->assign(temp);
  return true;
}

// PyFlowgraphWithInstructions - the flowgraph class for direct use from Python.

typedef struct PyFlowgraphWithInstructions {
  PyObject_HEAD
  FlowgraphWithInstructions* flowgraph_with_instructions_;
} PyFlowgraphWithInstructions;

static PyObject* PyFlowgraphWithInstructions_new(PyTypeObject *type,
  PyObject* args, PyObject *kwds) {
  PyFlowgraphWithInstructions* self;
  self = (PyFlowgraphWithInstructions*) type->tp_alloc(type, 0);
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
  return 0;
}

// No externally accessible members.
static PyMemberDef PyFlowgraphWithInstructions_members[] = {
  { NULL },
};

static PyObject* PyFlowgraphWithInstructions__add_node(PyObject* self,
  PyObject* args) {
  uint64_t address;
  if (!PyArg_ParseTuple(args, "L", &address)) {
    PyErr_SetString(functionsimsearch_error, "Failed to parse argument");
    return NULL;
  }

  PyFlowgraphWithInstructions* this_ = (PyFlowgraphWithInstructions*)self;
  this_->flowgraph_with_instructions_->AddNode(address);
  Py_RETURN_NONE;
}

static PyObject* PyFlowgraphWithInstructions__add_edge(PyObject* self,
  PyObject* args) {
  uint64_t source, dest;
  if (!PyArg_ParseTuple(args, "LL", &source, &dest)) {
    PyErr_SetString(functionsimsearch_error, "Failed to parse arguments");
    return NULL;
  }

  PyFlowgraphWithInstructions* this_ = (PyFlowgraphWithInstructions*)self;
  this_->flowgraph_with_instructions_->AddEdge(source, dest);

  Py_RETURN_NONE;
}

static PyObject* PyFlowgraphWithInstructions__to_json(PyObject* self,
  PyObject* args) {
  std::ostringstream json;
  PyFlowgraphWithInstructions* this_ = (PyFlowgraphWithInstructions*)self;

  this_->flowgraph_with_instructions_->WriteJSON(&json,
    FlowgraphWithInstructionInstructionGetter(
      this_->flowgraph_with_instructions_));
  return PyUnicode_FromString(json.str().c_str());
}

static PyObject* PyFlowgraphWithInstructions__add_instructions(PyObject* self,
  PyObject* args) {
  // Extract arguments. The arguments are Tuples-of-Tuples of the form:
  // (
  //   (mnemonic_string, (operand1, ..., operandN)),
  //   ...
  // )
  uint64_t block_address;
  PyObject* block_object = nullptr;
  if (!PyArg_ParseTuple(args, "LO", &block_address, &block_object)) {
    PyErr_SetString(functionsimsearch_error, "Failed to parse argument"); 
    return NULL;
  }
  // Check if we are dealing with a tuple.
  if (!PyTuple_Check(block_object)) {
    PyErr_SetString(functionsimsearch_error, "Second argument is not a tuple?");
    return NULL;
  }
  PyTupleObject* block_tuple = (PyTupleObject*)(block_object);
  std::vector<Instruction> instructions;
  // Now begin a loop to iterate over all elements in the tuple.
  for (uint64_t instruction_index = 0;
    instruction_index < (uint64_t)PyTuple_Size((PyObject*)(block_tuple));
    ++instruction_index) {
    // Get the first instruction tuple.
    PyObject* instruction_object = PyTuple_GetItem((PyObject*)block_tuple,
      instruction_index);
    if (!PyTuple_Check(instruction_object)) {
      PyErr_SetString(functionsimsearch_error,
        "Instruction Object is not a tuple.");
      return NULL;
    }
    PyTupleObject* instruction_tuple =
      (PyTupleObject*)instruction_object;
    // Parse the contents of the tuple. First object is the mnemonic.
    PyObject* mnemonic_object = PyTuple_GetItem((PyObject*) instruction_tuple, 0);
    PyObject* operands_object = PyTuple_GetItem((PyObject*) instruction_tuple, 1);
    if ((mnemonic_object == NULL) || (operands_object == NULL) ||
      !PyTuple_Check(operands_object)) {
      PyErr_SetString(functionsimsearch_error, "Operands are not a tuple.");
      return NULL;
    }
    std::vector<std::string> operands;
    for (uint64_t operand_index = 0; operand_index <
      (uint64_t)PyTuple_Size(operands_object); ++operand_index) {
      PyObject* element = PyTuple_GetItem(operands_object, operand_index);
      if (element == NULL) {
        return NULL;
      }
      std::string operand;
      if (!PyObjectToString(element, &operand)) {
        printf("Failed to convert op to string");
        return NULL;
      }
      operands.push_back(operand);
    }
    std::string mnemonic;
    if (!PyObjectToString(mnemonic_object, &mnemonic)) {
      return NULL;
    }
    instructions.emplace_back(mnemonic, operands);
  }
  // All instructions have been parsed. Add them to the corresponding block.
  PyFlowgraphWithInstructions* this_ = (PyFlowgraphWithInstructions*)self;
  this_->flowgraph_with_instructions_->AddInstructions(block_address,
    instructions);

  Py_RETURN_NONE;
}

static PyMethodDef PyFlowgraphWithInstructions_methods[] = {
  { "add_node", (PyCFunction)PyFlowgraphWithInstructions__add_node,
    METH_VARARGS, NULL },
  { "add_edge", (PyCFunction)PyFlowgraphWithInstructions__add_edge,
    METH_VARARGS, NULL },
  { "add_instructions",
    (PyCFunction)PyFlowgraphWithInstructions__add_instructions, METH_VARARGS,
    NULL },
  { "to_json", (PyCFunction)PyFlowgraphWithInstructions__to_json,
    METH_VARARGS, NULL },
  { NULL, NULL, 0, NULL },
};

static PyTypeObject PyFlowgraphWithInstructionsType = []() -> PyTypeObject  {
  PyTypeObject type = {PyObject_HEAD_INIT(NULL) 0};
  type.tp_name = "functionsimsearch.FlowgraphWithInstructions";
  type.tp_basicsize = sizeof(PyFlowgraphWithInstructions);
  type.tp_dealloc = (destructor)PyFlowgraphWithInstructions_dealloc;
  type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  type.tp_doc = "A simple CFG-with-instructions class for calculating "
    "similarity-preserving hashes from code.";
  type.tp_methods = PyFlowgraphWithInstructions_methods;
  type.tp_members = PyFlowgraphWithInstructions_members;
  type.tp_init = (initproc)PyFlowgraphWithInstructions_init;
  type.tp_new = &PyFlowgraphWithInstructions_new;
  return type;
}();

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

static int PySimHasher_init(PySimHasher* self, PyObject* args, PyObject *kwds) {
  // No arguments, so no need to parse any.
  self->function_simhasher_ = new FunctionSimHasher("weights.txt");
  return 0;
}

static PyObject* PySimHasher__calculate_hash(PyObject* self,
  PyObject* args) {
  PyFlowgraphWithInstructions* flowgraph;
  if (!PyArg_ParseTuple(args, "O!", &PyFlowgraphWithInstructionsType,
    &flowgraph)) {
    return NULL;
  }
  FlowgraphWithInstructionsFeatureGenerator feature_gen(
    *(flowgraph->flowgraph_with_instructions_));
  std::vector<uint64_t> output_hashes;
  PySimHasher* this_ = (PySimHasher*)self;
  this_->function_simhasher_->CalculateFunctionSimHash(&feature_gen, 2,
    &output_hashes);

  PyObject* return_tuple = PyTuple_New(2);
  printf("%lx - %lx\n", output_hashes[0], output_hashes[1]);
  PyTuple_SetItem(return_tuple, 0, PyLong_FromLong(output_hashes[0]));
  PyTuple_SetItem(return_tuple, 1, PyLong_FromLong(output_hashes[1]));

  return return_tuple;
}

// No externally accessible members.
static PyMemberDef PySimHasher_members[] = {
  { NULL },
};

static PyMethodDef PySimHasher_methods[] = {
  { "calculate_hash", (PyCFunction)PySimHasher__calculate_hash, METH_VARARGS, NULL },
  { NULL, NULL, 0, NULL },
};

// Initializing a PyTypeObject is painful in C++ - in C99 one could use
// designated initializer lists, but those are explicitly not adopted by C++.
// We work around this by the following messy construct: Initializing them by
// lambdas to be executed on program startup.

static PyTypeObject PySimHasherType = []() -> PyTypeObject  {
  PyTypeObject type = {PyObject_HEAD_INIT(NULL) 0};
  type.tp_name = "functionsimsearch.SimHasher";
  type.tp_basicsize = sizeof(PySimHasher);
  type.tp_dealloc = (destructor)PySimHasher_dealloc;
  type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  type.tp_doc = "SimHasher calculates a similarity-preserving hash for a CFG.";
  type.tp_methods = PySimHasher_methods;
  type.tp_members = PySimHasher_members;
  type.tp_init = (initproc)PySimHasher_init;
  type.tp_new = &PySimHasher_new;
  return type;
}();

// PySimHashSearchIndex stuff.
typedef struct PySimHashSearchIndex {
  PyObject_HEAD
  SimHashSearchIndex* search_index_;
} PySimHashSearchIndex;

static PyObject* PySimHashSearchIndex_new(PyTypeObject *type, PyObject* args, 
  PyObject* kwds) {
  PySimHashSearchIndex* self;
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
  self->search_index_ = new SimHashSearchIndex("search.index", false);
  return 0;
}

static PyObject* PySimHashSearchIndex__get_free_size(PyObject* self,
  PyObject* args) {
  PySimHashSearchIndex* index = (PySimHashSearchIndex*)self;
  return PyLong_FromLong(index->search_index_->GetIndexFileFreeSpace());
}

static PyObject* PySimHashSearchIndex__get_used_size(PyObject* self,
  PyObject* args) {
  PySimHashSearchIndex* index = (PySimHashSearchIndex*)self;
  return PyLong_FromLong(index->search_index_->GetIndexSetSize());
}

static PyObject* PySimHashSearchIndex__add_function(PyObject* self,
  PyObject* args) {
  // Arguments are hash_A, hash_B (both uint64_t), a 128-bit FileID, and an
  // address of the function in question.
  uint64_t hash_A, hash_B, file_id, address;
  if (!PyArg_ParseTuple(args, "kkkkk", &hash_A, &hash_B, &file_id, &address)) {
    return NULL;
  }
  PySimHashSearchIndex* index = (PySimHashSearchIndex*)self;
  index->search_index_->AddFunction(hash_A, hash_B, file_id, address);
  Py_RETURN_NONE;
}

static PyObject* PySimHashSearchIndex__query_top_N(PyObject* self, 
  PyObject* args) {
  uint32_t how_many;
  uint64_t hash_A, hash_B;
  std::vector<std::pair<float, SimHashSearchIndex::FileAndAddress>> results;
  if (!PyArg_ParseTuple(args, "kkI", &hash_A, &hash_B, &how_many)) {
    return NULL;
  }
  PySimHashSearchIndex* index = (PySimHashSearchIndex*)self;
  index->search_index_->QueryTopN(hash_A, hash_B, how_many, &results);
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

// No externally accessible members.
static PyMemberDef PySimHashSearchIndex_members[] = {
  { NULL },
};

static PyMethodDef PySimHashSearchIndex_methods[] = {
  { "get_free_size", (PyCFunction)PySimHashSearchIndex__get_free_size, METH_VARARGS, NULL },
  { "get_used_size", (PyCFunction)PySimHashSearchIndex__get_used_size, METH_VARARGS, NULL },
  { "add_function", (PyCFunction)PySimHashSearchIndex__add_function, METH_VARARGS, NULL },
  { "query_top_N", (PyCFunction)PySimHashSearchIndex__query_top_N, METH_VARARGS, NULL },
  { NULL, NULL, 0, NULL },
};

static PyTypeObject PySimHashSearchIndexType = []() -> PyTypeObject  {
  PyTypeObject type = {PyObject_HEAD_INIT(NULL) 0};
  type.tp_name = "functionsimsearch.SimHashSearchIndex";
  type.tp_basicsize = sizeof(PySimHashSearchIndex);
  type.tp_dealloc = (destructor)PySimHashSearchIndex_dealloc;
  type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  type.tp_doc = "SimHashSearchIndex allows ANN search for similarity-preserving hashes.";
  type.tp_methods = PySimHashSearchIndex_methods;
  type.tp_members = PySimHashSearchIndex_members;
  type.tp_init = (initproc)PySimHashSearchIndex_init;
  type.tp_new = &PySimHashSearchIndex_new;
  return type;
}();

static PyMethodDef functionsimsearch_methods[] = {
  { NULL, NULL, NULL}
};

PyMODINIT_FUNC
initfunctionsimsearch(void) {
  PyObject *module;

  if (PyType_Ready(&PyFlowgraphWithInstructionsType) ||
    PyType_Ready(&PySimHasherType) ||
    PyType_Ready(&PySimHashSearchIndexType)) {
    return;
  }

  module = Py_InitModule3("functionsimsearch", functionsimsearch_methods,
    "FunctionSimSearch Python Bindings");
  if (module == NULL) {
    return;
  }

  functionsimsearch_error = PyErr_NewException((char*)"functionsimsearch.error",
    NULL, NULL);
  Py_INCREF(functionsimsearch_error);
  PyModule_AddObject(module, "error", functionsimsearch_error);

  Py_INCREF(&PyFlowgraphWithInstructionsType);
  PyModule_AddObject(module, "FlowgraphWithInstructions",
    (PyObject*)&PyFlowgraphWithInstructionsType);

  Py_INCREF(&PySimHasherType);
  PyModule_AddObject(module, "SimHasher", (PyObject*)&PySimHasherType);

  Py_INCREF(&PySimHashSearchIndexType);
  PyModule_AddObject(module, "SimHashSearchIndex",
    (PyObject*)&PySimHashSearchIndexType);
}
