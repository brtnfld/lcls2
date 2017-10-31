#!/usr/bin/env python
#

import sys, os
import time
import getopt
import pprint

sys.path.append('../build/psana')
import dgram
#

class DataSource:
    """Stores variables and arrays loaded from an XTC source.\n"""
    def __init__(self, xtcdata_filename, verbose=0, debug=0):
        fd = os.open(xtcdata_filename,
                     os.O_RDONLY|os.O_LARGEFILE)
        self._config = dgram.Dgram(file_descriptor=fd,
                                   verbose=verbose,
                                   debug=debug)
    def __iter__(self):
        return self

    def __next__(self):
        d=dgram.Dgram(config=self._config,
                      verbose=self._get_verbose(),
                      debug=self._get_debug())
        for var_name in self.event_variables():
            delattr(self, var_name)
        for key in sorted(d.__dict__.keys()):
            setattr(self, key, getattr(d, key))
        return self

    def _get_verbose(self):
        return getattr(self._config, "verbose")
    def _set_verbose(self, value):
        setattr(self._config, "verbose", value)
    _verbose = property(_get_verbose, _set_verbose)

    def _get_debug(self):
        return getattr(self._config, "debug")
    def _set_debug(self, value):
        setattr(self._config, "debug", value)
    _debug = property(_get_debug, _set_debug)

    def event_variables(self):
        var_names=[]
        for var_name in sorted(vars(self)):
            if var_name != "_config":
                var_names.append(var_name)
        return var_names

    def print_event_variables(self):
        for var_name in self.event_variables():
            v=getattr(self, var_name)
            t=type(v).__name__
            if t=="ndarray":
                print("%s: %s array with shape %s" % (var_name, v.dtype, v.shape))
            else:
                print("%s: %s variable with value" % (var_name, t), v)


def parse_command_line():
    opts, args_proper = getopt.getopt(sys.argv[1:], 'hvd:f:')
    verbose=0
    debug=0
    xtcdata_filename="data.xtc"
    for option, parameter in opts:
        if option=='-h': usage_error()
        if option=='-v': verbose+=1
        if option=='-d': debug = int(parameter)
        if option=='-f': xtcdata_filename = parameter
    if xtcdata_filename is None:
        xtcdata_filename="data.xtc"
    if verbose>0:
        sys.stdout.write("xtcdata filename: %s\n" % xtcdata_filename)
        sys.stdout.write("verbose: %d\n" % verbose)
        sys.stdout.write("debug: %d\n" % debug)
    elif debug>0:
        sys.stdout.write("debug: %d\n" % debug)
    return (args_proper, xtcdata_filename, verbose, debug)

def getMemUsage():
    pid=os.getpid()
    ppid=os.getppid()
    cmd="/usr/bin/ps -q %d --no-headers -eo size" % pid
    p=os.popen(cmd)
    size=int(p.read())
    return size

def main():
    args_proper, xtcdata_filename, verbose, debug = parse_command_line()
    ds=DataSource(xtcdata_filename, verbose=verbose, debug=debug)
    count=0
    ds.print_event_variables()
    for evt in ds:
        print("evt:", count)
        evt.print_event_variables()
        a=evt.array0_pgp
        try:
            a[0][0]=999
        except ValueError:
            print("The evt.array0_pgp array is read-only, as it should be.")
        else:
            print("Warning: the evt.array0_pgp array is writable")
        print()
        count+=1
    return

def usage_error():
    s="usage: python %s" %  os.path.basename(sys.argv[0])
    sys.stdout.write("%s [-h] [-v] [-d <DEBUG_NUMBER>]\n" % s)
    sys.stdout.write("%s [-f xtcdata_filename]\n" % (" "*len(s)))
    sys.exit(1)

if __name__=='__main__':
    main()
