"""
plot_results.py
Doc benchmark_results.json, tinh Speedup va Hieu suat,
xuat bang so lieu va du lieu pgfplots cho LaTeX.
"""
import json, math, os

with open("benchmark_results.json", "r") as f:
    data = json.load(f)

results = data["results"]
threads = [1, 2, 8]

# ------------------------------------------------------------------ #
# 1.  Xuat bang so lieu  (benchmark_table.tex)                       #
# ------------------------------------------------------------------ #
lines_table = []
lines_table.append(r"\begin{table}[h!]")
lines_table.append(r"  \centering")
lines_table.append(r"  \caption{K\'{e}t qu\h{a} th\d{o}i gian, Speedup v\`{a} Hi\d{e}u su\'\^{a}t c\h{u}a t\'{\i}ch v\^{o} h\u{u}\'{o}ng song song}")
lines_table.append(r"  \label{tab:dot_benchmark}")
lines_table.append(r"  \vspace{0.2cm}")
lines_table.append(r"  \small")
lines_table.append(r"  \begin{tabular}{r|rrr|rrr|rrr}")
lines_table.append(r"    \toprule")
lines_table.append(r"    \textbf{n} & \multicolumn{3}{c|}{\textbf{Th\`{o}i gian (s)}} & \multicolumn{3}{c|}{\textbf{Speedup}} & \multicolumn{3}{c}{\textbf{Hi\d{e}u su\'\^{a}t}} \\")
lines_table.append(r"               & T=1 & T=2 & T=8 & S(1) & S(2) & S(8) & E(1) & E(2) & E(8) \\")
lines_table.append(r"    \midrule")

for row in results:
    n = row["n"]
    times = row["times"]
    t1 = times["1"]
    t2 = times["2"]
    t8 = times["8"]
    s1 = 1.0
    s2 = t1 / t2
    s8 = t1 / t8
    e1 = s1 / 1
    e2 = s2 / 2
    e8 = s8 / 8
    n_str = f"{n:,}".replace(",", r".")  # dạng 1.000.000
    lines_table.append(
        f"    {n_str} & {t1:.4f} & {t2:.4f} & {t8:.4f}"
        f" & {s1:.2f} & {s2:.2f} & {s8:.2f}"
        f" & {e1:.2f} & {e2:.2f} & {e8:.2f} \\\\"
    )

lines_table.append(r"    \bottomrule")
lines_table.append(r"  \end{tabular}")
lines_table.append(r"\end{table}")

with open("benchmark_table.tex", "w", encoding="utf-8") as f:
    f.write("\n".join(lines_table) + "\n")
print("Xuat benchmark_table.tex OK")

# ------------------------------------------------------------------ #
# 2.  Xuat du lieu pgfplots  (speedup_data.tex)                      #
# ------------------------------------------------------------------ #
# Labels ngan cho truc x: 1M, 5M, 10M, 50M, 100M
labels = ["1M", "5M", "10M", "50M", "100M"]

# --- Speedup chart ---
lines_sp = []
lines_sp.append(r"\begin{tikzpicture}")
lines_sp.append(r"\begin{axis}[")
lines_sp.append(r"  title={Speedup theo k\'ich th\u{u}\'{o}c vector},")
lines_sp.append(r"  xlabel={K\'ich th\u{u}\'{o}c vector $n$},")
lines_sp.append(r"  ylabel={Speedup $S(p) = T_1 / T_p$},")
lines_sp.append(r"  xtick=data,")
lines_sp.append(r"  xticklabels={" + ",".join(labels) + r"},")
lines_sp.append(r"  legend pos=north west,")
lines_sp.append(r"  ymajorgrids=true,")
lines_sp.append(r"  grid style=dashed,")
lines_sp.append(r"  width=0.85\textwidth,")
lines_sp.append(r"  height=6cm,")
lines_sp.append(r"]")

