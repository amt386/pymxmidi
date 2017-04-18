#include <Python.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

#include "structmember.h"


typedef struct {
    PyObject_HEAD

    int channel;    // unsigned_char
    int type;       // snd_seq_event_type_t
    int value1;
    int value2;

    PyObject *first; /* first name */
    PyObject *last;  /* last name */
    int number;
} MidiEvent;

static void
MidiEvent_dealloc(MidiEvent* self)
{
    Py_XDECREF(self->first);
    Py_XDECREF(self->last);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MidiEvent_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MidiEvent *self;

    self = (MidiEvent *)type->tp_alloc(type, 0);
    if (self != NULL) {

        self->first = PyUnicode_FromString("");
        if (self->first == NULL) {
            Py_DECREF(self);
            return NULL;
        }

        self->last = PyUnicode_FromString("");
        if (self->last == NULL) {
            Py_DECREF(self);
            return NULL;
        }

        self->number = 0;

        self->channel = 0;
        self->type = 0;
        self->value1 = 0;
        self->value2 = 0;
    }

    return (PyObject *)self;
}

static int
MidiEvent_init(MidiEvent *self, PyObject *args, PyObject *kwds)
{
    // PyObject *first=NULL, *last=NULL, *tmp;

    static char *kwlist[] = {
        //"first", "last", "number",
        "channel", "type", "value1", "value2", NULL
    };

    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "iii|i", kwlist,
                //&first, &last, &self->number,
                &self->channel, &self->type, &self->value1,  &self->value2))
        return -1;
    
    /*
    if (first) {
        tmp = self->first;
        Py_INCREF(first);
        self->first = first;
        Py_XDECREF(tmp);
    }

    if (last) {
        tmp = self->last;
        Py_INCREF(last);
        self->last = last;
        Py_XDECREF(tmp);
    }
    */

    return 0;
}


static PyMemberDef MidiEvent_members[] = {
    {"first", T_OBJECT_EX, offsetof(MidiEvent, first), 0,
        "first name"},
    {"last", T_OBJECT_EX, offsetof(MidiEvent, last), 0,
        "last name"},
    {"number", T_INT, offsetof(MidiEvent, number), 0,
        "some number"},
    {"channel", T_INT, offsetof(MidiEvent, channel), 0,
        "MIDI channel"},
    {"type", T_INT, offsetof(MidiEvent, type), 0,
        "Event type"},
    {"value1", T_INT, offsetof(MidiEvent, value1), 0,
        "Value 1"},
    {"value2", T_INT, offsetof(MidiEvent, value2), 0,
        "Value 2"},
    {NULL}  /* Sentinel */
};

static PyObject *
MidiEvent_name(MidiEvent* self)
{
    /*
    if (self->first == NULL) {
        PyErr_SetString(PyExc_AttributeError, "first");
        return NULL;
    }

    if (self->last == NULL) {
        PyErr_SetString(PyExc_AttributeError, "last");
        return NULL;
    }
    return PyUnicode_FromFormat("%S %S", self->first, self->last);
    */
    return PyUnicode_FromFormat("%d %d %d %d", self->channel, self->type,
            self->value1, self->value2);

}

static PyMethodDef MidiEvent_methods[] = {
    {"name", (PyCFunction)MidiEvent_name, METH_NOARGS,
     "Return the name, combining the first and last name"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject MidiEventType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "mxmidi.MidiEvent",             /* tp_name */
    sizeof(MidiEvent),             /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)MidiEvent_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "MidiEvent objects",           /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    MidiEvent_methods,             /* tp_methods */
    MidiEvent_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)MidiEvent_init,      /* tp_init */
    0,                         /* tp_alloc */
    MidiEvent_new,                 /* tp_new */
};


/* ================================================================ */


int valid;
snd_seq_t *seq_handle;
int in_port;
int out_port;
snd_seq_addr_t seq_addr;
snd_seq_event_t ev;

static PyObject *AlsaSequencerError;
// TODO: use this everywhere!

static PyObject *my_callback = NULL;


