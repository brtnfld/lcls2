#!/bin/bash

set -e
source setup_env.sh

conda activate ps-0.0.6-py27 
export INSTDIR=`pwd`/install
export PYTHONPATH=$INSTDIR/lib/python2.7/site-packages

mkdir -p $INSTDIR/lib/python2.7/site-packages/
cd psana
python setup.py build_ext --xtcdata=$INSTDIR -f --inplace
python setup.py develop  --xtcdata=$INSTDIR --prefix=$INSTDIR
cd ..

nosetests psana/psana/tests
