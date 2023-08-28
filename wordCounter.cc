#include <iostream>
#include <utility>
#include <fstream>
#include <string>
#include <filesystem>
#include <math.h>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>

using namespace std;

const int nThreads = 10;
const int maxGbs = 10;
const int maxRam = 10;
typedef long long ll;

ifstream filePointer;
vector<unordered_map<string, int>> counter;
string filename;

struct Indexes
{
	ll begin;
	ll end;
	int mapIndex;
	Indexes()
	{
		begin = -1;
		end = -1;
	}
};

ll getFileSize() {
    ifstream file(filename, ios::binary | ios::ate);
    
    if (!file.is_open()) {
        return -1;
    }

    ll fileSize = file.tellg();
    file.close();

    return fileSize;
}

ll getValidEndOfWord(ifstream &filePointer, ll endPos)
{
	char tmpChar;
	filePointer.seekg(endPos, filePointer.beg);
	while (filePointer.get(tmpChar))
	{
		if (tmpChar == ' ')
			return endPos;
		endPos++;
	}
	return endPos;
}

void splitFileInChunks(ifstream &filePointer, ll &fileSize, int currGbs, vector<Indexes> &chunkBoundaries)
{
	ll numberOfChunks = ll(ceil(float(currGbs) / maxRam));
	ll chunkSize = ll(ceil(float(fileSize) / numberOfChunks));
	cout<<numberOfChunks<<" "<<chunkSize<<'\n';
	chunkBoundaries.resize(numberOfChunks);

	for (ll i = 0, j = 0; i < numberOfChunks; i++, j++)
	{
		cout<<j<<" ";
		chunkBoundaries[i].begin = j;
		ll tmpEndOfWord = j + chunkSize - 1;
		ll validEndOfWord = getValidEndOfWord(filePointer, tmpEndOfWord);
		chunkBoundaries[i].end = validEndOfWord;
		j = validEndOfWord;
		cout<<j<<" \n";
	}

	chunkBoundaries[numberOfChunks - 1].end = fileSize - 1;
}

void splitChunkInSlices(ifstream &filePointer, Indexes &chunk, vector<Indexes> &sliceBoundaries)
{
	ll chunkSize = chunk.end - chunk.begin + 1;
	cout << "chunkSize: " << chunkSize << '\n';

	int numberOfSlices = nThreads;
	int sliceSize = ll(ceil(float(chunkSize) / numberOfSlices));
	// sliceBoundaries.reserve(numberOfSlices);

	cout << "sliceSize: " << sliceSize << '\n';

	bool endReached = false;

	for (ll i = 0, j = chunk.begin; i < numberOfSlices && !endReached; i++, j++)
	{
		Indexes sliceBoundary;
		sliceBoundary.begin = j;
		ll tmpEndOfWord = j + sliceSize - 1;
		ll validEndOfWord = getValidEndOfWord(filePointer, tmpEndOfWord);
		if (validEndOfWord > chunk.end)
		{
			validEndOfWord = chunk.end;
			endReached = true;
			numberOfSlices = i + 1;
		}
		sliceBoundary.end = validEndOfWord;

		j = sliceBoundary.end;
		sliceBoundaries.push_back(sliceBoundary);
	}

	sliceBoundaries[numberOfSlices - 1].end = chunk.end;
	cout << "chunk splitted\n";
}

void *countSliceWorker(void *arg)
{
	struct Indexes *args = (struct Indexes *)arg;
	cout << "worker: " << args->begin << " " << args->end << "\n";

	ifstream partialPointer;
	partialPointer.open(filename, ios::binary);
	partialPointer.seekg(args->begin, partialPointer.beg);

	string currWord;
	for (ll pos = args->begin; pos <= args->end; pos++)
	{
		char tmpChar;
		partialPointer.get(tmpChar);

		if (tmpChar != ' ')
		{
			currWord += tmpChar;
		}
		else if (tmpChar == ' ' && !currWord.empty())
		{
			counter[args->mapIndex][currWord]++;
			currWord.clear();
		}
	}
	if (!currWord.empty())
		counter[args->mapIndex][currWord]++;

	return NULL;
}

int main()
{
	filename = "samples/1gb.txt";

	ll fileSize;
	int currGbs;

	int numberOfChunks;
	ll chunkSize;
	vector<Indexes> chunkBoundaries;

	int numberOfSlices;
	ll sliceSize;
	vector<Indexes> sliceBoundaries;

	filePointer.open(filename, ios::binary);
	fileSize = getFileSize();

	auto start = chrono::high_resolution_clock::now();

	cout << "filesize: " << fileSize << '\n';

	currGbs = float(fileSize) / 1073741824;

	if (currGbs > maxGbs)
	{
		cout << "more than 10gbs\n";
		splitFileInChunks(filePointer, fileSize, currGbs, chunkBoundaries);
		numberOfChunks = chunkBoundaries.size();
	}
	else
	{
		cout << "less than 10gbs\n";
		Indexes fileIndexes;
		fileIndexes.begin = 0;
		fileIndexes.end = fileSize - 1;
		numberOfChunks = 1;

		chunkBoundaries.push_back(fileIndexes);
	}

	pthread_t threads[nThreads];
	for (int i = 0; i < numberOfChunks; i++)
	{
		cout << "chunk: " << i << '\n';
		splitChunkInSlices(filePointer, chunkBoundaries[i], sliceBoundaries);
		int numOfSlices = sliceBoundaries.size();
		counter.resize(numOfSlices);

		for (int j = 0; j < numOfSlices; j++)
		{
			sliceBoundaries[j].mapIndex = j;
			pthread_create(&threads[j], NULL, countSliceWorker, (void *)&sliceBoundaries[j]);
		}
		for (int j = 0; j < numOfSlices; j++)
			pthread_join(threads[j], NULL);
	}

	cout << "counter:\n";

	unordered_map<string, int> mergedCounter;

	for (const auto &map : counter)
	{
		for (const auto &entry : map)
			mergedCounter[entry.first] += entry.second;
	}

	auto end = chrono::high_resolution_clock::now();

	for (const auto &entry : mergedCounter)
		cout << entry.first << ": " << entry.second << endl;

	cout<<"number of different words: "<<mergedCounter.size()<<'\n'; 

	chrono::duration<double> duration = end - start;
	cout << "Time elapsed: " << duration.count() << " seconds" << '\n';
}