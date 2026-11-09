// Демо-код: генератор тестових файлів. Компілятор: MSVC, C++20.
// Для кожного сценарію a, b, c і кожної кількості потоків k = 1, 2, 3 створює k файлів
// data/<сценарій>/threads<k>_<номер потоку>.txt по total_ops / k операцій.
// Кількість операцій кожного виду в файлі точно дорівнює заданій частці, порядок - випадковий.
#include "scenarios.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace
{
    using namespace std;
    using namespace lab;

    void write_file(const filesystem::path &path, const Scenario &s, int count, uint32_t seed)
    {
        vector<uint8_t> ops;
        ops.reserve(count);
        for (int k = 0; k < kinds; ++k)
            ops.insert(ops.end(), static_cast<size_t>(count) * s.percent[k] / 100, static_cast<uint8_t>(k));

        mt19937 rng(seed);
        shuffle(ops.begin(), ops.end(), rng);
        uniform_int_distribution<int> value(1, 1000);

        string text;
        text.reserve(ops.size() * 10);
        for (const uint8_t k : ops)
            switch (k)
            {
            case 0: text += "read 0\n"; break;
            case 1: text += format("write 0 {}\n", value(rng)); break;
            case 2: text += "read 1\n"; break;
            case 3: text += format("write 1 {}\n", value(rng)); break;
            default: text += "string\n"; break;
            }

        ofstream out(path, ios::binary);
        out.write(text.data(), static_cast<streamsize>(text.size()));
        if (!out)
            throw runtime_error("не вдалося записати " + path.string());
    }
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    try
    {
        const filesystem::path root = argc > 1 ? argv[1] : "data";
        for (const auto &s : scenarios)
        {
            const filesystem::path dir = root / string(1, s.name);
            filesystem::create_directories(dir);
            for (int threads = 1; threads <= max_threads; ++threads)
                for (int t = 0; t < threads; ++t)
                {
                    const auto path = dir / format("threads{}_{}.txt", threads, t);
                    const uint32_t seed = 1000u * s.name + 10u * threads + t;
                    write_file(path, s, total_ops / threads, seed);
                }
            cout << format("Сценарій {} ({}): файли створено в {}\n", s.name, s.title, dir.string());
        }
        cout << format("Усього операцій для кожної кількості потоків: {}\n", total_ops);
    }
    catch (const exception &e)
    {
        cerr << "Помилка: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
