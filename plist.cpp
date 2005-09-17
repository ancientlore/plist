#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>

const _TCHAR* match(const _TCHAR* one, const _TCHAR* two)
{
	TCHAR ONE[MAX_PATH];
	TCHAR TWO[MAX_PATH];
	_tcsncpy(ONE, one, MAX_PATH - 1);
	ONE[MAX_PATH - 1] = 0;
	_tcsncpy(TWO, two, MAX_PATH - 1);
	TWO[MAX_PATH - 1] = 0;
	_tcslwr(ONE);
	_tcslwr(TWO);
	return _tcsstr(ONE, TWO);
}

int listProcesses(bool header, const _TCHAR* srch, bool memInfo)
{
	DWORD* processes;
	const DWORD ProcessFetchSize = 2048;	// should be enough
	DWORD pNeeded;
	int count = 0;

	if (header)
		_tprintf(_T("Filename       PID Path\n"));

	processes = new DWORD[ProcessFetchSize];
	if (processes == 0) {
        _tprintf(_T("Could not allocate process array"));
		return 0;
	}

	BOOL result = EnumProcesses(processes, ProcessFetchSize * sizeof(DWORD), &pNeeded);

	pNeeded /= sizeof(DWORD);
	if (pNeeded > ProcessFetchSize)
		pNeeded = ProcessFetchSize;

	for (DWORD i = 0; i < pNeeded; i++) {
		TCHAR name[MAX_PATH] = _T("");
		TCHAR fullname[MAX_PATH] = _T("");
		HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION  | PROCESS_VM_READ /* | SYNCHRONIZE | PROCESS_TERMINATE */, FALSE, processes[i]);
		if (hProc == NULL) {
			if (srch == 0) {
				_tprintf(_T("%-12s %5u\n"), _T("?"), processes[i]);
				count++;
			}
		}
		else {
			HMODULE hMod;
			DWORD mNeeded;
			result = EnumProcessModules(hProc, &hMod, sizeof(DWORD), &mNeeded);
			// note: you don't need to CloseHandle on hMod
			if (result != TRUE) {
				if (srch == 0) {
					_tprintf(_T("%-12s %5u\n"), _T("??"), processes[i]);
					count++;
				}
			}
			else {
				DWORD res = GetModuleBaseName(hProc, hMod, name, MAX_PATH);
				if (res == 0) {
					name[0] = _T('?');
					name[1] = 0;
				}
				res = GetModuleFileNameEx(hProc, hMod, fullname, MAX_PATH);
				if (res == 0)
					fullname[0] = 0;
				if (srch == 0 || match(name, srch) != 0 || match(fullname, srch) != 0) {
                    _tprintf(_T("%-12s %5u %s\n"), name, processes[i], fullname);
					count++;

					if (memInfo) {
						PROCESS_MEMORY_COUNTERS mem;
						BOOL ret = GetProcessMemoryInfo(hProc, &mem, sizeof(mem));
						if (ret != TRUE)
							_tprintf(_T("\tError reading memory data\n"));
						else {
							_tprintf(_T("\tPageFaultCount:             %10u\n"), mem.PageFaultCount);
							_tprintf(_T("\tPeakWorkingSetSize:         %10u\n"), mem.PeakWorkingSetSize);
							_tprintf(_T("\tWorkingSetSize:             %10u\n"), mem.WorkingSetSize);
							_tprintf(_T("\tQuotaPeakPagedPoolUsage:    %10u\n"), mem.QuotaPeakPagedPoolUsage);
							_tprintf(_T("\tQuotaPagedPoolUsage:        %10u\n"), mem.QuotaPagedPoolUsage);
							_tprintf(_T("\tQuotaPeakNonPagedPoolUsage: %10u\n"), mem.QuotaPeakNonPagedPoolUsage);
							_tprintf(_T("\tQuotaNonPagedPoolUsage:     %10u\n"), mem.QuotaNonPagedPoolUsage);
							_tprintf(_T("\tPeakPagefileUsage:          %10u\n"), mem.PeakPagefileUsage);
							_tprintf(_T("\tPagefileUsage:              %10u\n"), mem.PagefileUsage);
						}
					}
				}
			}
			result = CloseHandle(hProc);
#ifdef _DEBUG
			if (result != TRUE)
				_tprintf(_T("\tUnable to close process handle for process %u\n"), processes[i]);
#endif
		}
	}

	delete [] processes;

	return count;
}

