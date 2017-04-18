#!/usr/bin/env python3
"""
Example usage:
    midisend -t mxmd:0 --off n:C5 n:64

"""
import argparse
import time

import mxmidi

parser = argparse.ArgumentParser(
    description='Send MIDI events using ALSA sequencer API.'
)

NOTE_VALUES = [
    'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'
]

def _get_note_value_by_name(name):
    pitch_cls = name[0].upper()
    if name[1] == '#':
        name = name[2:]
        pitch_cls += '#'
    else:
        name = name[1:]
    try:
        pitch_cls_number = NOTE_VALUES.index(pitch_cls)
    except ValueError:
        raise argparse.ArgumentTypeError('Invalid note name: %s' % pitch_cls)
    octave = int(name)
    return octave * 12 + pitch_cls_number

def event_data(string):

    tokens = string.split(':')

    type_token = tokens.pop(0).lower()

    event_type = None       # TODO: use C constants for event type
    value1 = 0
    value2 = 0

    if type_token in ('note', 'n'):
        event_type = 'note'

        try:

            token = tokens.pop(0)

            try:
                value1 = int(token)
            except ValueError:
                value1 = _get_note_value_by_name(token)
            if value1 < 0 or value1 > 127:
                raise argparse.ArgumentTypeError('Invalid note: %d' % value1)

        except IndexError:
            raise argparse.ArgumentTypeError('Too few tokens for Note event')

        try:
            value2 = int(tokens.pop(0))
        except IndexError:
            # TODO: change default velocity?
            value2 = 64

        # TODO: validate velocity

        if len(tokens):
            raise argparse.ArgumentTypeError('Too more tokens for Note event')

        # TODO: validate
    elif type_token in ('cc', 'c'):
        if len(tokens) != 2:
            raise argparse.ArgumentTypeError('CC event 3 tokens')
        # TODO: validate
        event_type = 'cc'
        value1 = int(tokens[0])
        value2 = int(tokens[1])
    elif type_token in ('prog', 'p'):
        if len(tokens) != 1:
            raise argparse.ArgumentTypeError('Prog event 2 tokens')
        # TODO: validate
        event_type = 'prog'
        value1 = int(tokens[0])
    elif type_token in ('bend', 'b'):
        if len(tokens) != 1:
            raise argparse.ArgumentTypeError('Bend event 2 tokens')
        # TODO: validate
        event_type = 'bend'
        value1 = int(tokens[0])
    else:
        raise argparse.ArgumentTypeError('Unknown event type: %s.' % type_token)

    return event_type, value1, value2

 
parser.add_argument('-n', '--client-name', type=str, dest='client_name', default='midisend.py',
        help='ALSA sequencer client name')
parser.add_argument('-p', '--port-name', type=str, dest='port_name', default='out_0',
        help='ALSA sequencer port name')

# TODO: wait for specified client (with timeout)
parser.add_argument('-t', '--target', type=str, dest='target_port', default=None,
        help='ALSA sequencer port to connect to (eg 128:0)')

#TODO: validate
parser.add_argument('-c', '--channels', dest='channels', type=int, nargs='+',
    default=[0], metavar='N', help='Channel number (one or many)')

parser.add_argument('--off', dest='auto_off', action='store_true',
        default=False,
        help='Auto send note-off for all played notes in the end.')

# TODO: do not allow looping if delay is zero
parser.add_argument('--loop', dest='loop', action='store_true',
        default=False,
        help='Loop event sequence.')

parser.add_argument('-d', '--delay', type=float, dest='delay', default=0.0,
        help='Delay between notes (in seconds, e.g. "-d 0.5").')
parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
        default=False,
        help='Print information to stdout.')

# TODO: help with format
parser.add_argument('messages', metavar='messages', type=event_data, nargs='+',
                    help='List of MIDI messages')


args = parser.parse_args()


# TODO: handle Ctrl+Z

if args.verbose:
    print('CHANNELS: ', args.channels)
    print('AUTO-OFF: ', args.auto_off)
    print('LOOP: ', args.loop)
    print('------------------')
    
try:
    mxmidi.open(args.client_name, args.port_name)
except mxmidi.AlsaSequencerError as e:
    print(str(e))
    exit(1)

if args.target_port:
    mxmidi.connect(args.target_port)

auto_off_notes = set()

repeat = True

def _send_event(ch, event_data):
    mxmidi.send(ch, event_data[0], event_data[1], event_data[2])
    if args.verbose:
        print(ch, event_data)

try:
    while repeat:
        for event_data in args.messages:

            for ch in args.channels:
                _send_event(ch, event_data)

            if event_data[0] == 'note':
                if event_data[2] > 0:
                    auto_off_notes.add(event_data[1])
                else:
                    auto_off_notes.discard(event_data[1])

            if args.delay:
                time.sleep(args.delay)

        repeat = args.loop
finally:
    # sending this even after KeyboardInterrupt
    if args.auto_off:
        for note in auto_off_notes:
            for ch in args.channels:
                _send_event(ch, ('note', note, 0))
