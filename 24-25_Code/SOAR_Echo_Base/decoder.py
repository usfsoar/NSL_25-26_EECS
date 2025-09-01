import re
import csv

def extract_bytearray(line):
    match = re.search(r"Data Bytes: bytearray\(b'(.*?)'\)", line)
    if match:
        return match.group(1).strip('"')  # Remove quotes from the beginning and end
    return None

def is_valid_line(line):
    return "Valid data:True" in line and line.startswith("LORA:")

def is_valid_bytearray(bytearray_str):
    return bytearray_str.startswith('IS')

def main():
    with open('Payload_serial_log_03-16-2024 16-20-24.csv', 'r') as file, open('decoded_strings.csv', 'w', newline='') as output_file:
        csv_reader = csv.reader(file)
        csv_writer = csv.writer(output_file)
        for row in csv_reader:
            if len(row) >= 2 and is_valid_line(row[1]):
                bytearray_str = extract_bytearray(row[1])
                if bytearray_str and is_valid_bytearray(bytearray_str):
                    csv_writer.writerow([bytearray_str])

if __name__ == '__main__':
    main()
