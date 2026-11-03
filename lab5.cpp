#include <iostream>
#include <vector>
#include <string>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/wait.h>
#endif

const int NR_PROCESE = 10;
const int NUMERE_PE_PROCES = 1000;

bool estePrim(int n)
{
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    for (int d = 3; d * d <= n; d += 2)
        if (n % d == 0) return false;
    return true;
}

std::vector<int> cautaPrime(int start, int sfarsit)
{
    std::vector<int> prime;
    for (int n = start; n <= sfarsit; n++)
        if (estePrim(n)) prime.push_back(n);
    return prime;
}

void afiseaza(int nr, int start, int sfarsit, const std::vector<int>& prime)
{
    std::cout << "Procesul " << nr << " (interval " << start << " - " << sfarsit
              << ") a gasit " << prime.size() << " numere prime:" << std::endl;
    for (size_t i = 0; i < prime.size(); i++) {
        std::cout << prime[i] << " ";
        if ((i + 1) % 20 == 0) std::cout << std::endl;
    }
    std::cout << std::endl << std::endl;
}

#ifdef _WIN32

bool scrieTot(HANDLE h, const void* date, DWORD lungime)
{
    const char* p = (const char*)date;
    while (lungime > 0) {
        DWORD scrisi = 0;
        if (!WriteFile(h, p, lungime, &scrisi, NULL) || scrisi == 0) return false;
        p += scrisi;
        lungime -= scrisi;
    }
    return true;
}

bool citesteTot(HANDLE h, void* date, DWORD lungime)
{
    char* p = (char*)date;
    while (lungime > 0) {
        DWORD cititi = 0;
        if (!ReadFile(h, p, lungime, &cititi, NULL) || cititi == 0) return false;
        p += cititi;
        lungime -= cititi;
    }
    return true;
}

