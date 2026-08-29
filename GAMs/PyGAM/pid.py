import numpy as np
import pygam
from copy import *

# --- CODE INFO ---

pygam.RootInputDict['pid'] = {
        'types'      : ( np.float64, ),
        'dimensions' : ( (1, 1),  ),
        'names'      : ( 'Input',   )
        }

pygam.RootOutputDict['pid'] = {
        'types'      : ( np.float64, ),
        'dimensions' : ( (1, 1),     ),
        'names'      : ( 'Output',        )
        }

pygam.ParameterDict['pid'] = {
        'types'        : ( np.float64,   np.float64, np.float64, np.float64   ), 
        'dimensions'   : ( (1,1),        (1,1),      (1,1), (1,1)        ),
        'names'        : ( 'Kp',     'Ki',  'Kd', 'T'     ),
        'defaultValues': ( [1.],      [0.],    [0.], [1E-3] )
        }

# --- MAIN FUNCTION ---


#Past history required for pid 
global y_1, x_1, x_2, x_fact, x_1_fact, x_2_fact
y_1 = float(0.)
x_1 = float(0.)
x_2 = float(0.)
#PID factors in Z, computed once for all
x_fact = 0
x_1_fact = 0
x_2_fact = 0


def setup():
    global x_fact,x_1_fact,x_2_fact
    Kp = pygam.data['Kp']    
    Ki = pygam.data['Ki']   
    Kd = pygam.data['Kd']
    T = pygam.data['T']
    print('Kp: '+str(Kp))    
    print('Ki: '+str(Ki))   
    print('Kd: '+str(Kd))    
    print('T: '+str(T))    

    x_fact = float(Kp + T*Ki + Kd/T)
    x_1_fact = float(-(Kp+2.*Kd/T))
    x_2_fact = float(Kd/T)

def execute(x):
    global y_1
    global x_1, x_2, x_fact, x_1_fact, x_2_fact

    ciccio = copy(x_2)+0.*x

    y =  y_1 + x_fact * x + x_1_fact * x_1 + x_2_fact * x_2
    y_1 = copy(y)
    x_2 = copy(x_1)
    x_1 = copy(x)

    

     # return value shall be a tuple, hence the extra comma
    return y,
