from cpython cimport array
import array
from psana.dgram import Dgram
from psana.event import Event
import numpy as np

cdef class EventBuilder:
    """Builds a batch of events
    Takes memoryslice 'views' and identifies matching timestamp
    dgrams as an event. Returns list of events (size=batch_size)
    as another memoryslice 'batch'.
    
    views: each smd file is seperated by b'endofstream'
    batch: each dgram and event is seperated by b'eod' and b'eoe'."""
    cdef short nsmds
    cdef array.array offsets 
    cdef array.array sizes
    cdef array.array timestamps
    cdef array.array dgram_sizes
    cdef array.array dgram_timestamps
    cdef array.array event_timestamps
    cdef list views
    cdef list configs
    cdef unsigned nevents

    def __init__(self, views, configs):
        self.nsmds = len(views)
        self.offsets = array.array('I', [0]*self.nsmds)
        self.sizes = array.array('I', [memoryview(view).nbytes for view in views])
        self.timestamps = array.array('L', [0]*self.nsmds)
        self.dgram_sizes = array.array('I', [0]*self.nsmds)
        self.dgram_timestamps = array.array('L', [0]*self.nsmds)
        self.event_timestamps = array.array('L', [0]*self.nsmds)
        self.views = views
        self.configs = configs
        self.nevents = 0

    def _has_more(self):
        for i in range(self.nsmds):
            if self.offsets[i] < self.sizes[i]:
                return True
        return False

    def build(self, unsigned batch_size=1, filter=0):
        cdef unsigned got = 0
        batch = bytearray()
        while got < batch_size and self._has_more():
            array.zero(self.timestamps)
            array.zero(self.dgram_sizes)
            dgrams = []
            for i, view in enumerate(self.views):
                if self.offsets[i] < self.sizes[i]:
                    d = Dgram(config=self.configs[i], view=view, offset=self.offsets[i])
                    dgrams.append(d)
                    self.timestamps[i] = d.seq.timestamp()
                    self.dgram_sizes[i] = memoryview(d).shape[0]

            sorted_smd_id = np.argsort(self.timestamps)
            for smd_id in sorted_smd_id:
                if self.timestamps[smd_id] == 0:
                    continue

                evt = Event(size=self.nsmds)
                array.zero(self.event_timestamps)
                self.event_timestamps[smd_id] = self.timestamps[smd_id]
                self.offsets[smd_id] += self.dgram_sizes[smd_id]
                evt.replace(smd_id, dgrams[smd_id])
                for i, view in enumerate(self.views):
                    if i == smd_id or self.offsets[i] >= self.sizes[i]:
                        continue

                    d = Dgram(config=self.configs[i], view=view, offset=self.offsets[i])
                    while d.seq.timestamp() <= self.event_timestamps[smd_id]:
                        if d.seq.timestamp() == self.event_timestamps[smd_id]:
                            self.event_timestamps[i] = d.seq.timestamp()
                            self.timestamps[i] = 0
                            evt.replace(i, d)

                        self.offsets[i] += memoryview(d).shape[0]

                        if self.offsets[i] == self.sizes[i]:
                            break

                        d = Dgram(config=self.configs[i], view=view, offset=self.offsets[i])
                
                if filter:
                    if filter(evt):
                        batch.extend(evt.to_bytes())
                        got += 1
                else:
                    batch.extend(evt.to_bytes())
                    got += 1
                
                if got == batch_size:
                    break
                
        
        self.nevents = got
        return batch

    @property
    def nevents(self):
        return self.nevents
