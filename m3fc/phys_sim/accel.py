#Motion parameters
g = -9.81		#acceleration due to gravity assume to be constant for now
y =0			#define ground level 	
t=0			#define initial time
dt=0.001		#small increment of time in 1ms
v=0			#initial velocity
Ymax=0			#Register maximum height

#Drag parameters
SA_roc = 0.031 		#Surface Area of rocket
SA_chute1=0.031*5	#Surface Area of small parachute
SA_main=0.031*50	#Surface Area of main parachute
rho = 1.225 		#air density assume to be constant for now
Cd_rock = 0.5 		#Coefficient of drag of rocket
Cd_chute1=1.75		#Coefficient of drag of small parachute
Cd_main=1.75		#Coefficient of drag of main parachute

#Engine parameters
Br = 10			#-dm/dt
Vex = 1000		#exhaust velocity relative to rocket
Md = 100		#Mass of empty
Mw = 200		#Mass of rocket + fuel
m=Mw			#initial mass of rocket is wet mass

def drag(SA, rho, Cd, v):		#Drag equation
	Fd=0.5*rho*Cd*SA*v**2	
	return Fd
def grav(y,m):				#Newton's law of Gravitation	
	Fg=6.63e-11*5.98e24*m/(y+6.37e6)**2
	return Fg
def thrust(Vex,Br):			#Thrust force
	Ft= Vex*Br
	return Ft



#powered_ascend
while y>= 0 and m >=Md:
	t+=dt
	a=(thrust(Vex,Br)-grav(y,m)-drag(SA_roc, rho, Cd_rock, v))/m
	v=v+a*dt
	y=y+v*dt
	m=m-Br*dt
	
	print(t,v,a,y,m)

#burnout
while m<=Md and v>=0:
	t+=dt
	a=(-grav(y,m)-drag(SA_roc, rho, Cd_rock, v))/m
	v=v+a*dt
	y=y+v*dt
	print(t,v,a,y,m)
	if v<0:
		Ymax=y
#apogee
#drogue_decend
while v<=0 and y>=300:
	t+=dt
	a=(-grav(y,m)+drag(SA_chute1, rho, Cd_chute1, v))/m
	v=v+a*dt
	y=y+v*dt
	print(t,v,a,y,m)
#main
while y<=300 and y>=0:
	t+=dt
	a=(-grav(y,m)+drag(SA_main, rho, Cd_main, v))/m
	v=v+a*dt
	y=y+v*dt
	print(t,v,a,y,m)
#land
print('tada!!','Max height reached:', Ymax)