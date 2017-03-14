flash = []
with open("bq_flash_orig.txt") as f:
    for line in f:
        x = line.split(" ")
        for y in x[2:-1]:
            flash.append(int(y.strip(), 16))
with open("bq_flash_hex.txt", "wb") as f:
    f.write(bytes(flash))
