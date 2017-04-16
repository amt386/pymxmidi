from distutils.core import setup, Extension
 
ext = Extension(
    'mxmidi',
    sources=['mxmidimodule.c'],
    include_dirs=['/usr/include/alsa'],
    libraries=['asound'],
)
 
setup(
    name='mxmidi',
    version='0.1',
    description='Python MIDI wrappers and tools.',
    ext_modules=[ext],
    scripts=['midisend.py']
)
