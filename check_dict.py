
def remove_duplicates_and_sort(filename : str) -> None:
    # Read the file and remove duplicate lines
    with open(filename, 'r') as file:
        lines = set(file.readlines())
        
    # Convert the set to a sorted list
    sorted_lines = sorted(lines)

    # Write the sorted list back to the file
    with open(filename, 'w') as file:
        for line in sorted_lines:
            file.write(line)
    print(f"largest word in dictionary: {max(len(x) for x in sorted_lines)}")
    
def main():
    filename = 'dictionary.txt'
    remove_duplicates_and_sort(filename)
    print("Duplicates removed and sorted successfully.")
    
if __name__ == "__main__":
    main()
