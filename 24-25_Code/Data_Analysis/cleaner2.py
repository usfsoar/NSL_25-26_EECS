import pandas as pd

from datetime import timedelta
from io import StringIO

input_file = "altimeter_filt.csv"
output_file = "cleaned_altitude.csv"

chunks = []
current_chunk = []
line_numbers = []
reset_id = 0
line_number = 0

with open(input_file) as f:
    for line in f:
        line_number += 1
        stripped = line.strip()
        if not stripped or not any(c.isdigit() for c in stripped):
            if current_chunk:
                chunks.append(current_chunk)
                current_chunk = []
            continue
        current_chunk.append(stripped)

if current_chunk:
    chunks.append(current_chunk)

all_data = []
time_offset = 0.0
for chunk in chunks:
    df = pd.read_csv(StringIO("\n".join(chunk)), header=None)
    df.columns = ["hours", "minutes", "seconds", "microseconds", "altitude", "temperature", "pressure"]


    df = df[df["altitude"] > 0]

    if df.empty:
        continue

    df["time"] = df.apply(
        lambda row: timedelta(
            hours=int(row["hours"]),
            minutes=int(row["minutes"]),
            seconds=int(row["seconds"]),
            microseconds=int(row["microseconds"]),
        ),
        axis=1
    )
    df["relative_time"] = df["time"].dt.total_seconds()

    df["adjusted_time_ms"] = (df["relative_time"] - df["relative_time"].iloc[0] + time_offset) * 1000

    if len(df) > 1:
        delta = df["adjusted_time_ms"].iloc[-1] - df["adjusted_time_ms"].iloc[-2]

        time_offset = df["adjusted_time_ms"].iloc[-1] / 1000 + delta / 1000
    else:
        time_offset += 1.0

    all_data.append(df[["adjusted_time_ms", "altitude", "temperature", "pressure"]])

final_df = pd.concat(all_data, ignore_index=True)

final_df.to_csv(output_file, index=False)


print(f"Cleaned altitude data saved to: {output_file}")
