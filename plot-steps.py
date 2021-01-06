import pandas as pd
import seaborn as sns
from matplotlib import pyplot as plt

df_steps = pd.read_csv('cmake-build-debug/steps.csv', index_col=0)

df_steps = df_steps.transpose()

sns_plot_steps = sns.lineplot(data=df_steps, markers=True, dashes=False)

sns_plot_steps.set(xlabel='Problems solved', ylabel='Steps')

fiugre_steps = sns_plot_steps.get_figure()

fiugre_steps.savefig("cactus-steps.png")