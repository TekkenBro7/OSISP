#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <chrono>
#include <atomic>


int num_readers = 5;
int num_writers = 2;
int min_read_time = 700;
int max_read_time = 900;
int min_write_time = 500;
int max_write_time = 800;
int min_delay_time = 200;
int max_delay_time = 500;
double meaningful_block_time = 0.2;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> read_dist(min_read_time, max_read_time);
std::uniform_int_distribution<> write_dist(min_write_time, max_write_time);
std::uniform_int_distribution<> delay_dist(min_delay_time, max_delay_time);

int readers_count = 0;
int writers_count = 0;
int readers_count_waiting = 0;

HANDLE resource_semaphore;
HANDLE counter_mutex;
HANDLE reader_cv, writer_cv;

std::atomic<int> successful_reads(0);
std::atomic<int> successful_writes(0);
std::atomic<int> blocked_reads(0);
std::atomic<int> blocked_writes(0);

void reader(int id)
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
        auto start_time = std::chrono::high_resolution_clock::now();

        WaitForSingleObject(counter_mutex, INFINITE);
        ++readers_count_waiting;

        if (writers_count > 0)
        {
            ReleaseMutex(counter_mutex);
            ResetEvent(reader_cv);
            WaitForSingleObject(reader_cv, INFINITE);
        }
        else
        {
            ReleaseMutex(counter_mutex);
        }

        WaitForSingleObject(counter_mutex, INFINITE);

        if (++readers_count == 1)
        {
            ReleaseMutex(counter_mutex);
            WaitForSingleObject(resource_semaphore, INFINITE);
        }
        else
        {
            ReleaseMutex(counter_mutex);
        }

        WaitForSingleObject(counter_mutex, INFINITE);
        --readers_count_waiting;
        ReleaseMutex(counter_mutex);

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> block_duration = end_time - start_time;
        if (block_duration.count() > meaningful_block_time)
        {
            blocked_reads++;
        }

        std::cout << "Читатель " << id << " читает данные.\n" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(read_dist(gen)));
        successful_reads++;

        WaitForSingleObject(counter_mutex, INFINITE);

        if (--readers_count == 0)
        {
            ReleaseSemaphore(resource_semaphore, 1, NULL);
            SetEvent(writer_cv);
        }

        ReleaseMutex(counter_mutex);

    }
}

void writer(int id)
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));

        auto start_time = std::chrono::high_resolution_clock::now();

        WaitForSingleObject(counter_mutex, INFINITE);
        writers_count++;
        ReleaseMutex(counter_mutex);

        while (readers_count > 0)
        {
            WaitForSingleObject(writer_cv, INFINITE);
            ResetEvent(writer_cv);
        }

        WaitForSingleObject(resource_semaphore, INFINITE);

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> block_duration = end_time - start_time;
        if (block_duration.count() > meaningful_block_time)
        {
            blocked_writes++;
        }

        std::cout << "Писатель " << id << " записывает данные.\n" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(write_dist(gen)));
        successful_writes++;

        WaitForSingleObject(counter_mutex, INFINITE);

        writers_count--;

        if (writers_count > 0 && readers_count_waiting < ((num_readers - 1) < 1 ? 1 : (num_readers - 1)))
        {
            SetEvent(writer_cv);
        }
        else
        {
            SetEvent(reader_cv);
        }

        ReleaseMutex(counter_mutex);
        ReleaseSemaphore(resource_semaphore, 1, NULL);
    }
}

void print_statistics()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "\n--- Статистика ------\n";
        std::cout << "Успешных чтений: " << successful_reads.load() << "\n";
        std::cout << "Блокированных чтений: " << blocked_reads.load() << "\n";
        std::cout << "Успешных записей: " << successful_writes.load() << "\n";
        std::cout << "Блокированных записей: " << blocked_writes.load() << "\n";
        std::cout << "---------------------\n";
    }
}

int main()
{
    setlocale(LC_ALL, "Russian");

    resource_semaphore = CreateSemaphore(NULL, 1, 1, NULL);
    counter_mutex = CreateMutex(NULL, FALSE, NULL);
    reader_cv = CreateEvent(NULL, TRUE, TRUE, NULL);
    writer_cv = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (resource_semaphore == NULL || counter_mutex == NULL || reader_cv == NULL || writer_cv == NULL)
    {
        std::cerr << "Не удалось создать синхронизаторы.\n";
        return 1;
    }
    ResetEvent(writer_cv);
    std::vector<std::thread> readers;
    for (int i = 0; i < num_readers; i++)
    {
        readers.emplace_back(reader, i + 1);
    }
    std::vector<std::thread> writers;
    for (int i = 0; i < num_writers; i++)
    {
        writers.emplace_back(writer, i + 1);
    }
    std::thread stats_thread(print_statistics);
    for (auto& t : readers)
    {
        t.join();
    }
    for (auto& t : writers)
    {
        t.join();
    }
    stats_thread.join();

    CloseHandle(reader_cv);
    CloseHandle(writer_cv);
    CloseHandle(resource_semaphore);
    CloseHandle(counter_mutex);

    return 0;
}