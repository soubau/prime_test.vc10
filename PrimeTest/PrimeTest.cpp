// PrimeTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

//#define _HAS_ITERATOR_DEBUGGING 0

#include <vector>
#include <iostream>

#include <Windows.h>

// Utilities

__int64 intSqrt(__int64 n)
{
	return static_cast<__int64>(sqrt(static_cast<double>(n)));
}

class Timer
{
public:
	Timer(char *msg_):msg(msg_), start(GetTickCount()) { }
	~Timer() { std::cout << msg << ": " << GetTickCount() - start << "\n"; }
private:
	DWORD start;
	const char *msg;
};

// First solution

bool IsPrime(__int64 n)
{
#ifdef TEST_SLOW_CONDITION
	Sleep(1);
#endif
	__int64 sqrt_n = intSqrt(n);
	for (__int64 i = 2; i <= sqrt_n; ++i)
	{
		if (n % i == 0)
			return false;
	}
	return true;
}

__int64 FindPrimes_v1(__int64 max)
{
	std::vector<__int64> result;
	__int64 count = 0;
	for (__int64 n = 2; n <= max; ++n)
	{
		if (IsPrime(n))
		{
			result.push_back(n);
		}
	}
	
	return result.size();
}

// First solution in multiple threads

const __int64 cpuCount = 8;
size_t minList = 10 * 1000;
size_t maxList = 20 * 1000;

CRITICAL_SECTION m;
CONDITION_VARIABLE cv_needMore, cv_handleNow;
std::vector<__int64> vector;
std::vector<__int64> result_array[cpuCount];
__int64 time_array[cpuCount];
__int64 endNumber;
bool done;

HANDLE *threads;

DWORD WINAPI FindPrimes_Thread(LPVOID index)
{
	__int64 threadIndex = reinterpret_cast<__int64>(index);
	while (1)
	{
		__int64 n;
		{
			EnterCriticalSection(&m);
			while (vector.empty() && !done)
			{
				SleepConditionVariableCS(&cv_handleNow, &m, INFINITE);
			}
			if (vector.empty() && done)
			{
				LeaveCriticalSection(&m);
				return 0;
			}
			n = vector.back();
			vector.pop_back();
			if (vector.size() <= minList)
			{
				WakeConditionVariable(&cv_needMore);
			}
			LeaveCriticalSection(&m);
		}
		DWORD t = GetTickCount();
		if (IsPrime(n))
		{
			result_array[threadIndex].push_back(n);
		}
		time_array[threadIndex] += GetTickCount() - t;
	}

	return 0;
}

__int64 FindPrimes_v2(__int64 max)
{
	InitializeCriticalSection(&m);
	InitializeConditionVariable(&cv_needMore);
	InitializeConditionVariable(&cv_handleNow);

	done = false;

	{
	Timer t("queue");

	for (__int64 n = 2; n <= max; ++n)
	{
		EnterCriticalSection(&m);
//		while (vector.size() >= maxList)
		{
//			SleepConditionVariableCS(&cv_needMore, &m, INFINITE);
		}
		vector.push_back(n);
		if (vector.size() >= minList)
		{
//			WakeConditionVariable(&cv_handleNow);
		}
		LeaveCriticalSection(&m);
	}

	}

	threads = new HANDLE[cpuCount];
	for (__int64 i = 0; i < cpuCount; ++i)
	{
		threads[i] = CreateThread(NULL, 0, FindPrimes_Thread, (LPVOID)i, 0, NULL);
	}

	EnterCriticalSection(&m);
	done = true;
	LeaveCriticalSection(&m);
	WakeAllConditionVariable(&cv_handleNow);

	__int64 total = 0;
	for (__int64 i = 0; i < cpuCount; ++i)
	{
		WaitForSingleObject(threads[i], INFINITE);
		std::cout << "    count: " << result_array[i].size() << "\t";
		std::cout << "    time: " << time_array[i] << "\n";
		total += result_array[i].size();

		result_array[i].clear();
		time_array[i] = 0;
	}
	delete [] threads;

	return total;
}

// First solution in multiple threads #2

DWORD WINAPI FindPrimes_Thread_v3(LPVOID index)
{
	__int64 threadIndex = reinterpret_cast<__int64>(index);
	for (__int64 n = 2; n <= endNumber; ++n)
	{
		if ((n - 2) / maxList % cpuCount != threadIndex)
			continue;

		if (IsPrime(n))
		{
			result_array[threadIndex].push_back(n);
		}
	}

	return 0;
}

