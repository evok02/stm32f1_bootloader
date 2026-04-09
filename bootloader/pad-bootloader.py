BOOT_LOADER_SIZE = 0x8000
BOOT_LOADER_FILE = "./bootloader.bin"

with open(BOOT_LOADER_FILE, "rb") as f:
    raw_file = f.read()

bytes_to_pad = BOOT_LOADER_SIZE - len(raw_file)
padding = bytes([0xff for _ in range(bytes_to_pad)])

with open(BOOT_LOADER_FILE, "wb") as f:
    f.write(raw_file + padding)
