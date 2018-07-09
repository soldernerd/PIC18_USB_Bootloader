istfile = 'ist.txt'
sollfile = 'soll.txt'

def parse(filename):
    f = open(filename, 'r')
    data = {}
    for line in f:
        try:
            line = [x.strip() for x in line.split('\t')]
            address = line[2]
            opcode= line[3]
            command = line[5]
            data[address] = (opcode, command)
        except:
            pass
    return data

def compare(ist, soll, outfile):
    diffs = 0
    txt = '{0};{1};{2};{3};{4};{5}\n'
    with open(outfile, 'w') as f:
        f.write('Type;Address;IstOpcode;SollOpcode;IstCommand;SollCommand\n')
        for address in sorted(ist.keys()):
            if not address in soll:
                continue
            if not ist[address] == soll[address]:
                if not ist[address][1] == soll[address][1]:
                    line = txt.format('A', '0x'+address, '0x'+ist[address][0], '0x'+soll[address][0], ist[address][1], soll[address][1])
                    print('A', address, ist[address], soll[address])
                else:
                    line = txt.format('B', '0x'+address, '0x'+ist[address][0], '0x'+soll[address][0], ist[address][1], soll[address][1])
                    print('B', address, ist[address], soll[address])
                diffs += 1
                f.write(line)
        print('Differences:', diffs)

ist = parse(istfile)
print('Ist:', len(ist))

soll = parse(sollfile)
print('Soll:', len(soll))

compare(ist, soll, 'diff.csv')


