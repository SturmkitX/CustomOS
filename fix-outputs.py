lines = list()

with open('../doom-good.txt', 'r+') as f:
    for line in f.readlines():
        line = line[:8]
        if line[-1] != '\n':
            line += '\n'
        lines.append(line)
    f.seek(0, 0)
    f.truncate(0)
    f.writelines(lines)

