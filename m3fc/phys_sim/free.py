#initial values
t=0
u = 50
a = -9.81
h=0


def velocity (u, a, t):
	v = u+a*t	
	return v

def altitude (u, a, t):
	h = u*t + 0.5*a*t**2
	return h

while altitude (u, a, t) >= 0:
	t+=0.001
	print (t, velocity (u, a, t), a, altitude (u, a, t))