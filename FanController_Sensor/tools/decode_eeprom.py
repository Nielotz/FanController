import sys


def idx_to_min(index):
    if index < 30:
        return 1 + index
    if index < 50:
        return 32 + (index - 30) * 2
    return 74 + (index - 50) * 4


def unpack(word):
    return ((word >> 10) & 0x3F) + 20, ((word >> 5) & 0x1F) + 20, (word & 0x1F) + 20


def packed_is_valid(word):
    return ((word >> 10) & 0x3F) <= 60 and ((word >> 5) & 0x1F) <= 20 and (word & 0x1F) <= 15


bytes_ = []
for line in sys.stdin:
    line = line.strip()
    if not line or line in ("EEPROM:512", "END"):
        continue
    bytes_.extend(int(value, 16) for value in line.split())


print("idx,elapsed_min,t1_c,t2_c,t3_c")
for index in range(0, len(bytes_) - 1, 2):
    word = bytes_[index] | (bytes_[index + 1] << 8)
    record_index = index // 2
    if word == 0xFFFF or not packed_is_valid(word):
        break

    t1, t2, t3 = unpack(word)
    print(f"{record_index},{idx_to_min(record_index)},{t1},{t2},{t3}")