int listDrivers(bool header, const _TCHAR* srch)
{
	LPVOID* drivers;
	const DWORD DriverFetchSize = 2048;	// should be enough
	DWORD pNeeded;
	int count = 0;

	if (header)
		_tprintf(_T("Filename     BaseAddr Path\n"));

	drivers = new LPVOID[DriverFetchSize];
	if (drivers == 0) {
        _tprintf(_T("Could not allocate driver array"));
		return 0;
	}

	BOOL result = EnumDeviceDrivers(drivers, DriverFetchSize * sizeof(LPVOID), &pNeeded);

	pNeeded /= sizeof(LPVOID);
	if (pNeeded > DriverFetchSize)
		pNeeded = DriverFetchSize;

	for (DWORD i = 0; i < pNeeded; i++) {
		TCHAR name[MAX_PATH] = _T("");
		TCHAR fullname[MAX_PATH] = _T("");
		DWORD res = GetDeviceDriverBaseName(drivers[i], name, MAX_PATH);
		if (res == 0) {
			name[0] = _T('?');
			name[1] = 0;
		}
		res = GetDeviceDriverFileName(drivers[i], fullname, MAX_PATH);
		if (res == 0)
			fullname[0] = 0;

		if (srch == 0 || match(name, srch) != 0 || match(fullname, srch) != 0) {
			_tprintf(_T("%-12s %08p %s\n"), name, drivers[i], fullname);
			count++;
		}
	}

	delete [] drivers;

	return count;
}

void usage()
{
	_tprintf(
		_T("\nplist [options] [searchtext]\n")
		_T("\tList processes and device drivers\n")
		_T("\tCopyright (C) 2003, 2004 Michael D. Lore.\n")
		_T("\nOptions:\n")
		_T("-d\tShow device drivers (otherwise shows processes)\n")
		_T("-h\tShow column headers\n")
		_T("-m\tShow process memory information\n")
		_T("-p\tShow process performance information\n")
		_T("-?\tShow this help (or -help)\n")
		_T("\nThe return code is the number of processes listed.\n")
		);
}

void showPerfInfo()
{
	_tprintf(_T("\n"));
#if 0
	PERFORMACE_INFORMATION perf;
	BOOL ret = GetPerformanceInfo(&perf, sizeof(perf));
	if (ret != TRUE)
		_tprintf(_T("Unable to get performance data.\n"));
	else {
		_tprintf(_T("CommitTotal:       %10u\n"), perf.CommitTotal);
		_tprintf(_T("CommitLimit:       %10u\n"), perf.CommitLimit);
		_tprintf(_T("CommitPeak:        %10u\n"), perf.CommitPeak);
		_tprintf(_T("PhysicalTotal:     %10u\n"), perf.PhysicalTotal);
		_tprintf(_T("PhysicalAvailable: %10u\n"), perf.PhysicalAvailable);
		_tprintf(_T("SystemCache:       %10u\n"), perf.SystemCache);
		_tprintf(_T("KernelTotal:       %10u\n"), perf.KernelTotal);
		_tprintf(_T("KernelPaged:       %10u\n"), perf.KernelPaged);
		_tprintf(_T("KernelNonPaged:    %10u\n"), perf.KernelNonpaged);
		_tprintf(_T("PageSize:          %10u\n"), perf.PageSize);
		_tprintf(_T("HandleCount:       %10u\n"), perf.HandleCount);
		_tprintf(_T("ProcessCount:      %10u\n"), perf.ProcessCount);
		_tprintf(_T("ThreadCount:       %10u\n"), perf.ThreadCount);
	}
#else
	_tprintf(_T("Performance data not implemented yet.\n"));
#endif
}

int _tmain(int argc, _TCHAR* argv[])
{
	int rval = 0;
	bool listProc = true;
	bool header = false;
	_TCHAR* srch = 0;
	bool memInfo = false;
	bool perfInfo = false;

	for (int i = 1; i < argc; i++) {
		if (_tcsicmp(argv[i], _T("-?")) == 0 || _tcsicmp(argv[i], _T("-help")) == 0  || _tcsicmp(argv[i], _T("/?")) == 0) {
			usage();
			return 0;
		}
		else if (argv[i][0] == _T('-')) {
			// process options flags
			int len = (int) _tcslen(argv[i]);
			for (int j = 1; j < len; j++) {
				switch (argv[i][j]) {
				case _T('d'):
					listProc = false;
					break;
				case _T('h'):
					header = true;
					break;
				case _T('m'):
					memInfo = true;
					break;
				case _T('p'):
					perfInfo = true;
					break;
				case _T('?'):
					usage();
					return 0;
				default:
					_tprintf(_T("Unknown option '%c'.\n"), argv[i][j]);
					usage();
					return 0;
				}
			}
		}
		else {
			// search text
			srch = argv[i];
		}
	}

	if (listProc)
		rval = listProcesses(header, srch, memInfo);
	else
		rval  = listDrivers(header, srch);

	if (perfInfo)
		showPerfInfo();

	return rval;
}