for th in [2, 8]:
    coords = []
    for i, row in enumerate(results):
        t1 = row["times"]["1"]
        tp = row["times"][str(th)]
        sp = t1 / tp
        coords.append(f"({i+1},{sp:.4f})")
    style = "mark=square*" if th == 2 else "mark=triangle*"
    lines_sp.append(f"\\addplot+[{style}] coordinates {{{' '.join(coords)}}};")
    lines_sp.append("\\addlegendentry{$p=" + str(th) + "$ lu\\`{o}ng}")

# Lý thuyết lý tưởng cho p=2 và p=8 (đường nét đứt)
for th in [2, 8]:
    coords = " ".join([f"({i+1},{th})" for i in range(len(results))])
    lines_sp.append(f"\\addplot+[dashed,mark=none] coordinates {{{coords}}};")
    lines_sp.append("\\addlegendentry{L\\'{y} t\\u{u}\\'{o}ng $p=" + str(th) + "$}")

lines_sp.append(r"\end{axis}")
lines_sp.append(r"\end{tikzpicture}")

with open("speedup_chart.tex", "w", encoding="utf-8") as f:
    f.write("\n".join(lines_sp) + "\n")
print("Xuat speedup_chart.tex OK")

# --- Efficiency chart ---
lines_ef = []
lines_ef.append(r"\begin{tikzpicture}")
lines_ef.append(r"\begin{axis}[")
lines_ef.append(r"  title={Hi\d{e}u su\'\^{a}t song song},")
lines_ef.append(r"  xlabel={K\'ich th\u{u}\'{o}c vector $n$},")
lines_ef.append(r"  ylabel={Hi\d{e}u su\'\^{a}t $E(p) = S(p)/p$},")
lines_ef.append(r"  xtick=data,")
lines_ef.append(r"  xticklabels={" + ",".join(labels) + r"},")
lines_ef.append(r"  ymin=0, ymax=1.2,")
lines_ef.append(r"  legend pos=south east,")
lines_ef.append(r"  ymajorgrids=true,")
lines_ef.append(r"  grid style=dashed,")
lines_ef.append(r"  width=0.85\textwidth,")
lines_ef.append(r"  height=6cm,")
lines_ef.append(r"]")

for th in [2, 8]:
    coords = []
    for i, row in enumerate(results):
        t1 = row["times"]["1"]
        tp = row["times"][str(th)]
        sp = t1 / tp
        ep = sp / th
        coords.append(f"({i+1},{ep:.4f})")
    style = "mark=square*" if th == 2 else "mark=triangle*"
    lines_ef.append(f"\\addplot+[{style}] coordinates {{{' '.join(coords)}}};")
    lines_ef.append(f"\\addlegendentry{{$p={th}$ lu\\`{{o}}ng}}")

# Đường hiệu suất lý tưởng = 1.0
coords_ideal = " ".join([f"({i+1},1.0)" for i in range(len(results))])
lines_ef.append(f"\\addplot+[dashed,mark=none,black] coordinates {{{coords_ideal}}};")
lines_ef.append(r"\addlegendentry{L\'{\^{y}} t\u{u}\'{o}ng}")

lines_ef.append(r"\end{axis}")
lines_ef.append(r"\end{tikzpicture}")

with open("efficiency_chart.tex", "w", encoding="utf-8") as f:
    f.write("\n".join(lines_ef) + "\n")
print("Xuat efficiency_chart.tex OK")

# ------------------------------------------------------------------ #
# 3.  In so lieu ra console de kiem tra                              #
# ------------------------------------------------------------------ #
print("\n=== So lieu tong hop ===")
print(f"{'n':>12} | {'T1':>8} {'T2':>8} {'T8':>8} | {'S2':>6} {'S8':>6} | {'E2':>6} {'E8':>6}")
print("-" * 70)
for row in results:
    n = row["n"]
    t1 = row["times"]["1"]
    t2 = row["times"]["2"]
    t8 = row["times"]["8"]
    s2 = t1/t2; s8 = t1/t8
    e2 = s2/2;  e8 = s8/8
    print(f"{n:>12,} | {t1:>8.4f} {t2:>8.4f} {t8:>8.4f} | {s2:>6.3f} {s8:>6.3f} | {e2:>6.3f} {e8:>6.3f}")