__int64 FindPrimes_v3(__int64 max)
{
	endNumber = max;
	threads = new HANDLE[cpuCount];
	for (__int64 i = 0; i < cpuCount; ++i)
	{
		threads[i] = CreateThread(NULL, 0, FindPrimes_Thread_v3, (LPVOID)i, 0, NULL);
	}

	__int64 total = 0;
	for (__int64 i = 0; i < cpuCount; ++i)
	{
		WaitForSingleObject(threads[i], INFINITE);
		total += result_array[i].size();

		result_array[i].clear();
		time_array[i] = 0;
	}
	delete [] threads;

	return total;
}

// Different solutoin

__int64 FindPrimes_v4(__int64 max)
{
	std::vector<__int64> result;

	bool isPrime;
	for (__int64 n = 2; n <= max; ++n)
	{
		DWORD t = GetTickCount();
		isPrime = true;
		__int64 sqrt_n = intSqrt(n);
		for (auto it = begin(result); it != end(result) && *it <= sqrt_n; ++it)
		{
			if (n % *it == 0)
			{
				isPrime = false;
				break;
			}
		}
		if (isPrime)
		{
			result.push_back(n);
		}
	}

	return result.size();
}

// Different solution in multiple threads

std::vector<__int64> primes;

void FindPrimes_Base_v5(__int64 max)
{
	primes.clear();

	bool isPrime;
	for (__int64 n = 2; n <= max; ++n)
	{
		isPrime = true;
		__int64 sqrt_n = intSqrt(n);
		for (auto it = begin(primes); it != end(primes) && *it <= sqrt_n; ++it)
		{
			if (n % *it == 0)
			{
				isPrime = false;
				break;
			}
		}
		if (isPrime)
		{
			primes.push_back(n);
		}
	}
}

bool IsPrime_withBase_v5(__int64 n)
{
	__int64 sqrt_n = intSqrt(n);
	for (auto it = begin(primes); it != end(primes) && *it <= sqrt_n; ++it)
	{
		if (n % *it == 0)
		{
			return false;
		}
	}
	return true;
}

void FindPrimes_Thread_Block_v5(__int64 blockCount, __int64 saveIndex, __int64 blockIndex)
{
	__int64 start_ = (endNumber / blockCount * blockIndex) + 1;
	__int64 end_ = endNumber / blockCount * (blockIndex + 1);
	for (__int64 n = max(2, start_); n <= end_; ++n)
	{
		if (IsPrime_withBase_v5(n))
		{
			result_array[saveIndex].push_back(n);
		}
	}
}

DWORD WINAPI FindPrimes_Thread_v5(LPVOID index)
{
	__int64 threadIndex = (__int64)index;
	__int64 blockCount = cpuCount * 2;
	FindPrimes_Thread_Block_v5(blockCount, threadIndex, threadIndex);
	FindPrimes_Thread_Block_v5(blockCount, threadIndex, (blockCount - 1) - threadIndex);
	return 0;
}

__int64 FindPrimes_v5(__int64 max)
{
	FindPrimes_Base_v5(intSqrt(max));
	endNumber = max;

	threads = new HANDLE[cpuCount];
	for (__int64 i = 0; i < cpuCount; ++i)
	{
		threads[i] = CreateThread(NULL, 0, FindPrimes_Thread_v5, (LPVOID)i, 0, NULL);
	}

	__int64 total = 0;
	for (__int64 i = 0; i < cpuCount; ++i)
	{
		WaitForSingleObject(threads[i], INFINITE);
		CloseHandle(threads[i]);
		total += result_array[i].size();

		result_array[i].clear();
		time_array[i] = 0;
	}

	primes.clear();
	delete [] threads;

	return total;
}

//

bool IsPrime_Different(__int64 n)
{
	__int64 sqrt_n = intSqrt(n);
	for (auto it = begin(primes); it != end(primes) && *it <= sqrt_n; ++it)
	{
		if (n % *it == 0)
		{
			return false;
		}
	}
	return true;
}

