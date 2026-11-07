import numpy as np
import matplotlib.pyplot as plt

plt.rcParams['font.sans-serif'] = ['Microsoft YaHei']
plt.rcParams['axes.unicode_minus'] = False

point=np.array([(-2, 0), (0, 0.9), (2, 2.0), (3, 6.5),
    (4, 2.9), (5, 8.8), (6, 3.95), (8, 5.03),
    (10, 5.97), (12, 7.1), (13, 1.2), (14, 8.2),
    (16, 8.5), (18, 10.1)],dtype=float)


def getline(p1,p2):
    a=p2[1]-p1[1]
    b=p1[0]-p2[0]
    c=p2[0]*p1[1]-p1[0]*p2[1]
    return a,b,c

def getdistance(point,line):
    a,b,c=line
    d=np.sqrt(a*a+b*b)
    dist=np.abs((a*point[:,0]+b*point[:,1]+c)/d)
    return dist

def ransac(point,cnt=1000,d=0.5):
    rng=np.random.default_rng(42)
    ans=[]
    for _ in range(cnt):
        [idx1,idx2]=rng.choice(len(point),2,replace=False)
        p1=point[idx1]
        p2=point[idx2]
        line=getline(p1,p2)
        dist=getdistance(point,line)
        cur=np.where(dist<=d)[0]
        if len(cur)>len(ans):
            ans=cur
    return ans

def fit(point,ans):
    cur=point[ans]
    A=np.column_stack((cur[:,0],np.ones(len(cur))))
    k,b=np.linalg.lstsq(A,cur[:,1],rcond=None)[0]
    return k,b

ans=ransac(point)
k,b=fit(point,ans)
out=np.ones(len(point),dtype=bool)
out[ans]=False
xline=np.linspace(-2,18,300)
yline=k*xline+b

plt.figure(figsize=(9,6))
plt.scatter(point[ans,0],point[ans,1],label=f"内点({len(ans)})")
plt.scatter(point[out,0],point[out,1],label=f"外点({len(point)-len(ans)})",marker="x")
for i, (x, y) in enumerate(point):
    plt.annotate(
        f"({x:g}, {y:g})",
        (x, y),
        xytext=(5, 5),
        textcoords="offset points",
        fontsize=10
    )
plt.plot(xline,yline,label=f"最小二乘拟合：y={k:.4f}x+{b:.4f}")
plt.xlabel("x")
plt.ylabel("y")
plt.title("RANSAC 拟合二维直线")
plt.grid(True)
plt.legend()
plt.tight_layout()

plt.savefig("RANSAC_result.png", dpi=220)
plt.show()

print("斜率：", k)
print("截距：", b)
print("内点数量：", len(ans))
print("离群点数量：", len(point) - len(ans))