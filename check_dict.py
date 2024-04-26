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

# Test the function with a sample file
filename = 'dictionary.txt'  # Change this to your file path
remove_duplicates_and_sort(filename)
print("Duplicates removed and sorted successfully.")