DWORD WINAPI FindPrimes_Thread_v6(LPVOID index)
{
	__int64 threadIndex = reinterpret_cast<__int64>(index);
	while (1)
	{
		__int64 n;
		{
			EnterCriticalSection(&m);
			while (vector.empty() && !done)
			{
				SleepConditionVariableCS(&cv_handleNow, &m, INFINITE);
			}
			if (vector.empty() && done)
			{
				LeaveCriticalSection(&m);
				return 0;
			}
			n = vector.back();
			vector.pop_back();
			if (vector.size() <= minList)
			{
				WakeConditionVariable(&cv_needMore);
			}
			LeaveCriticalSection(&m);
		}
		DWORD t = GetTickCount();
		if (IsPrime_Different(n))
		{
			result_array[threadIndex].push_back(n);
		}
		time_array[threadIndex] += GetTickCount() - t;
	}

	return 0;
}

__int64 FindPrimes_v6(__int64 max)
{
	InitializeCriticalSection(&m);
	InitializeConditionVariable(&cv_needMore);
	InitializeConditionVariable(&cv_handleNow);

	FindPrimes_Base_v5(intSqrt(max));

	done = false;
	threads = new HANDLE[cpuCount];
	for (__int64 i = 0; i < cpuCount; ++i)
	{
		threads[i] = CreateThread(NULL, 0, FindPrimes_Thread_v6, (LPVOID)i, 0, NULL);
	}

	for (__int64 n = 2; n <= max; ++n)
	{
		EnterCriticalSection(&m);
		while (vector.size() >= maxList)
		{
			SleepConditionVariableCS(&cv_needMore, &m, INFINITE);
		}
		vector.push_back(n);
		if (vector.size() >= minList)
		{
			WakeConditionVariable(&cv_handleNow);
		}
		LeaveCriticalSection(&m);
	}

	EnterCriticalSection(&m);
	done = true;
	LeaveCriticalSection(&m);
	WakeAllConditionVariable(&cv_handleNow);

	__int64 total = 0;
	for (__int64 i = 0; i < cpuCount; ++i)
	{
		WaitForSingleObject(threads[i], INFINITE);
		std::cout << "    count: " << result_array[i].size() << "\t";
		std::cout << "    time: " << time_array[i] << "\n";
		total += result_array[i].size();

		result_array[i].clear();
		time_array[i] = 0;
	}
	primes.clear();
	delete [] threads;

	return total;
}

// main

void main()
{
	__int64 MAX, COUNT;

	MAX = 1000000000000;	COUNT = 664579;
	{ Timer t("10 million - use list no lock");  std::cout << FindPrimes_v5(MAX); }
	{ Timer t("10 million - use list");          std::cout << FindPrimes_v4(MAX); }
	{ Timer t("10 million - no lock");           std::cout << FindPrimes_v3(MAX); }
	{ Timer t("10 million - multi");             std::cout << FindPrimes_v2(MAX); }
	{ Timer t("10 million - use list multi");    std::cout << FindPrimes_v6(MAX); }
	{ Timer t("10 million - single");            std::cout << FindPrimes_v1(MAX); }
	std::cout << "\n";

	/*
	MAX = 100 * 1000 * 1000; COUNT = 5761455;
	{ Timer t("100 million - single");           if (FindPrimes_v1(MAX) != COUNT) DebugBreak(); }
	{ Timer t("100 million - multi");            if (FindPrimes_v2(MAX) != COUNT) DebugBreak(); }
	{ Timer t("100 million - no lock");          if (FindPrimes_v3(MAX) != COUNT) DebugBreak(); }
	{ Timer t("100 million - use list");         if (FindPrimes_v4(MAX) != COUNT) DebugBreak(); }
	{ Timer t("100 million - use list no lock"); if (FindPrimes_v5(MAX) != COUNT) DebugBreak(); }
	{ Timer t("100 million - use list multi");   if (FindPrimes_v6(MAX) != COUNT) DebugBreak(); }
	std::cout << "\n";

	MAX = 1000 * 1000 * 1000; COUNT = 50847534;
	{ Timer t("1 billion - single");             if (FindPrimes_v1(MAX) != COUNT) DebugBreak(); }
	{ Timer t("1 billion - multi");              if (FindPrimes_v2(MAX) != COUNT) DebugBreak(); }
	{ Timer t("1 billion - no lock");            if (FindPrimes_v3(MAX) != COUNT) DebugBreak(); }
	{ Timer t("1 billion - use list");           if (FindPrimes_v4(MAX) != COUNT) DebugBreak(); }
	{ Timer t("1 billion - use list no lock");   if (FindPrimes_v5(MAX) != COUNT) DebugBreak(); }
	{ Timer t("1 billion - use list multi");     if (FindPrimes_v6(MAX) != COUNT) DebugBreak(); }
	std::cout << "\n";
	*/
}
