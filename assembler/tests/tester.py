import json
import random
from operator import index

with open('instructions\\alu.json', 'r') as f:
    data = json.load(f)

random.seed(0)

opcodes = {
    "add":  (0b000, None),
    "sub":  (0b001, None),
    "and":  (0b010, None),
    "or":   (0b011, None),
    "xor":  (0b100, None),
    "cmp":  (0b101, None),
    "test": (0b110, None),
    "shf":  (0b111, None),
    "adc":  (0, 0b00000000),
    "inc":  (0, 0b00000001),
    "dec":  (0, 0b00000010),
    "mul":  (0, 0b00000011),
    "div":  (0, 0b00000100),
    "not":  (0, 0b00000101)
}

modes = {
    "rm_ind": 0,
    "absolute": 1,
    "rm_disp": 2,
    "reg_reg": 3,
    "sib": 0,
    "imm_reg": 1,
    "sib_disp": 2,
    "imm_mem": 3
}

registers = {
    "A": 0,
    "B": 1,
    "C": 2,
    "D": 3,
    "SI": 4,
    "DI": 5,
    "BR": 11,
    "F":  12,
    "BP": 13,
    "SP": 14,
    "IP": 15
}

def calculate_prefixes(param):
    out = []
    high_registers = ["BR", "F", "BP", "SP", "IP"]
    high_modes = ["sib", "imm_reg", "sib_disp", "imm_mem"]
    REX = 0b01000000
    AEX = 0b01010000
    OEX = 0b01100000

    if type(param[3]) == tuple: # sib or sib disp
        if param[3][0] in high_registers:
            REX |= 0b0010
        if param[3][1] in high_registers:
            REX |= 0b0001
    if param[2] in high_registers:
        REX |= 0b1000
    if param[3] in high_registers:
        REX |= 0b0100
    if REX > 0b01000000: out.append(REX)

    if param[1] in high_modes:
        AEX |= 0b1
    if AEX > 0b01010000: out.append(AEX)

    if param[4] is not None:
        if "disp" in param[1]:
            if param[4] <= -128 or param[4] >= 127:
                OEX |= 0b1
        else:
            if param[4] > 256:
                OEX |= 0b1
    if OEX > 0b01100000: out.append(OEX)

    return out

def calculate_operand_bytes(param, has_OEX):
    out = []
    modrm_modes = ["rm_ind", "absolute", "rm_disp", "reg_reg", "sib", "imm_reg", "sib_disp", "imm_mem"]

    mode = param[1]
    if mode in modrm_modes:
        if mode == "reg_reg":
            dest = registers[param[2]]
            src = registers[param[3]]
            if dest > 7: dest = (dest >> 1) & 7
            if src > 7: src = (src >> 1) & 7

            out.append(modes[mode] << 6 | (dest & 7) << 3 | (src & 7))
            return out

        if mode == "imm_reg":
            dest = registers[param[2]]
            if dest > 7: dest = (dest >> 1) & 7

            out.append(modes[mode] << 6 | (dest & 7) << 3 | (0 & 7))

            out.append(param[4] & 0xff)
            if has_OEX:
                out.append((param[4] >> 8) & 0xff)

            return out

        if mode == "imm_mem":
            dest = registers[param[2]]
            if dest > 7: dest = (dest >> 1) & 7

            out.append(modes[mode] << 6 | (0 & 7) << 3 | (dest & 7))

            out.append(param[4] & 0xff)
            if has_OEX:
                out.append((param[4] >> 8) & 0xff)

            return out

        if mode == "absolute":
            dest = registers[param[2]]
            if dest > 7: dest = (dest >> 1) & 7

            out.append(modes[mode] << 6 | (dest & 7) << 3 | (0 & 7))

            out.append(param[4] & 0xff)
            if has_OEX:
                out.append((param[4] >> 8) & 0xff)

            return out

        if mode in ["rm_ind", "rm_disp"]:
            dest = registers[param[2]]
            src = registers[param[3]]
            if dest > 7: dest = (dest >> 1) & 7
            if src > 7: src = (src >> 1) & 7

            out.append(modes[mode] << 6 | (dest & 7) << 3 | (src & 7))
        elif mode in ["sib", "sib_disp"]:
            dest =registers[param[2]]
            sib = param[3]
            idx = registers[sib[0]]
            base = registers[sib[1]]
            if dest > 7: dest = (dest >> 1) & 7
            if idx > 7: idx = (idx >> 1) & 7
            if base > 7: base = (base >> 1) & 7

            out.append(modes[mode] << 6 | (dest & 7) << 3 | (0 & 7))
            out.append(sib[2] << 6 | (idx & 7) << 3 | (base & 7))

        if mode in ["rm_disp", "sib_disp"]:
            out.append(param[4] & 0xff)
            if has_OEX:
                out.append((param[4] >> 8) & 0xff)

    return out

def calculate_expected_bytes(param, op_class, directionality):
    out = []
    has_oex = 0

    out.extend(calculate_prefixes(param))

    for byte in out:
        if (byte & 0xf0) == 0x60: has_oex = 1

    opc = opcodes[str(param[0]).lower()]
    if opc[1] is not None:
        out.append(0b10000000 | (directionality << 5) | (op_class << 3) | opc[0])
        out.append(opc[1])
    else:
        out.append(0 | (directionality << 5) | (op_class << 3) | opc[0])

    out.extend(calculate_operand_bytes(param, has_oex))

    return out

def generate_parameters(spec):
    params = []
    registers = spec["registers"]
    instr = spec["instruction"]

    for mode in spec["mode"]:
        if mode == "reg_reg":
            for dst in registers:
                for src in registers:
                    params.append((instr, mode, dst, src, None))
        elif mode == "rm_ind":
            for dst in registers:
                for src in registers:
                    params.append((instr, mode, dst, src, None))
        elif mode == "rm_disp":
            for dst in registers:
                for src in registers:
                    disp = random.randint(-32768, 32767)
                    params.append((instr, mode, dst, src, disp))
        elif mode == "absolute":
            for dst in registers:
                addr = random.randint(0, 65535)
                params.append((instr, mode, dst, None, addr))
        elif mode == "sib":
            for dst in registers:
                for src in registers:
                    for base in registers:
                        for scale in [1,2,4,8]:
                            params.append((instr, mode, dst, (src, base, scale), None))
        elif mode == "sib_disp":
            for dst in registers:
                for src in registers:
                    for base in registers:
                        for scale in [1,2,4,8]:
                            disp = random.randint(-32768, 32767)
                            params.append((instr, mode, dst, (src, base, scale), disp))
        elif mode == "imm_reg":
            for dst in registers:
                imm = random.randint(0, 65535)
                params.append((instr, mode, dst, None, imm))
        elif mode == "imm_mem":
            for dst in registers:
                imm = random.randint(0, 65535)
                params.append((instr, mode, dst, None, imm))
    return params

all_params = []
expected_bytes = []
for spec in data.values():
    all_params.extend([generate_parameters(spec)])

for spec in all_params:
    for param in spec:
        tmp = [calculate_expected_bytes(param, 0, 0), calculate_expected_bytes(param, 0, 1)]
        expected_bytes.append(tmp)

