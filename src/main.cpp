// Лабораторна 4, варіант 18. Компілятор: MSVC, C++20.
// Виконує дії з тестових файлів у 1, 2 і 3 потоках над однією спільною структурою Fields
// і вимірює час. Час зчитування файлів не враховується.
#include "fields.h"
#include "scenarios.h"
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
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

    enum class Kind : uint8_t
    {
        Read,
        Write,
        String
    };

    struct Op
    {
        Kind kind;
        uint8_t field;
        int value;
    };

    using Ops = vector<Op>;

    constexpr int repeats = 10;

    int field_of(const string &word, istream &in, const filesystem::path &path)
    {
        int field = -1;
        if (!(in >> field) || field < 0 || field >= m)
            throw runtime_error(format("{}: некоректний номер поля після '{}'", path.string(), word));
        return field;
    }

    Ops load(const filesystem::path &path)
    {
        ifstream in(path);
        if (!in)
            throw runtime_error("не вдалося відкрити " + path.string() + " (спершу запустіть generator)");
        Ops ops;
        string word;
        while (in >> word)
        {
            if (word == "read")
                ops.push_back({Kind::Read, static_cast<uint8_t>(field_of(word, in, path)), 0});
            else if (word == "write")
            {
                const int field = field_of(word, in, path);
                int value = 0;
                if (!(in >> value))
                    throw runtime_error(path.string() + ": немає значення для write");
                ops.push_back({Kind::Write, static_cast<uint8_t>(field), value});
            }
            else if (word == "string")
                ops.push_back({Kind::String, 0, 0});
            else
                throw runtime_error(format("{}: невідома операція '{}'", path.string(), word));
        }
        return ops;
    }

    // Кожен потік рахує власну контрольну суму, щоб результати читань не були викинуті оптимізатором.
    int64_t execute(Fields &data, span<const Op> ops)
    {
        int64_t sum = 0;
        for (const Op &op : ops)
            switch (op.kind)
            {
            case Kind::Read: sum += data.get(op.field); break;
            case Kind::Write: data.set(op.field, op.value); break;
            case Kind::String: sum += static_cast<int64_t>(static_cast<string>(data).size()); break;
            }
        return sum;
    }

    struct Run
    {
        double ms;
        int64_t checksum;
    };

    // Потоки створюються заздалегідь і стартують одночасно за сигналом (mutex + condition_variable),
    // тож у вимір не потрапляє ні читання файлів, ні створення потоків.
    Run measure(const vector<Ops> &per_thread)
    {
        Fields data;
        vector<int64_t> sums(per_thread.size());
        mutex start_mutex;
        condition_variable start_cv;
        bool go = false;

        vector<thread> threads;
        threads.reserve(per_thread.size());
        for (size_t t = 0; t < per_thread.size(); ++t)
            threads.emplace_back([&, t] {
                {
                    unique_lock lock(start_mutex);
                    start_cv.wait(lock, [&] { return go; });
                }
                sums[t] = execute(data, per_thread[t]);
            });

        chrono::steady_clock::time_point start;
        {
            lock_guard lock(start_mutex);
            go = true;
            start = chrono::steady_clock::now();
        }
        start_cv.notify_all();
        for (auto &th : threads)
            th.join();
        const chrono::duration<double, milli> elapsed = chrono::steady_clock::now() - start;

        int64_t checksum = 0;
        for (const int64_t s : sums)
            checksum += s;
        return {elapsed.count(), checksum};
    }

    string percents(const vector<Ops> &per_thread)
    {
        array<size_t, kinds> count{};
        size_t total = 0;
        for (const auto &ops : per_thread)
            for (const Op &op : ops)
            {
                const int k = op.kind == Kind::String ? 4 : op.field * 2 + (op.kind == Kind::Write ? 1 : 0);
                ++count[k];
                ++total;
            }
        string text;
        for (int k = 0; k < kinds; ++k)
            text += format("{}{} {:.1f}%", k ? ", " : "", kind_names[k], 100.0 * count[k] / total);
        return text;
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
#ifdef NDEBUG
        cout << "Збірка: Release\n";
#else
        cout << "Збірка: Debug (для вимірювань використовуйте Release)\n";
#endif
        cout << format("Варіант 18, m = {}. Операцій у кожному вимірі: {}. Повторів для усереднення: {}.\n",
                       m, total_ops, repeats);

        array<array<double, max_threads>, scenarios.size()> table{};
        int64_t checksum = 0;

        for (size_t s = 0; s < scenarios.size(); ++s)
        {
            const Scenario &sc = scenarios[s];
            cout << format("\nСценарій {}) {}\n", sc.name, sc.title);
            for (int threads = 1; threads <= max_threads; ++threads)
            {
                vector<Ops> per_thread;
                size_t ops_count = 0;
                for (int t = 0; t < threads; ++t)
                {
                    per_thread.push_back(load(root / string(1, sc.name) / format("threads{}_{}.txt", threads, t)));
                    ops_count += per_thread.back().size();
                }
                if (ops_count != static_cast<size_t>(total_ops))
                    throw runtime_error(format("сценарій {}, {} потоки: {} операцій замість {}",
                                               sc.name, threads, ops_count, total_ops));
                if (threads == 1)
                    cout << "  фактичні частоти: " << percents(per_thread) << '\n';

                measure(per_thread); // розігрів, не враховується
                double sum = 0, best = 1e300, worst = 0;
                for (int r = 0; r < repeats; ++r)
                {
                    const Run run = measure(per_thread);
                    sum += run.ms;
                    best = min(best, run.ms);
                    worst = max(worst, run.ms);
                    checksum += run.checksum;
                }
                table[s][threads - 1] = sum / repeats;
                cout << format("  потоків: {}  середній час {:8.3f} мс  (мін {:8.3f}, макс {:8.3f})\n",
                               threads, sum / repeats, best, worst);
            }
        }

        cout << "\nУсереднений час виконання, мс:\n";
        cout << format("{:<12}{:>12}{:>12}{:>12}\n", "файл", "1 потік", "2 потоки", "3 потоки");
        for (size_t s = 0; s < scenarios.size(); ++s)
            cout << format("{:<12}{:>12.3f}{:>12.3f}{:>12.3f}\n", format("{})", scenarios[s].name),
                           table[s][0], table[s][1], table[s][2]);
        cout << format("\nКонтрольна сума: {}\n", checksum);
    }
    catch (const exception &e)
    {
        cerr << "Помилка: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
