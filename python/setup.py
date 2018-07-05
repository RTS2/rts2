from distutils.core import setup

setup(
    name='RTS2',
    version='0.1',
    packages=['rts2',],
    license='LGPL v2',
    description='RTS2 Python package, to interact with RTS2 system',
    install_requirer='astropy',
    long_description=open('README.txt').read(),
    url='https://github.com/RTS2/rts2',
    author='RTS2 team',
    author_email='rts2@rts2.org'
)
