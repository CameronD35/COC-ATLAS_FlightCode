import json
import pandas as pd
from tabulate import tabulate

filename = "pharosData.jsonl"

data = []

with open(filename, 'r') as f:
	for line in f:
		if line.strip():
			data.append(json.loads(line))

df = pd.DataFrame(data)

print(tabulate(df, headers='keys', tablefmt='psql', showindex=False))