import dpllv2
import numpy as np

samplerate = 44100.0
numchannels = 2
pl = dpllv2.createPeaklim(samplerate, numchannels)
assert(pl)
pl.set_input_gain(0.0)
pl.set_threshold(40.0)
pl.set_release(.05)
pl.set_truepeak(False)
latency = pl.get_latency()
print(f"dppplv2 latency: {latency}")

numframes = 128
shape = (numchannels, numframes)
dtype = np.float32
input=np.ones(shape, dtype=np.float64)
output=np.zeros(shape, dtype=np.float32)
input = np.resize(input, output.shape)

pl.process(np.float32(input), np.float32(output))

print(f"peak: {pl.peak} gmax: {pl.gmax} gmin: {pl.gmin} \n output: {output}")
