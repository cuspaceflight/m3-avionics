#Motion parameters
g = -9.81		#acceleration due to gravity assume to be constant for now

#Drag parameters
SA_roc = 0.031 		#Surface Area of rocket
rho = 1.225 		#air density assume to be constant for now
Cd = ? 			#Coefficient of drag of rocket

def drag(SA_roc, rho, Cd, v)		#Drag equation
	F=0.5*air_density*Cd*A*v**2	
	return F

#Engine parameters
Br = ?		#-dm/dt
Ev = ?		#exhaust velocity relative to rocket
Md = ?		#Mass of empty
Mw = ?		#Mass of rocket + fuel
