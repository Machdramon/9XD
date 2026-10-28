import numpy as np, matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.collections import PolyCollection
def render(V,F,path,title='',M=None):
    if not V: return
    P=np.concatenate(V);T=np.concatenate(F) if F else np.zeros((0,3),int)
    if len(T)==0: return
    # drop outlier triangles for view bounds
    c=np.median(P,0);r=np.percentile(np.linalg.norm(P-c,axis=1),98)*1.2
    fig,axs=plt.subplots(1,3,figsize=(15,5),facecolor='#1e1e1e')
    views=[(0,1,2,'front XY'),(0,2,1,'top XZ'),(2,1,0,'side ZY')]
    for ax,(i,j,k,name) in zip(axs,views):
        tri=P[T];depth=tri[:,:,k].mean(1);order=np.argsort(depth)
        n=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]);n/=np.linalg.norm(n,axis=1).clip(1e-12)[:,None]
        shade=0.25+0.75*np.abs(n[:,k])
        if M is not None:
            pid=np.concatenate([np.full(len(f),m) for f,m in zip(F,M)])
            pal=np.array(matplotlib.colormaps['tab20'].colors)[pid%20]
            cols=np.concatenate([pal*shade[:,None],np.ones((len(shade),1))],1)
        else: cols=np.stack([shade*0.55,shade*0.7,shade*0.95,np.ones_like(shade)],1)
        pc=PolyCollection(tri[order][:,:,[i,j]],facecolors=cols[order],edgecolors='none')
        ax.add_collection(pc);ax.set_xlim(c[i]-r,c[i]+r);ax.set_ylim(c[j]-r,c[j]+r);ax.set_aspect('equal')
        ax.set_facecolor('#1e1e1e');ax.set_title(name,color='w');ax.tick_params(colors='#666')
    fig.suptitle(title,color='w');fig.tight_layout();fig.savefig(path,dpi=70);plt.close(fig)
