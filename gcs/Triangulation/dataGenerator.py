import random, time
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

random.seed()
class Rocket:
    def __init__(self):
        self.ToF=0

        self.posx=0
        self.posy=0
        self.posz=0

        self.velx=0
        self.vely=0
        self.velz=0

        self.accx=0
        self.accy=0
        self.accz=0

    def update(self, dt):
        self.ToF+=dt

        if self.posz<0:
            self.posz=-1
        else:
            self.accx = random.gauss(0, 10)
            self.accy = random.gauss(0, 10)
            if self.ToF < 4:
                self.accz=random.gauss(30,10)
            else:
                self.accz = -9.81

            self.velx += self.accx*dt
            self.vely += self.accy*dt
            self.velz += self.accz*dt

            self.posx+=self.velx
            self.posy+=self.vely
            self.posz+=self.velz

    def getPos(self):
        return ( self.posx + random.gauss(0,10), 
                 self.posy + random.gauss(0,10),
                 self.posz + random.gauss(0,10) )


rocket = Rocket()
data = [ [], [], [] ]
for i in range(0,1000):
    rocket.update(0.1)
    pos=rocket.getPos()
    print(pos)
    #print(random.randrange(1,100))
    time.sleep(0.0001)
    data[0].append(pos[0])
    data[1].append(pos[1])
    data[2].append(pos[2])

fig = plt.figure()
ax = fig.add_subplot(111,projection='3d')
ax.scatter(data[0], data[1], data[2])

plt.xlim(-5000,5000)
plt.ylim(-5000,5000)
plt.show()
