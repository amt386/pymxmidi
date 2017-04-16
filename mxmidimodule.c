#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
  
int valid;
snd_seq_t *seq_handle;
int out_port;
snd_seq_addr_t seq_addr;
snd_seq_event_t ev;

static PyObject *AlsaSequencerError;
// TODO: use this everywhere!

static PyObject *
mxmidi_open(PyObject *self, PyObject *args)
{
    // TODO: do not open twice
    const char *client_name;
    const char *port_name;

    if (!PyArg_ParseTuple(args, "ss", &client_name, &port_name))
        return NULL;

    if (snd_seq_open(&seq_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
      PyErr_SetString(AlsaSequencerError, "Error opening ALSA sequencer.");
      return NULL;
    }
    snd_seq_set_client_name(seq_handle, client_name);
    if ((out_port = snd_seq_create_simple_port(seq_handle, port_name,
                    SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
                    SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
      PyErr_SetString(PyExc_RuntimeError, "Error creating sequencer port.");
      return NULL;
    }
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
mxmidi_connect(PyObject *self, PyObject *args)
{
    // TODO: check if opened
    // TODO: wait for client with timeout
    const char *port_name;
    int ret_code;

    if (!PyArg_ParseTuple(args, "s", &port_name))
        return NULL;

    snd_seq_parse_address(seq_handle, &seq_addr, port_name);
    ret_code = snd_seq_connect_to(seq_handle, out_port, seq_addr.client, seq_addr.port);

    if (ret_code) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot connect to the specified port");
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}



static PyObject *
mxmidi_send(PyObject *self, PyObject *args)
{
    int channel;
    const char *event_type;
    int value1;
    int value2;
    
    if (!PyArg_ParseTuple(args, "isii", &channel, &event_type, &value1, &value2))
        return NULL;
    
    // Prepare the event
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_subs(&ev);  
    snd_seq_ev_set_direct(&ev);
    snd_seq_ev_set_source(&ev, out_port);
    
    // TODO: validate
    ev.data.control.channel = channel;

    if (strcmp(event_type, "note") == 0) {
        // TODO: validate ranges, convert note names
        ev.type = (value2) ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
        ev.data.note.note = value1;
        ev.data.note.velocity = value2;
    } else if (strcmp(event_type, "cc") == 0) {
        ev.type = SND_SEQ_EVENT_CONTROLLER;
        ev.data.control.param = value1;
        ev.data.control.value = value2;
    } else if (strcmp(event_type, "pbend") == 0) {
        ev.type = SND_SEQ_EVENT_PITCHBEND;
        ev.data.control.value = value1;
    } else if (strcmp(event_type, "prog") == 0) {
        ev.type = SND_SEQ_EVENT_PGMCHANGE;
        ev.data.control.value = value1;
    } else {
        // TODO: use C string formatting for proper message
        PyErr_SetString(AlsaSequencerError, "Invalid event type.");
        return NULL;
    }

    snd_seq_event_output_direct(seq_handle, &ev);

    Py_INCREF(Py_None);
    return Py_None;

}

static PyObject *
mxmidi_get_client_id(PyObject *self, PyObject *args)
{
    int client_id;
    client_id = snd_seq_client_id(seq_handle);
    return PyLong_FromLong(client_id);
}

static PyObject *
mxmidi_close(PyObject *self, PyObject *args)
{
    // TODO: error if closed already
    snd_seq_close(seq_handle);

    Py_INCREF(Py_None);
    return Py_None;
}

/*
static PyObject *
mxmidi_gagaga(PyObject *self, PyObject *args)
{
    //fprintf(stderr, "\nMissing parameter(s): midisend --bend <ch> <val>\n\n");
    //return Py_BuildValue("s", seq_name);
    //return PyLong_FromLong(4);
}
*/

// TODO: argument help
static PyMethodDef MxmidiMethods[] = {
    {"open",  mxmidi_open, METH_VARARGS,
     "Initialize ALSA seq client."},
    {"send",  mxmidi_send, METH_VARARGS,
     "Send MIDI event."},
    {"get_client_id",  mxmidi_get_client_id, METH_NOARGS,
     "Get client id."},
    {"connect",  mxmidi_connect, METH_VARARGS,
     "Connect to port."},
    {"close",  mxmidi_close, METH_NOARGS,
     "Close ALSA seq client."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef mxmidimodule = {
   PyModuleDef_HEAD_INIT,
   "mxmidi",   /* name of module */
   NULL,     /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   MxmidiMethods
};

PyMODINIT_FUNC
PyInit_mxmidi(void)
{
    PyObject *m;

    m = PyModule_Create(&mxmidimodule);
    if (m == NULL)
        return NULL;

    AlsaSequencerError = PyErr_NewException("mxmidi.AlsaSequencerError", NULL, NULL);
    Py_INCREF(AlsaSequencerError);
    PyModule_AddObject(m, "AlsaSequencerError", AlsaSequencerError);

    return m;
}
