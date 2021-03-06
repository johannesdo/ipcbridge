### hack to avoid hardlinking through python setup.py sdist - doesn't work on vboxfs
import os
del os.link
###

from distutils.core import setup, Extension

#
# https://docs.python.org/3.3/extending/building.html#building
#

module1 = Extension('ipcbridge',
                    define_macros = [('MAJOR_VERSION', '0'),
                                     ('MINOR_VERSION', '0.5')],
                    include_dirs = ['/usr/include'],
                    libraries = ['pthread'],
                    library_dirs = ['/usr/lib'],
                    sources = ['ipcbridge.c'])

setup(  name = 'IpcBridge',
        version = '0.0.5',
        description = 'python IPC bridge module',
        author = 'Johannes Doerk',
        author_email = 'johannes.doerk@gmx.de',
        url = 'https://github.com/johannesdo/ipcbridge',
        long_description = '''
python IPC bridge module.
''',
        ext_modules = [module1])
