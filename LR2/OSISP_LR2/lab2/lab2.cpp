#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <windows.h>
#include <cstdlib>

const size_t MAX_NUM_THREADS = 16;
const size_t NUM_INTS = 1024 * 1024 * 128;

size_t NUM_THREADS;

struct ThreadDataMMap
{
	int* start;
	size_t length;
	long long partialSum;
};

struct ThreadDataTraditional
{
	int* start;
	size_t length;
	long long partialSum;
	HANDLE file;
	size_t offset;
};

int powi(int x, int power)
{
	int a = 1;
	for (int i = 0; i < power; i++)
	{
		a *= x;
	}
	return a;
}

DWORD WINAPI calculateSumMMap(LPVOID param)
{
	ThreadDataMMap* data = static_cast<ThreadDataMMap*>(param);
	for (int* current = data->start; current < data->start + data->length; current++)
	{
		*current = powi(*current, 3) - powi(*current, 2) + powi(*current, 1);
		data->partialSum += *current;
	}
	return 0;
}

DWORD WINAPI calculateSumTraditional(LPVOID param)
{
	ThreadDataTraditional* data = static_cast<ThreadDataTraditional*>(param);
	for (int* current = data->start; current < data->start + data->length; current++)
	{
		*current = powi(*current, 3) - powi(*current, 2) + powi(*current, 1);
		data->partialSum += *current;
	}
	DWORD bytesWritten;
	SetFilePointer(data->file, data->offset * sizeof(int), nullptr, FILE_BEGIN);
	WriteFile(data->file, data->start, data->length * sizeof(int), &bytesWritten, nullptr);
	return 0;
}

void fillFileWithInts(const char* filename, size_t numInts)
{
	HANDLE file = CreateFileA(
		filename,
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	std::vector<int> data(numInts);
	for (size_t i = 0; i < numInts; i++)
	{
		data[i] = rand() % 10 + 1;
	}
	DWORD bytesWritten;
	WriteFile(file, data.data(), numInts * sizeof(int), &bytesWritten, nullptr);
	CloseHandle(file);
}

void mmapAverage(const char* filename)
{
	HANDLE file = CreateFileA(
		filename,
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	LARGE_INTEGER fileSize;
	GetFileSizeEx(file, &fileSize);
	HANDLE fileMapping = CreateFileMappingA(
		file,
		nullptr,
		PAGE_READWRITE,
		0,
		0,
		nullptr);
	int* data = static_cast<int*>(MapViewOfFile(
		fileMapping,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		0));
	size_t numInts = fileSize.QuadPart / sizeof(int);
	size_t chunkSize = numInts / NUM_THREADS;
	std::vector<HANDLE> threads(NUM_THREADS);
	std::vector<ThreadDataMMap> threadData(NUM_THREADS);
	for (size_t i = 0; i < NUM_THREADS; i++)
	{
		size_t start = i * chunkSize;
		size_t end = (i == NUM_THREADS - 1) ? (numInts) : (start + chunkSize);
		threadData[i] = { data + start, end - start, 0 };
		threads[i] = CreateThread(
			nullptr,
			0,
			calculateSumMMap,
			&threadData[i],
			0,
			nullptr);
	}
	WaitForMultipleObjects(NUM_THREADS, threads.data(), TRUE, INFINITE);
	long double totalSum = 0;
	for (size_t i = 0; i < NUM_THREADS; ++i)
	{
		totalSum += threadData[i].partialSum;
		CloseHandle(threads[i]);
	}
	long double totalAverage = totalSum / numInts;

	std::cout << "Среднее значение: " << totalAverage << '\n';
	UnmapViewOfFile(data);
	CloseHandle(fileMapping);
	CloseHandle(file);
}

void traditionalAverage(const char* filename)
{
	HANDLE file = CreateFileA(
		filename,
		GENERIC_READ,
		0,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	LARGE_INTEGER fileSize;
	GetFileSizeEx(file, &fileSize);
	size_t numInts = fileSize.QuadPart / sizeof(int);
	std::vector<int> data(numInts);
	DWORD bytesRead;
	ReadFile(file, data.data(), fileSize.QuadPart, &bytesRead, nullptr);
	size_t chunkSize = numInts / NUM_THREADS;
	std::vector<HANDLE> threads(NUM_THREADS);
	std::vector<ThreadDataTraditional> threadData(NUM_THREADS);
	for (size_t i = 0; i < NUM_THREADS; i++)
	{
		size_t start = i * chunkSize;
		size_t end = (i == NUM_THREADS - 1) ? numInts : start + chunkSize;
		threadData[i] = { data.data() + start, end - start, 0, file, start };
		threads[i] = CreateThread(
			nullptr,
			0,
			calculateSumTraditional,
			&threadData[i],
			0,
			nullptr);
	}
	WaitForMultipleObjects(NUM_THREADS, threads.data(), TRUE, INFINITE);
	long double totalSum = 0;
	for (size_t i = 0; i < NUM_THREADS; i++)
	{
		totalSum += threadData[i].partialSum;
		CloseHandle(threads[i]);
	}
	long double totalAverage = totalSum / numInts;
	std::cout << "Среднее значение: " << totalAverage << std::endl;
	CloseHandle(file);
}


int main()
{
	setlocale(LC_ALL, "ru");
	time_t startTime = time(0);
	const char* filename = "largefile.dat";
	for (NUM_THREADS = 1; NUM_THREADS <= MAX_NUM_THREADS; NUM_THREADS++)
	{
		std::cout << "Число потоков: " << NUM_THREADS << '\n';
		srand(startTime);
		fillFileWithInts(filename, NUM_INTS);
		auto start = std::chrono::high_resolution_clock::now();
		traditionalAverage(filename);
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> traditionalDurational = end - start;
		std::cout << "Традиционная средняя продолжительность расчета: " << traditionalDurational.count() << " секунд\n";
		srand(startTime);
		fillFileWithInts(filename, NUM_INTS);
		start = std::chrono::high_resolution_clock::now();
		mmapAverage(filename);
		end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> mmapDuration = end - start;
		std::cout << "Отображаемая в памяти средняя продолжительность вычисления: " << mmapDuration.count() << " секунд\n";
		std::cout << '\n';
	}
	return 0;
}