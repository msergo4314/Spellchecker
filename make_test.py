import os, sys, random

NUM_WORDS : int = int(5e6) # 200_000 is the length of a long novel

# change dictionary path as needed
DICTIONARY_FILE : str = "/mnt/c/global c/dictionary.txt"
TEST_FILE_NAME : str = 'test.txt'

mistakes_1 = ('darfadf', 'raaaahd', 'blarg', 'bloog', 'xxxya')
mistakes_2 = ('farggg', 'sisgamg', 'adfqh', 'acafg', 'tttssqqd', 'oooooaaaaap', 'blurgz')

def make_test_file(name : str = TEST_FILE_NAME, dictionary_words : list[str] = None):
    if not dictionary_words:
        print("no dictionary list given")
        return
    words : list[str] = []
    for i in range(NUM_WORDS):
        num : float = random.random()
        if num < 0.001: # 0.1%
            words.append(random.choice(mistakes_1))
        elif num < 0.003: # 0.3%
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
            if (i + 1) % int(1e6) == 0:
                pass
                print(f"writing word #{i + 1:.1e}")
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