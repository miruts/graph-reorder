average = 0.0
with open("temp.txt") as fin:
	line = fin.readline()
	total, count = 0.0, 0
	while line:
		line = fin.readline()
		d = float(line)
		total += d
		count += 1
		line = fin.readline()
	average = total/count if count else 0
with open("temp.txt", "w") as fout:
	fout.write("{}".format(average*1000))


