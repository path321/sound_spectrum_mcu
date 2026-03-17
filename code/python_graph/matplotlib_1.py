# Source - https://stackoverflow.com/a/58190549
# Posted by Amit
# Retrieved 2026-03-14, License - CC BY-SA 4.0

import numpy as np
from matplotlib import pyplot as plt
from matplotlib.animation import FuncAnimation
from get_from_serial import get_data

fig, ax = plt.subplots()
plot, = plt.plot([],[])


def init_function():
    ax.set_xlim(0,2148)
    ax.set_ylim(0,5000)
    return plot,

def Redraw_Function(UpdatedVal):
    val = get_data()
    new_x = list(range(len(val)))#np.arange(500) #np.mulitply(list(range(len(val))),23)#
    new_y = val #np.arange(500) #np.arange(500)**2*UpdatedVal #
    #print(len(new_x),len(new_y))
    plot.set_data(new_x,new_y)
    return plot,

Animated_Figure = FuncAnimation(fig,Redraw_Function,init_func=init_function,frames=np.arange(1,5,1),interval=1000)
plt.show()
# Animated_Figure.save('MyAnimated.gif',writer='imagemagick')


    
