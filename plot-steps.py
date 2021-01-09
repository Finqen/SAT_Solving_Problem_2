import pandas as pd
import seaborn as sns
from matplotlib import pyplot as plt

df_steps = pd.read_csv('cmake-build-debug/steps.csv', index_col=0)


a = df_steps.to_numpy()

a.sort(axis=1)

for y in range((len(a))-1):
    for x in range((len(a[0])-1)):
        if(x>0):
            a[y,x]=a[y,x]+a[y,x-1]

df_steps.replace(a)


df_steps = df_steps.transpose()

sns_plot_steps = sns.lineplot(data=df_steps, markers=True, dashes=False)

sns_plot_steps.set(xlabel='Problems solved', ylabel='Steps', xticks=[5,10,15,20,25,30])

fiugre_steps = sns_plot_steps.get_figure()

fiugre_steps.savefig("cactus-steps.png")