int ruleazaCopil()
{
    HANDLE intrare = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE iesire = GetStdHandle(STD_OUTPUT_HANDLE);

    int interval[2];
    if (!citesteTot(intrare, interval, sizeof(interval))) return 1;

    std::vector<int> prime = cautaPrime(interval[0], interval[1]);

    int cate = (int)prime.size();
    scrieTot(iesire, &cate, sizeof(cate));
    if (cate > 0) scrieTot(iesire, prime.data(), cate * sizeof(int));
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc > 1 && strcmp(argv[1], "copil") == 0) {
        return ruleazaCopil();
    }

    char caleExe[MAX_PATH];
    GetModuleFileNameA(NULL, caleExe, MAX_PATH);

    HANDLE catreCopil[NR_PROCESE];
    HANDLE delaCopil[NR_PROCESE];
    PROCESS_INFORMATION pi[NR_PROCESE];

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    std::cout << "Parintele (PID " << GetCurrentProcessId() << ") creeaza "
              << NR_PROCESE << " procese..." << std::endl << std::endl;

    for (int i = 0; i < NR_PROCESE; i++) {
        HANDLE copilCiteste, copilScrie;

        if (!CreatePipe(&copilCiteste, &catreCopil[i], &sa, 0) ||
            !CreatePipe(&delaCopil[i], &copilScrie, &sa, 0)) {
            std::cerr << "Eroare la CreatePipe: " << GetLastError() << std::endl;
            return 1;
        }

        SetHandleInformation(catreCopil[i], HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(delaCopil[i], HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = copilCiteste;
        si.hStdOutput = copilScrie;
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        ZeroMemory(&pi[i], sizeof(pi[i]));

        std::string comanda = "\"" + std::string(caleExe) + "\" copil";

        if (!CreateProcessA(NULL, &comanda[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi[i])) {
            std::cerr << "Eroare la CreateProcess: " << GetLastError() << std::endl;
            return 1;
        }

        CloseHandle(copilCiteste);
        CloseHandle(copilScrie);

        int interval[2] = { i * NUMERE_PE_PROCES + 1, (i + 1) * NUMERE_PE_PROCES };
        scrieTot(catreCopil[i], interval, sizeof(interval));
        CloseHandle(catreCopil[i]);
    }

    int total = 0;
    for (int i = 0; i < NR_PROCESE; i++) {
        int cate = 0;
        std::vector<int> prime;
        if (citesteTot(delaCopil[i], &cate, sizeof(cate)) && cate > 0) {
            prime.resize(cate);
            citesteTot(delaCopil[i], prime.data(), cate * sizeof(int));
        }
        CloseHandle(delaCopil[i]);

        afiseaza(i + 1, i * NUMERE_PE_PROCES + 1, (i + 1) * NUMERE_PE_PROCES, prime);
        total += (int)prime.size();
    }

    for (int i = 0; i < NR_PROCESE; i++) {
        WaitForSingleObject(pi[i].hProcess, INFINITE);
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }

    std::cout << "Total numere prime pana la " << NR_PROCESE * NUMERE_PE_PROCES
              << ": " << total << std::endl;
    return 0;
}

#else

bool scrieTot(int fd, const void* date, size_t lungime)
{
    const char* p = (const char*)date;
    while (lungime > 0) {
        ssize_t scrisi = write(fd, p, lungime);
        if (scrisi <= 0) return false;
        p += scrisi;
        lungime -= scrisi;
    }
    return true;
}

bool citesteTot(int fd, void* date, size_t lungime)
{
    char* p = (char*)date;
    while (lungime > 0) {
        ssize_t cititi = read(fd, p, lungime);
        if (cititi <= 0) return false;
        p += cititi;
        lungime -= cititi;
    }
    return true;
}

int main()
{
    int catreCopil[NR_PROCESE][2];
    int delaCopil[NR_PROCESE][2];
    pid_t pids[NR_PROCESE];

    std::cout << "Parintele (PID " << getpid() << ") creeaza "
              << NR_PROCESE << " procese..." << std::endl << std::endl;

    for (int i = 0; i < NR_PROCESE; i++) {
        if (pipe(catreCopil[i]) < 0 || pipe(delaCopil[i]) < 0) {
            std::cerr << "Eroare la pipe()" << std::endl;
            return 1;
        }

        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "Eroare la fork()" << std::endl;
            return 1;
        }

        if (pid == 0) {
            for (int j = 0; j < i; j++) {
                close(delaCopil[j][0]);
            }
            close(catreCopil[i][1]);
            close(delaCopil[i][0]);

            int interval[2];
            if (!citesteTot(catreCopil[i][0], interval, sizeof(interval))) _exit(1);
            close(catreCopil[i][0]);

            std::vector<int> prime = cautaPrime(interval[0], interval[1]);

            int cate = (int)prime.size();
            scrieTot(delaCopil[i][1], &cate, sizeof(cate));
            if (cate > 0) scrieTot(delaCopil[i][1], prime.data(), cate * sizeof(int));
            close(delaCopil[i][1]);
            _exit(0);
        }

        pids[i] = pid;
        close(catreCopil[i][0]);
        close(delaCopil[i][1]);

        int interval[2] = { i * NUMERE_PE_PROCES + 1, (i + 1) * NUMERE_PE_PROCES };
        scrieTot(catreCopil[i][1], interval, sizeof(interval));
        close(catreCopil[i][1]);
    }

    int total = 0;
    for (int i = 0; i < NR_PROCESE; i++) {
        int cate = 0;
        std::vector<int> prime;
        if (citesteTot(delaCopil[i][0], &cate, sizeof(cate)) && cate > 0) {
            prime.resize(cate);
            citesteTot(delaCopil[i][0], prime.data(), cate * sizeof(int));
        }
        close(delaCopil[i][0]);

        afiseaza(i + 1, i * NUMERE_PE_PROCES + 1, (i + 1) * NUMERE_PE_PROCES, prime);
        total += (int)prime.size();
    }

    for (int i = 0; i < NR_PROCESE; i++) {
        waitpid(pids[i], NULL, 0);
    }

    std::cout << "Total numere prime pana la " << NR_PROCESE * NUMERE_PE_PROCES
              << ": " << total << std::endl;
    return 0;
}
#endif
