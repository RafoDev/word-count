import random
from nltk.stem import PorterStemmer
from nltk.corpus import words


stemmer = PorterStemmer()

nltk_words = words.words()

def generate_random_chunk(chunk_size):
    random_chunk = []
    for _ in range(chunk_size):
        random_word = random.choice(nltk_words)
        stemmed_word = stemmer.stem(random_word)
        random_chunk.append(stemmed_word)
    return ' '.join(random_chunk)

chunk_size = 100000

total_file_size_bytes = (1024 * 1024 * 1024)/16

output_filename = 's27.txt'
with open(output_filename, 'w') as file:
    bytes_written = 0
    while bytes_written < total_file_size_bytes:
        chunk = generate_random_chunk(chunk_size)
        bytes_written += file.write(chunk)


print("Listo...")