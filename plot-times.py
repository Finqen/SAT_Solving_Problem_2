import pandas as pd
import seaborn as sns
from matplotlib import pyplot as plt

df_times = pd.read_csv('cmake-build-debug/times.csv', index_col=0)

a = df_times.to_numpy()

a.sort(axis=1)

for y in range((len(a))-1):
    for x in range((len(a[0])-1)):
        if(x>0):
            a[y,x]=a[y,x]+a[y,x-1]

df_times.replace(a)

df_times = df_times.transpose()

sns_plot_times = sns.lineplot(data=df_times, markers=True, dashes=False)

sns_plot_times.set(xlabel='Problems solved', ylabel='Computation time [ms]', xticks=[5,10,15,20,25,30])

fiugre_times = sns_plot_times.get_figure()

fiugre_times.savefig("cactus-times.png")



