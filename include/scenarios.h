#pragma once
#include <array>
#include <string_view>

// Сценарії тестових файлів. Частки у відсотках у порядку:
// read 0, write 0, read 1, write 1, string.
namespace lab
{
    inline constexpr int kinds = 5;
    inline constexpr std::array<std::string_view, kinds> kind_names{
        "read 0", "write 0", "read 1", "write 1", "string"};

    struct Scenario
    {
        char name;
        std::string_view title;
        std::array<int, kinds> percent;
    };

    inline constexpr std::array<Scenario, 3> scenarios{{
        {'a', "частоти за умовою (варіант 18)", {20, 10, 25, 10, 35}},
        {'b', "усі частоти рівні", {20, 20, 20, 20, 20}},
        {'c', "частоти дуже не відповідають умові", {5, 40, 5, 40, 10}},
    }};

    inline constexpr int max_threads = 3;
    // Загальна кількість операцій однакова для 1, 2 і 3 потоків.
    inline constexpr int total_ops = 300000;

    static_assert(total_ops % (100 * 2 * 3) == 0, "частки мають ділитися націло");

    constexpr bool sums_to_100()
    {
        for (const auto &s : scenarios)
        {
            int sum = 0;
            for (const int p : s.percent)
                sum += p;
            if (sum != 100)
                return false;
        }
        return true;
    }
    static_assert(sums_to_100());
}
