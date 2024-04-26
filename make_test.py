import os, sys, random

# change dictionary path as needed
DICTIONARY_FILE : str = "/mnt/c/global c/dictionary.txt"
TEST_FILE_NAME : str = 'test.txt'
NUM_ENTRIES : int = int(1e7) #150 000 is typical novel length

mistakes_1 = ('darfadf', 'raaaahd', 'blarg', 'bloog', 'foo', 'bar', 'xxxya')
mistakes_2 = ('farggg', 'sisgamg', 'adfqh', 'acafg', 'tttssqqd', 'oooooaaaaap', 'blurgz')

def make_test_file(name : str = TEST_FILE_NAME, dictionary_words : list[str] = None):
    if not dictionary_words:
        print("no dictionary list given")
        return
    words : list[str] = []
    for i in range(NUM_ENTRIES):
        num : float = random.random()
        if num < 0.03: # 3%
            words.append(random.choice(mistakes_1))
        elif num < 0.05: # 5%
            words.append(random.choice(mistakes_2))
        else:
            # a valid word is used in this case
            words.append(random.choice(dictionary_words))
    
    with open(name, 'w') as file:
        for i, word in enumerate(words):
            if (i + 1) % 15 == 0:
                file.write(f'{word}\n')
            else:
                file.write(f'{word} ')
                
def main() -> int:
    if not os.path.exists(DICTIONARY_FILE):
        print(f"dictionary at location {DICTIONARY_FILE} not found. Exiting")
        sys.exit()
    with open(DICTIONARY_FILE, 'r') as file:
        dictionary_words = [word for word in file.read().split()]
    make_test_file(dictionary_words=dictionary_words)
    return 0

if __name__ == "__main__":
    main()