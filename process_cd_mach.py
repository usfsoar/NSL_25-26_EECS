"""
process_cd_mach.py
- Cleans OpenRocket 'Cd vs Mach' exported text (with comments)
- Detects which column is Mach vs Cd
- Filters out obviously invalid rows
- Averages duplicate Mach values (mean Cd), sorts by Mach
- Writes clean CSV "Cd_vs_Mach_openrocket_clean.csv" with header Mach,Cd
- Shows how to load it into RocketPy
"""

import re
import numpy as np
import pandas as pd
from io import StringIO

# --------- Paste your raw OpenRocket export here (or read from file) ----------
# Option A: if you already saved the export to disk, uncomment and use:
raw_text = open("cd_vs_mach.csv").read()

# Option B: paste the text directly into this triple-quoted string:
#raw_text = r"""
# mach vs cd (Up to date)
# 960 data points written for 2 variables.
# Drag coefficient (​),Mach number (​)
# (paste the rest of your file here)

# ---------------------------------------------------------------------------

# Remove comment lines and blank lines; gather numeric pairs
pairs = []
for line in raw_text.splitlines():
    line = line.strip()
    if not line or line.startswith('#'):
        continue
    # accept lines like "num,num" possibly with spaces
    m = re.match(r'^\s*([+-]?\d*\.?\d+(?:[eE][+-]?\d+)?)[,\s]+([+-]?\d*\.?\d+(?:[eE][+-]?\d+)?)\s*$', line)
    if m:
        a = float(m.group(1))
        b = float(m.group(2))
        pairs.append((a, b))
    # else ignore that line

if len(pairs) == 0:
    raise SystemExit("No numeric pairs found—make sure raw_text contains the data or read the correct file.")

arr = np.array(pairs)  # shape (N,2)

# Heuristic to decide which column is Mach and which is Cd.
# We expect: Mach typically <= 10 (usually < 3), Cd typically between ~0.01 and ~3.
col0 = arr[:, 0]
col1 = arr[:, 1]

# Compute simple stats
def plausibility_score_mach(col):
    # prefer columns with median within [0,5] and max not huge; penalize negatives
    med = np.median(col)
    mx = np.max(col)
    negs = np.sum(col < 0)
    score = 0
    if 0 <= med <= 5: score += 2
    if mx <= 50: score += 1
    score -= negs * 5
    return score

s0 = plausibility_score_mach(col0)
s1 = plausibility_score_mach(col1)

# If a column looks like Mach by score, choose accordingly. Else fallback to header info:
if s0 > s1:
    mach = col0
    cd = col1
elif s1 > s0:
    mach = col1
    cd = col0
else:
    # fallback using header hint: you told us "Drag coefficient (​),Mach number (​)" meaning first=Cd,second=Mach
    # So we swap to produce (Mach, Cd)
    mach = col1
    cd = col0

# Now filter out obviously invalid rows:
valid_mask = np.isfinite(mach) & np.isfinite(cd)
# plausible ranges
valid_mask &= (mach >= 0) & (mach <= 50)    # allow up to 50 to catch weird supersonic exports; adjust if needed
valid_mask &= (cd >= 0) & (cd <= 10)       # Cd > 10 is probably garbage
mach = mach[valid_mask]
cd = cd[valid_mask]

# If there are contiguous event blocks that create many duplicate mach values, average Cd for same Mach:
df = pd.DataFrame({"Mach": mach, "Cd": cd})
# Round Mach to a sensible precision before grouping to average near-duplicates (optional)
# e.g., round to 4 decimal places for grouping
df["Mach_r"] = df["Mach"].round(6)
df_grouped = df.groupby("Mach_r", as_index=False).Cd.mean().rename(columns={"Cd": "Cd_mean"})
# restore Mach values (use Mach_r as Mach)
df_clean = pd.DataFrame({"Mach": df_grouped["Mach_r"].values, "Cd": df_grouped["Cd_mean"].values})
df_clean = df_clean.sort_values("Mach").reset_index(drop=True)

# Optional: trim to a reasonable Mach range that your sim will cover, e.g., 0..5
max_mach_keep = 5.0
df_clean = df_clean[df_clean["Mach"] <= max_mach_keep].copy()
if df_clean.shape[0] == 0:
    raise SystemExit("No points remain after filtering. Try increasing max_mach_keep or inspect raw data.")

# Save as CSV in the format RocketPy expects: Mach,Cd
out_csv = "Cd_vs_Mach_openrocket_clean.csv"
df_clean.to_csv(out_csv, index=False, header=["Mach","Cd"])
print(f"Wrote cleaned CSV with {len(df_clean)} rows to: {out_csv}")

# Quick display of first/last rows
print(df_clean.head(8).to_string(index=False))
print("...")
print(df_clean.tail(8).to_string(index=False))

# --------- How to load into RocketPy ----------
# Option 1: pass filename to Rocket(...) as power_on_drag / power_off_drag
#
# rocket = Rocket(..., power_off_drag="Cd_vs_Mach_openrocket_clean.csv", power_on_drag="Cd_vs_Mach_openrocket_clean.csv", ...)
#
# Option 2: load to numpy array and pass it directly:
#
# import numpy as np
# power_drag = np.loadtxt("Cd_vs_Mach_openrocket_clean.csv", delimiter=',', skiprows=1)
# rocket = Rocket(..., power_off_drag=power_drag, power_on_drag=power_drag, ...)
#
# Option 3: if you want to downsample / re-interpolate to a fixed Mach grid:
#
# import numpy as np
# m = df_clean['Mach'].values
# c = df_clean['Cd'].values
# m_grid = np.linspace(m.min(), m.max(), 200)  # e.g. 200 points
# c_interp = np.interp(m_grid, m, c)
# power_drag = np.column_stack([m_grid, c_interp])
# rocket = Rocket(..., power_off_drag=power_drag, power_on_drag=power_drag, ...)
#
print("\\nNow use the CSV in your RocketPy script as shown above.")
