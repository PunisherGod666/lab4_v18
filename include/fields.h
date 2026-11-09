#pragma once
#include <format>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>

// Варіант 18: m = 2.
// Частоти операцій: поле 0 - read 20%, write 10%; поле 1 - read 25%, write 10%; string 35%.
//
// Схема захисту: два std::shared_mutex, по одному на поле.
//   read  i  -> shared_lock  на mutex поля i   (читання одного поля паралельні між собою);
//   write i  -> unique_lock  на mutex поля i   (запис у поле 0 не заважає роботі з полем 1);
//   string   -> shared_lock  на обидва mutex (завжди в порядку 0, 1), копіюємо значення,
//               а рядок форматуємо вже після звільнення м'ютексів.
// Операції лише читання становлять 80% (20 + 25 + 35), тому shared-блокування дають
// змогу виконувати їх одночасно. Запис блокує тільки своє поле.
// Кожне поле разом зі своїм м'ютексом лежить в окремій кеш-лінії (64 байти),
// щоб потоки, які працюють з різними полями, не заважали один одному (false sharing).
// Взаємоблокування неможливе: два м'ютекси одночасно бере тільки string і завжди в одному порядку.
namespace lab
{
    inline constexpr int m = 2;

    class Fields
    {
    public:
        int get(int field) const
        {
            const Slot &slot = at(field);
            std::shared_lock lock(slot.mutex);
            return slot.value;
        }

        void set(int field, int value)
        {
            Slot &slot = at(field);
            std::unique_lock lock(slot.mutex);
            slot.value = value;
        }

        explicit operator std::string() const
        {
            int v0, v1;
            {
                std::shared_lock lock0(slots_[0].mutex);
                std::shared_lock lock1(slots_[1].mutex);
                v0 = slots_[0].value;
                v1 = slots_[1].value;
            }
            return std::format("[{}, {}]", v0, v1);
        }

    private:
#pragma warning(push)
#pragma warning(disable : 4324) // структуру доповнено до вирівнювання - так і задумано
        struct alignas(64) Slot
        {
            mutable std::shared_mutex mutex;
            int value = 0;
        };
#pragma warning(pop)

        static void check(int field)
        {
            if (field < 0 || field >= m)
                throw std::out_of_range("номер поля поза межами");
        }

        const Slot &at(int field) const
        {
            check(field);
            return slots_[field];
        }

        Slot &at(int field)
        {
            check(field);
            return slots_[field];
        }

        Slot slots_[m]; // звичайний масив розміру m - дозволено умовою
    };
}
