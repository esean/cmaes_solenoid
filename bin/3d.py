#!/usr/bin/env python
import sys
import re
from matplotlib import pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.animation
import numpy as np

def usage(x):
    print sys.argv[0]," [xyz file]"
    print "where [xyz file] format is,"
    print " '[x],[y],[z]'"
    sys.exit(x)

if len(sys.argv) < 2:
    usage(1)

INFILE = sys.argv[1]

x=list()
y=list()
z=list()
maxx = maxy = maxz = 0.0

with open(INFILE) as f:
    lines = f.readlines()
    x1=list()
    y1=list()
    z1=list()
    for line in lines:
        lx = line.rstrip()
        a = re.match('([^,]*),([^,]*),(.*)',lx)
        if a:
            #print "# POINT:",a.group(1),a.group(2),a.group(3)
            xf = float(a.group(1))
            yf = float(a.group(2))
            zf = float(a.group(3))
            x1.append(xf)
            y1.append(yf)
            z1.append(zf)
            if xf > maxx: maxx = xf
            if yf > maxy: maxy = yf
            if zf > maxz: maxz = zf
        else:
            if len(x1)>0:
                x.append(x1)
                y.append(y1)
                z.append(z1)
                #print "# INSERT GROUP, x=",x," x1=",x1
                x1=[]
                y1=[]
                z1=[]

def animate(i):
    line.set_data(x[i], y[i])
    line.set_3d_properties(z[i])
    line.set_color(colorst[i])
    return line,

fig = plt.figure(figsize = (12,8))
ax = fig.add_subplot(111, projection='3d')
line, = ax.plot(x[0], y[0], z[0], c='r',marker='o')

colormap = plt.cm.gist_ncar #nipy_spectral, Set1,Paired
colorst = [colormap(i) for i in np.linspace(0, 0.9,len(x))]

ax.set_xlim(0,maxx)
ax.set_ylim(0,maxy)
ax.set_zlim(0,maxz)

anim = matplotlib.animation.FuncAnimation(fig, animate,
                               frames=len(x), interval=200, blit=True, repeat=False)
anim.save('%s.mp4' % INFILE, fps=10, extra_args=['-vcodec', 'libx264'])
plt.show()

