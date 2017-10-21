#!/usr/bin/env python
#

import sys, os
import time
import getopt
import pprint

import inspect;
import gc

import pydgram
#sys.path.append('../build/xtcdata')
#from dgram import Dgram
#

def setAttr(d, attr, value, verbose=None):
    if verbose is not None:
        verbose0=d.verbose
        d.verbose=verbose
    print("Set value of %s:" % (attr))
    setattr(d, attr, value)
    print(value)
    if verbose is not None:
        d.verbose=verbose0
    return True

def getAttr(d, attr, verbose=None):
    if verbose is not None:
        verbose0=d.verbose
        d.verbose=verbose
    print("Get value of %s (id=%s):" % (attr, id(attr)))
    value=getattr(d, attr)
    print(value)
    if verbose is not None:
        d.verbose=verbose0
    return value

def do_it(args_proper, xtcdata_filename, verbose, debug):
    print("gc.isenabled(): %s" % gc.isenabled())
    print("gc.get_debug(): %s" % gc.get_debug())
    print("gc.get_stats():")
    pprint.pprint(gc.get_stats())
    print()

    print("get d1")
    #fd = os.open(xtcdata_filename, os.O_RDONLY|os.O_LARGEFILE)
    #d1=Dgram(fd, verbose, debug)
    d1=pydgram.PyDgram(xtcdata_filename, verbose=verbose, debug=debug)
    print("dir(d1):"); dir(d1)
    print("d1:", d1)
    print("gc.is_tracked(d1): %s" % gc.is_tracked(d1))
    print("gc.get_referrers(d1)")
    pprint.pprint(gc.get_referrers(d1))
    print("gc.get_referents(d1)")
    pprint.pprint(gc.get_referents(d1))
    print("gc.garbage: %s" % gc.garbage)
    print()

    print("get a1")
    a1=getAttr(d1, 'array0')
    print("id(a1):", id(a1))
    print("a1.base:", a1.base)
    print("gc.is_tracked(a1): %s" % gc.is_tracked(a1))
    print("gc.get_referrers(a1)")
    pprint.pprint(gc.get_referrers(a1))
    print("gc.get_referents(a1)")
    pprint.pprint(gc.get_referents(a1))
    print("get a2")
    a2=getAttr(d1, 'array0')
    print("id(a2):", id(a2))
    print("a2.base:", a2.base)
    print("gc.is_tracked(a2): %s" % gc.is_tracked(a2))
    print("gc.get_referrers(a2)")
    pprint.pprint(gc.get_referrers(a2))
    print("gc.get_referents(a2)")
    pprint.pprint(gc.get_referents(a2))
    print("gc.garbage: %s" % gc.garbage)
    print()

    print("del d1")
    del d1
    print()

    print("show a1")
    print("id(a1):", id(a1))
    print("a1.base:", a1.base)
    print()
    print("del a1"); del a1
    print()

    print("show a2")
    print("id(a2):", id(a2))
    print("a2.base:", a2.base)
    print()
    print("del a2"); del a2
    print()

def parse_command_line():
    opts, args_proper = getopt.getopt(sys.argv[1:], 'hvd:')
    verbose=0
    debug=0
    xtcdata_filename=None
    for option, parameter in opts:
        if option=='-h': usage_error()
        if option=='-v': verbose+=1
        if option=='-d': debug = int(parameter)
    if xtcdata_filename is None: xtcdata_filename="data.xtc"
    if verbose>0:
        sys.stdout.write("xtcdata filename: %s\n" % xtcdata_filename)
        sys.stdout.write("verbose: %d\n" % verbose)
        sys.stdout.write("debug: %d\n" % debug)
    elif debug>0:
        sys.stdout.write("debug: %d\n" % debug)
    return (args_proper, xtcdata_filename, verbose, debug)

def main():
    args_proper, xtcdata_filename, verbose, debug = parse_command_line()

    do_it(args_proper, xtcdata_filename, verbose, debug)

    return

def usage_error():
    s="usage: python %s" %  os.path.basename(sys.argv[0])
    sys.stdout.write("%s [-h] [-v] [-d <DEBUG_NUMBER>]\n" % s)
    sys.exit(1)

if __name__=='__main__':
    main()
