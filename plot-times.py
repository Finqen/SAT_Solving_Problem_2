import pandas as pd
import seaborn as sns
from matplotlib import pyplot as plt

df_times = pd.read_csv('cmake-build-debug/times.csv', index_col=0)

df_times = df_times.transpose()

sns_plot_times = sns.lineplot(data=df_times, markers=True, dashes=False)

sns_plot_times.set(xlabel='Problems solved', ylabel='Computation time [ms]')

fiugre_times = sns_plot_times.get_figure()

fiugre_times.savefig("cactus-times.png")



