#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import pickle

DIR = "raw_plots"
PLOT = f"{DIR}/ccdf_1755_cutset"
fig_object = pickle.load(open(f'{PLOT}','rb'))
fig_object.show()
