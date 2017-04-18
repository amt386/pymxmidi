#!/usr/bin/env python3
"""
Example usage:
    mididump

"""
import argparse
import signal
import time
import threading

import mxmidi
from mxmidi import MidiEvent

# Should be imported BEFORE starting threads? # TODO: is it true?
try:
    import IPython
    if int(IPython.__version__.split('.')[0]) >= 5:
        ipython_kwargs = dict(banner1='', confirm_exit=False)
    else:
        from traitlets.config.loader import Config
        cfg = Config()
        cfg.TerminalInteractiveShell.confirm_exit = False
        prompt_config = cfg.PromptManager
        prompt_config.in_template = 'In <\\#>: '
        prompt_config.in2_template = '   .\\D.: '
        prompt_config.out_template = 'Out<\\#>: '
        ipython_kwargs = dict(config=cfg, banner1='')
except ImportError:
    pass


parser = argparse.ArgumentParser(
    description='Dump incoming  MIDI events to stdout.'
)

 
parser.add_argument('-n', '--client-name', type=str, dest='client_name', default='mididump',
        help='ALSA sequencer client name')
parser.add_argument('-p', '--port-name', type=str, dest='port_name', default='out_0',
        help='ALSA sequencer port name')

args = parser.parse_args()


def handler(signum, frame):
    print('\nTerminating on signal ', signum)
    exit(0)
    #raise KeyboardInterrupt
    #print('Ctrl+Z pressed, but ignored')
signal.signal(signal.SIGTSTP, handler)  # Ctrl+C
signal.signal(signal.SIGINT, handler)   # Ctrl+D


try:
    IPython.embed(**ipython_kwargs)
except:
    pass

    
try:
    mxmidi.open(args.client_name, args.port_name)
    mxmidi.connect('mxmd:0')
except mxmidi.AlsaSequencerError as e:
    print(str(e))
    exit(1)


def thread_beat():
    while True:
        mxmidi.send(9, 'note', 36, 32)
        time.sleep(2)
        mxmidi.send(9, 'note', 36, 0)
t = threading.Thread(target=thread_beat)
t.start()

'''
def wait_loop():
    while True:
        data = mxmidi.wait_for_event()
        print('Event: ', data)
'''


def event_handler(*args, **kwargs):
    print('args: ', args, 'kwargs: ', kwargs)
    if args[1] == 'cc' and args[2] == 123:   # Panic CC
        raise KeyboardInterrupt
    return True
mxmidi.set_event_handler(event_handler)
mxmidi.listen()


while True:
    pass