static void
print_event_info(snd_seq_event_t *ev) {
    //printf("channel %d\n", ev->data.control.channel);
    int ch = ev->data.control.channel;
    switch (ev->type) {
      case SND_SEQ_EVENT_NOTEON:
        printf("Ch %X, NoteOn %5d: %5d\n",
            ch, ev->data.note.note, ev->data.note.velocity);
        break;        
      case SND_SEQ_EVENT_NOTEOFF: 
        printf("Ch %X, NoteOff %5d\n", ch, ev->data.note.note);           
        break;        
      case SND_SEQ_EVENT_CONTROLLER: 
        printf("Ch %X, CC %5d: %5d\n",
                ch, ev->data.control.param, ev->data.control.value);
        break;
      case SND_SEQ_EVENT_PITCHBEND:
        printf("Ch %X: Pitch %5d    \n", ch, ev->data.control.value);
        break;
      case SND_SEQ_EVENT_PGMCHANGE:
        printf("Ch %X: Prog  %5d   \n", ch, ev->data.control.value);
        break;
      case SND_SEQ_EVENT_CHANPRESS:
        printf("Ch %X: ChanPres  %5d   \n", ch, ev->data.control.value);
        break;
    }
}

static PyObject *
build_value_for_event(snd_seq_event_t *ev) {

    int ch = ev->data.control.channel;
    char event_type[30] = "";
    int value1 = -1;
    int value2 = -1;

    switch (ev->type) {
      case SND_SEQ_EVENT_NOTEON:
        strcpy(event_type, "note");
        value1 = ev->data.note.note;
        value2 = ev->data.note.velocity;
        break;        
      case SND_SEQ_EVENT_NOTEOFF: 
        strcpy(event_type, "note");
        value1 = ev->data.note.note;           
        value2 = 0;
        break;        
      case SND_SEQ_EVENT_CONTROLLER: 
        strcpy(event_type, "cc");
        value1 = ev->data.control.param;
        value2 = ev->data.control.value;
        break;
      case SND_SEQ_EVENT_PITCHBEND:
        strcpy(event_type, "bend");
        value1 = ev->data.control.value;
        break;
      case SND_SEQ_EVENT_CHANPRESS:
        strcpy(event_type, "chanpres");
        value1 = ev->data.control.value;
        break;
      case SND_SEQ_EVENT_PGMCHANGE:
        strcpy(event_type, "prog");
        value1 = ev->data.control.value;
        break;
    }
    return Py_BuildValue("isii", ch, event_type, value1, value2);
}


static void *
event_input_thread(void *arg)
{
    snd_seq_event_t *ev;
    PyObject *arglist;
    PyObject *result;
    
    /* Poll descriptors */
    int npfd;
    struct pollfd *pfd;
    npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
    pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
    snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN); 

    while (1) {  /* main loop */
        if (poll(pfd, npfd, 1000000) > 0) {
          do {
            snd_seq_event_input(seq_handle, &ev);

            /* Call the callback */
            if (my_callback) {
                arglist = build_value_for_event(ev);
                result = PyObject_CallObject(my_callback, arglist);
                if (!result) {
                    return(NULL);
                }
                Py_DECREF(arglist);       // TODO: what the hell?
            }
            else {
                // Dump event data to stdout if no handler assigned
                print_event_info(ev);
            }

            snd_seq_free_event(ev);
          } while (snd_seq_event_input_pending(seq_handle, 0) > 0);
        }
    }

}

static PyObject *
mxmidi_wait_for_event(PyObject *self, PyObject *args)
{
    /*  WARNING!
     *  Blocks the entire process anyway (even if wrapped in `threading`).
     *
     */
    snd_seq_event_t *ev;
    PyObject *ret;
    
    /* Poll descriptors */
    // TODO: init polls only once
    int npfd;
    struct pollfd *pfd;
    npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
    pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
    snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN);

    if (snd_seq_event_input_pending(seq_handle, 0) <= 0) {
        if (poll(pfd, npfd, 1000000) <= 0) {
            Py_INCREF(Py_None);
            return Py_None;
        }
    }

    snd_seq_event_input(seq_handle, &ev);
    ret = build_value_for_event(ev);
    
    snd_seq_free_event(ev);
    //Py_DECREF(arglist);       // TODO: what the hell?
    //
    return ret;
}


