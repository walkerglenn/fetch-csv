# Create a .csv file with argument-specified dimensions.
# Each value is a randomly generated int between 1 and 999,000

import sys
import random

# TODO: Create default args if none supplied, or maybe fail (with printed debug)
num_rows = int(sys.argv[1])
num_cols = int(sys.argv[2])

csv_rows = []

for i in range(0, num_rows):
    #csv_rows.append(((str(random.randint(1, 1_000_000)) + ',') * num_cols)[:-1] + '\n')
    csv_rows.append(((f'Row: {i},') * num_cols)[:-1] + '\n')
        
with open('build/Test.csv', 'w') as file:
    file.writelines(csv_rows)
