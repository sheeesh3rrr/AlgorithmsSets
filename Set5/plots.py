import pandas as pd
import numpy as np
import matplotlib.pyplot as plt


plt.style.use('ggplot')

def graph1():

    df = pd.read_csv("graph1.csv")
    error = abs(df["estimate"] - df["exact"])

    plt.figure(figsize=(10,6))

    plt.plot(df["size"], df["exact"],
             linewidth=2,
             label="Точное значение $F_0(t)$")

    plt.plot(df["size"], df["estimate"],
             linestyle='--',
             linewidth=2,
             label="Оценка HyperLogLog $N_t$")

    plt.fill_between(df["size"],
                     df["estimate"] - error,
                     df["estimate"] + error,
                     alpha=0.2,
                     label="Область ошибки")

    plt.xlabel("Размер обработанного потока")
    plt.ylabel("Количество уникальных элементов")
    plt.title("Сравнение точного значения и оценки HyperLogLog")

    plt.legend()
    plt.grid(alpha=0.3)

    plt.savefig("plot1_hll_vs_exact.png", dpi=300, bbox_inches='tight')
    plt.show()

def graph2():

    df = pd.read_csv("graph2.csv")

    plt.figure(figsize=(10,6))

    plt.plot(df["step"], df["mean"],
             linewidth=2,
             label="Математическое ожидание $E(N_t)$")

    plt.fill_between(df["step"],
                     df["mean"] - df["stddev"],
                     df["mean"] + df["stddev"],
                     alpha=0.25,
                     label="Область $E(N_t) \\pm \\sigma$")

    plt.xlabel("Доля обработанного потока")
    plt.ylabel("Оценка количества уникальных")
    plt.title("Статистика оценки HyperLogLog")

    plt.legend()
    plt.grid(alpha=0.3)

    plt.savefig("plot2_statistics.png", dpi=300, bbox_inches='tight')
    plt.show()

if __name__ == "__main__":

    graph1()
    graph2()
