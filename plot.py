import pandas as pd
import seaborn as sns
from matplotlib import pyplot as plt

df = pd.read_csv('data.csv', index_col=0)

sns_plot = sns.lineplot(data=df, markers=True, dashes=False)

sns_plot.set(xlabel='Problems solved', ylabel='Computation time [ms]')

fiugre = sns_plot.get_figure()

fiugre.savefig("cactus.png")
