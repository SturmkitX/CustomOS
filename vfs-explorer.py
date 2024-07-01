import struct
import math

files = list()
entry_format = '<IIII16s'   # 4 dwords, 16 bytes (total 32 bytes, little endian)
entry_size = struct.calcsize(entry_format)

def get_files(hdd):
    with open(hdd, 'rb') as f:
        f.seek(10000 * 512, 0)      # headers start at sector 10000
        while (len(files) < 1024):
            print('Current file location', f.tell())
            (start, sb, ss, curr, name) = struct.unpack(entry_format, f.read(entry_size))
            if name[0] == 0:
                print('Reached end of files')
                break
            files.append({
                'start_sector': start,
                'size_bytes': sb,
                'size_sectors': ss,
                'current': curr,
                'name': name
            })
    
    # Print files
    for file in files:
        print("[%d] Name: %s, Size: %d (%d blocks)" % (files.index(file), file['name'].decode('utf-8'), file['size_bytes'], file['size_sectors']))

def put_file(hdd, target, newName = None):
    content = b''
    with open(target, 'rb') as ft:
        content = ft.read()
    
    filename = target[:16]
    if newName is not None:
        filename = newName[:16]

    with open(hdd, 'r+b') as f:
        entry = {
            'start_sector': 10000 + entry_size * 2 + len(files) * 16000,
            'size_bytes': len(content),
            'size_sectors': math.ceil(len(content) / 512),
            'current': 0,
            'name': filename.encode()
        }

        f.seek(10000 * 512 + len(files) * entry_size, 0)      # headers start at sector 10000
        f.write(struct.pack(entry_format, entry['start_sector'], entry['size_bytes'], entry['size_sectors'], entry['current'], entry['name']))

        files.append(entry)

        remBytes = entry['size_bytes'] - 16000 * 512
        while (remBytes > 0):
            entry2 = {
                'start_sector': entry['start_sector'],
                'size_bytes': 0,
                'size_sectors': 0,
                'current': 0,
                'name': filename.encode()
            }

            f.seek(10000 * 512 + len(files) * entry_size, 0)      # headers start at sector 10000
            f.write(struct.pack(entry_format, entry2['start_sector'], entry2['size_bytes'], entry2['size_sectors'], entry2['current'], entry2['name']))

            files.append(entry2)
            remBytes -= 16000 * 512


        f.seek(entry['start_sector'] * 512, 0)
        f.write(content)
    
    # Print files
    for file in files:
        print("[%d] Name: %s, Size: %d (%d blocks)" % (files.index(file), file['name'], file['size_bytes'], file['size_sectors']))


def extract_file(hdd, filename, targetname = None):
    readb = b''
    file = None
    for x in files:
        name = x['name'].decode('utf-8').strip()
        namelen = min(len(name), len(filename))
        if name[:namelen] == filename[:namelen]:
            file = x
            break       # we may have duplicates, ignore them for now

    if file is None:
        print("File not found")
        exit(1)

    with open(hdd, 'rb') as ft:
        ft.seek(file['start_sector'] * 512)
        readb = ft.read(file['size_bytes'])

        if (len(readb) < file['size_bytes']):
            print("WARNING: Read Bytes size is smaller than file size; Read Bytes {}, File size: {}" % (len(readb), file['size_bytes']))

    if (targetname is None or len(targetname) == 0):
        targetname = filename

    with open(targetname, 'wb') as fo:
        fo.write(readb)

    print("File extracted successfully")


if __name__ == '__main__':
    print("Format size = ", entry_size)
    get_files('hdda.img')
    # put_file('hdda.img', 'udp-server.py')
    # put_file('hdda.img', 'pic-loading.raw')
    # put_file('hdda.img', 'DOOM.WAD', 'WADS/doom.wad')
    # extract_file('hdda.img', 'e13.raw')
