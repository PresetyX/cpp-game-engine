# Regenerates torus.obj in the current directory: python torus_generator.py
import math
R, r, N, M = 0.8, 0.3, 32, 16
out = ["# Torus, major radius 0.8, minor radius 0.3, %d x %d segments." % (N, M),
       "# Generated for this project (torus_generator.py). CC0 1.0 / public domain."]
for i in range(N):
    for j in range(M):
        u = 2*math.pi*i/N; v = 2*math.pi*j/M
        out.append("v %.5f %.5f %.5f" % ((R+r*math.cos(v))*math.cos(u), r*math.sin(v), (R+r*math.cos(v))*math.sin(u)))
for i in range(N+1):
    for j in range(M+1):
        out.append("vt %.5f %.5f" % (i/N, j/M))
for i in range(N):
    for j in range(M):
        u = 2*math.pi*i/N; v = 2*math.pi*j/M
        out.append("vn %.5f %.5f %.5f" % (math.cos(v)*math.cos(u), math.sin(v), math.cos(v)*math.sin(u)))
def vi(i,j): return (i%N)*M + (j%M) + 1
def ti(i,j): return i*(M+1) + j + 1
for i in range(N):
    for j in range(M):
        a=(i,j); b=(i+1,j); c=(i+1,j+1); d=(i,j+1)
        # counter-clockwise seen from outside
        out.append("f " + " ".join("%d/%d/%d" % (vi(*p), ti(*p), vi(*p)) for p in (a,d,c)))
        out.append("f " + " ".join("%d/%d/%d" % (vi(*p), ti(*p), vi(*p)) for p in (a,c,b)))
open("torus.obj","w",newline="\n").write("\n".join(out)+"\n")