static PyObject *
mxmidi_set_event_handler(PyObject *self, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *temp;

    if (PyArg_ParseTuple(args, "O:set_callback", &temp)) {
        if (!PyCallable_Check(temp)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            return NULL;
        }
        Py_XINCREF(temp);         /* Add a reference to new callback */
        Py_XDECREF(my_callback);  /* Dispose of previous callback */
        my_callback = temp;       /* Remember new callback */
        /* Boilerplate to return "None" */
        Py_INCREF(Py_None);
        result = Py_None;
    }

    return result;
}


static PyObject *
mxmidi_listen(PyObject *self, PyObject *args)
{
    /* Launch event listening loop in separated thread */
    pthread_t tid;
    pthread_create(&tid, NULL, event_input_thread, NULL);
    //pthread_join(tid,NULL);
    
    //event_input_thread();

    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
mxmidi_open(PyObject *self, PyObject *args)
{
    // TODO: do not open twice
    const char *client_name;
    const char *out_port_name;

    if (!PyArg_ParseTuple(args, "ss", &client_name, &out_port_name))
        return NULL;

    if (snd_seq_open(&seq_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
      PyErr_SetString(AlsaSequencerError, "Error opening ALSA sequencer.");
      return NULL;
    }
    snd_seq_set_client_name(seq_handle, client_name);
    if ((out_port = snd_seq_create_simple_port(seq_handle, out_port_name,
                    SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
                    SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
      PyErr_SetString(PyExc_RuntimeError, "Error creating output port.");
      return NULL;
    }

    /* open one input port */
    if ((in_port = snd_seq_create_simple_port
            (seq_handle, "in",
            SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
            SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
        PyErr_SetString(AlsaSequencerError, "Error creating input port.");
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
    {"set_event_handler",  mxmidi_set_event_handler, METH_VARARGS,
     "Set callback function for processing incoming MIDI events."},
    {"listen",  mxmidi_listen, METH_VARARGS,
     "Listen for incoming events and handle them."},
    {"wait_for_event",  mxmidi_wait_for_event, METH_VARARGS,
     "Wait and return incoming event."},
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
    
    if (PyType_Ready(&MidiEventType) < 0)
        return NULL;

    m = PyModule_Create(&mxmidimodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&MidiEventType);
    PyModule_AddObject(m, "MidiEvent", (PyObject *)&MidiEventType);

    // TODO: INCREFs here?
    PyModule_AddObject(m, "EVENT_NOTE", PyLong_FromLong(SND_SEQ_EVENT_NOTE));
    PyModule_AddObject(m, "EVENT_NOTEON", PyLong_FromLong(SND_SEQ_EVENT_NOTEON));
    PyModule_AddObject(m, "EVENT_NOTEOFF", PyLong_FromLong(SND_SEQ_EVENT_NOTEOFF));
    PyModule_AddObject(m, "EVENT_CONTROLLER", PyLong_FromLong(SND_SEQ_EVENT_CONTROLLER));
    PyModule_AddObject(m, "EVENT_PITCHBEND", PyLong_FromLong(SND_SEQ_EVENT_PITCHBEND));
    PyModule_AddObject(m, "EVENT_CHANPRESS", PyLong_FromLong(SND_SEQ_EVENT_CHANPRESS));
    PyModule_AddObject(m, "EVENT_PGMCHANGE", PyLong_FromLong(SND_SEQ_EVENT_PGMCHANGE));


    AlsaSequencerError = PyErr_NewException("mxmidi.AlsaSequencerError", NULL, NULL);
    Py_INCREF(AlsaSequencerError);
    PyModule_AddObject(m, "AlsaSequencerError", AlsaSequencerError);

    return m;
}
