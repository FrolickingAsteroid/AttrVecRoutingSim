#!/usr/bin/env python3
import pickle
import matplotlib.pyplot as plt
import pylustrator

pylustrator.start()

DIR = "raw_plots"
PLOT_1 = f"{DIR}/cliquePointPlot_unit"
PLOT_2 = f"{DIR}/cliquePointPlot_unit_all"


with open(PLOT_1, "rb") as f:
    fig_object_1 = pickle.load(f)

with open(PLOT_2, "rb") as f:
    fig_object_2 = pickle.load(f)

# Make sure pyplot knows about the unpickled figure as figure 1
if fig_object_1 not in [plt.figure(n) for n in plt.get_fignums()]:
    dummy = plt.figure(1)
    manager = dummy.canvas.manager
    manager.canvas.figure = fig_object_1
    fig_object_1.set_canvas(manager.canvas)
    fig_object_1.number = 1

if fig_object_2 not in [plt.figure(n) for n in plt.get_fignums()]:
    dummy = plt.figure(1)
    manager = dummy.canvas.manager
    manager.canvas.figure = fig_object_2
    fig_object_2 .set_canvas(manager.canvas)
    fig_object_2 .number = 1

# % start: automatic generated code from pylustrator
plt.figure(1).ax_dict = {ax.get_label(): ax for ax in plt.figure(1).axes}
import matplotlib as mpl

getattr(plt.figure(1), '_pylustrator_init', lambda: ...)()

# % end: automatic generated code from pylustrator

plt.show()
