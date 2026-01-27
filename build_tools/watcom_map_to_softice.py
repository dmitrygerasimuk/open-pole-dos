
import re

def parse_open_watcom_map(map_text):
    lines = map_text.splitlines()
    segments = []
    symbols = []
    current_section = None

    for line in lines:
       
        if "Segment" in line and "Class" in line and "Address" in line:
            current_section = 'segments'
            continue
        elif "Address" in line and "Symbol" in line:
            current_section = 'symbols'
            continue
        elif not line.strip():
            continue

        if current_section == 'segments':
            match = re.match(r"(\S+)\s+(\S+)\s+(\S+)\s+([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})\s+([0-9A-Fa-f]{8})", line)
            if match:
                name, class_name, group, seg, offset, size = match.groups()
                seg_start = int(seg, 16) * 16 + int(offset, 16)
                seg_size = int(size, 16)
                seg_end = seg_start + seg_size - 1
                segments.append({
                    'name': name,
                    'class': class_name,
                    'start': seg_start,
                    'end': seg_end,
                    'length': seg_size
                })

        elif current_section == 'symbols':
            match = re.match(r"([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})(\+?)\s+(\S+)", line)
            if match:
                seg, offset, local_flag, name = match.groups()
                if True:
                    address = f"{seg}:{offset}"
                    symbols.append((address, name))

    return segments, symbols

def format_segment_line(start, end, name, class_name):
    length = end - start + 1
    return f"{start:04X} {end:04X} {length:04X} {name:<20} {class_name}"

def format_symbol_line(address, name):
    return f" {address:<15} {name}"

def convert_to_softice_map(map_text):
    segments, symbols = parse_open_watcom_map(map_text)

   
    segments.sort(key=lambda x: x['start'])

    output_lines = []

    output_lines.append(" Start  Stop   Length Name               Class\r\n")
    output_lines.append("\r\n")

    for i, seg in enumerate(segments):
        line = format_segment_line(seg['start'] >> 4, seg['end'] >> 4, f"seg{i:03}", seg['class'])
        output_lines.append(line + "\r\n")

    output_lines.append("\r\n\r\n Address         Publics by Value\r\n")
    output_lines.append("\r\n")

    for addr, name in symbols:
        output_lines.append(format_symbol_line(addr, name) + "\r\n")
 
    with open("softice.map", "w", encoding="utf-8") as f:
        f.writelines(output_lines)

 
if __name__ == "__main__":
    with open("openpole.map", "r", encoding="utf-8") as f:
        map_text = f.read()
    convert_to_softice_map(map_text)
    print("Файл softice.map создан.")
