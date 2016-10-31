import numpy as np
#Motion parameters
g = -9.81        #acceleration due to gravity assume to be constant for now
y = 0            #define ground level     
t=0            #define initial time
dt=0.001        #small increment of time in 1ms
v=0            #initial velocity
Ymax=0            #Register maximum height

#Drag parameters
SA_rock = 0.003         #Surface Area of rocket
SA_drogue= 0.1        #Surface Area of small parachute (I picked 0.1m^2 so that when simulating we don't have to wait too long before the program finishs, should really be about 3m^2)
SA_main= 10        #Surface Area of main parachute
rho = 1.225         #air density assume to be constant for now
Cd_rock = 0.2         #Coefficient of drag of rocket
Cd_drogue= 0.8        #Coefficient of drag of small parachute
Cd_main= 0.8        #Coefficient of drag of main parachute

#Engine parameters
Br = 1            #-dm/dt
Vex = 2000        #exhaust velocity relative to rocket
Md = 20            #Mass of empty
Mw = 40            #Mass of rocket + fuel
m=Mw            #initial mass of rocket is wet mass

def drag(SA, rho, Cd, v):        #Drag equation
    Fd=0.5*rho*Cd*SA*v**2    
    return Fd
def grav(y,m):                #Newton's law of Gravitation    
    Fg=6.63e-11*5.98e24*m/(y+6.37e6)**2
    return Fg
def thrust(Vex,Br):            #Thrust force
    Ft= Vex*Br
    return Ft

data = []




#powered_ascend
while y>= 0 and m >=Md:
    t+=dt
    a=(thrust(Vex,Br)-grav(y,m)-drag(SA_rock, rho, Cd_rock, v))/m
    v=v+a*dt
    y=y+v*dt
    m=m-Br*dt
    
    data.append([t,v,a,y,m])

#burnout
while m<=Md and v>=0:                            #motion under gravity(and drag)
    t+=dt
    a=(-grav(y,m)-drag(SA_rock, rho, Cd_rock, v))/m
    v=v+a*dt
    y=y+v*dt
    data.append([t,v,a,y,m])
    if v<0:
        Ymax=y
#apogee
#drogue_decend
while v<=0 and y>=300:
    t+=dt
    a=(-grav(y,m)+drag(SA_drogue, rho, Cd_drogue, v))/m        #small parachute is released
    v=v+a*dt
    y=y+v*dt
    data.append([t,v,a,y,m])
    
    
#main
while y<=300 and y>=0:
    t+=dt
    a=(-grav(y,m)+drag(SA_main, rho, Cd_main, v))/m            #main parachute is released
    v=v+a*dt
    y=y+v*dt
    data.append([t,v,a,y,m])
#land
print('tada!!','Max height reached:', Ymax)

height = [x[3] for x in data]
acc = [x[2] for x in data]
vel = [x[1] for x in data]

import matplotlib.pyplot as plt
import math

# plt.plot(height)

# plt.plot(vel)

# plt.plot(acc)

M = 28.9645
R = 8.31
L = [-0.0065, 0.0, 0.001, 0.0028, 0.0, -0.0028, -0.002]
P = [101325.0, 22632.10, 5474.89, 868.02, 110.91, 66.94, 3.96]
T = [288.15, 216.65, 216.65, 228.65, 270.65, 270.65, 214.65]
H = [0.0, 11000.0, 20000.0, 32000.0, 47000.0, 51000.0, 71000.0]
g0 = 9.8


# plt.plot(P)


def get_rho(h):
    b = get_b(h)
    
    if L[b] != 0:
        rho = P[b] * ( T[b] / (T[b] + L[b]*(h - H[b])) ) ** (M*g0 / R*L[b])
    if L[b] == 0:
        rho = P[b] * math.exp(-g0 * M * (h-H[b]) / R * T[b])
    return rho

rhos = [get_rho(h) for h in range(70000)]

# plt.plot(rhos)

def get_b(h):
    if h >= 0 and h < 11000:
        return 0
    elif h >= 11000 and h < 20000:
        return 1
    elif h >= 20000 and h < 32000:
        return 2
    elif h >= 32000 and h < 47000:        
        return 3
    elif h >= 47000 and h < 51000:
        return 4
    elif h >= 51000 and h < 71000:
        return 5
    elif h >= 71000:
        return 6
    else:
        return 'error'



