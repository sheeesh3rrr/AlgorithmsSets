#include <iostream>
#include <fstream>
#include <numeric>
#include <vector>
#include <string>
#include <random>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <unordered_set>

class RandomStreamGen {
private:
    std::vector<std::string> stream;
    std::mt19937 rng;

    const std::string alphabet =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789-";

    std::string generateString() {
        std::uniform_int_distribution<int> lenDist(1, 30);
        std::uniform_int_distribution<int> charDist(0, alphabet.size()-1);

        int len = lenDist(rng);

        std::string s;
        s.reserve(len);

        for(int i=0;i<len;i++)
            s += alphabet[charDist(rng)];

        return s;
    }

public:

    RandomStreamGen(size_t size, uint32_t seed = 42) : rng(seed) {
        stream.reserve(size);

        for(size_t i=0;i<size;i++)
            stream.push_back(generateString());
    }

    const std::vector<std::string>& getStream() const {
        return stream;
    }

    std::vector<std::string> getPrefix(double percent) const {

        size_t count = stream.size() * percent;

        return std::vector<std::string>(
            stream.begin(),
            stream.begin() + count
        );
    }
};

class HashFuncGen {
public:

    static uint32_t fnv(const std::string& s){

        uint32_t hash = 2166136261;

        for(unsigned char c : s){
            hash ^= c;
            hash *= 16777619;
        }

        return hash;
    }
};

#include <vector>
#include <cmath>
#include <algorithm>

class HyperLogLog {

private:

    int B;
    int q;
    std::vector<uint8_t> registers;

    double alpha() const {

        if (q == 2) return 0.3512;
        if (q == 4) return 0.5324;
        if (q == 16) return 0.673;
        if (q == 32) return 0.697;
        if (q == 64) return 0.709;

        return 0.7213 / (1 + 1.079/q);
    }

    int leadingZeros(uint32_t x){

        if(x == 0) return 32;

        return __builtin_clz(x);
    }

public:

    HyperLogLog(int B) : B(B), q(1<<B), registers(q,0) {}

    void add(uint32_t hash){

        uint32_t index = hash >> (32-B);

        uint32_t w = hash << B;

        int zeros = leadingZeros(w) + 1;

        registers[index] = std::max(
            registers[index],
            (uint8_t)zeros
        );
    }

    double estimate() const {

        double sum = 0;

        for(auto r : registers)
            sum += std::pow(2.0, -r);

        double raw = alpha()*q*q / sum;

        return raw;
    }
};

size_t exactCount(const std::vector<std::string>& stream) {
    std::unordered_set<std::string> s(
        stream.begin(),
        stream.end()
    );

    return s.size();
}

/*
Пусть в наших экспериментах B = 12
Обоснование:

В HyperLogLg точность оценки количества уникальных элементов
напрямую зависит от числа потоков: q = 2^B

Стандартное отклонение оценки задается формулой: sigma ~ 1.04 / sqrt(q)

Следовательно, увеличение B уменьшает ошибку, но увеличивает
потребление памяти

-----------------------------------------
Теоретическая оценка точнсти для разных B:

B = 8
q = 256
sigma ~ 1.04 / 16 ~ 6.5%

B = 10
q = 1024
sigma ~ 1.04 / 32 ~ 3.25%

B = 12
q = 4096
sigma ~ 1.04 / 64 ~ 1.6%

B = 14
q = 16384
sigma ~ 1.04 / 128 ~ 0.8%

------------------------------------------------
Анализ компромисса память / точность

Каждый поток хранится в uint8_t (1 байт).

Память:
B = 10 -> 1 KB  
B = 12 -> 4 KB  
B = 14 -> 16 KB  

Даже B=12 требует крайне мало памяти,
но уже обеспечивает ошибку около 1–2%,
что считается отличным результатом
для вероятностных алгоритмов.

------------------------------------------------
Почему не берем больше?

Увеличение B после 12 дает
не дает большого выигыша в точности

1.6% -> 0.8%

При этом память растт в 4 раза.

----------------------------------------------
Вывод:

Параметр B = 12 обеспечивает:

- малую теоретическую ошибку (~1.6%)
- низкое потребление памяти (~4 KB)

Поэтому B=12 выбран как оптимальный
баланс точности и эффективности
*/

int main() {

    const int B = 12;
    const int STREAM_COUNT = 10;
    const size_t STREAM_SIZE = 300000;

    std::vector<double> steps =
        {0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0};

    std::ofstream graph1("graph1.csv");
    std::ofstream graph2("graph2.csv");

    graph1 << "size,exact,estimate\n";
    graph2 << "step,mean,stddev\n";

    std::vector<std::vector<double>> allEstimates(steps.size());
    std::vector<size_t> exactValues(steps.size());

    for (int streamId = 0; streamId < STREAM_COUNT; ++streamId){

        RandomStreamGen gen(STREAM_SIZE, streamId + 1);

        for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex){

            double step = steps[stepIndex];
            auto prefix = gen.getPrefix(step);

            HyperLogLog hll(B);

            for (const auto& s : prefix) {
                hll.add(HashFuncGen::fnv(s));
            }

            double estimate = hll.estimate();
            size_t exact = exactCount(prefix);

            allEstimates[stepIndex].push_back(estimate);

            if (streamId == 0) {
                exactValues[stepIndex] = exact;
                graph1 << prefix.size()
                       << "," << exact
                       << "," << estimate
                       << "\n";
            }
        }
    }

    double theoreticalSigma = 1.04 / std::sqrt(1 << B);

    std::cout << "Параметр B = " << B << "\n";
    std::cout << "Количество потоков: " << (1 << B) << "\n";
    std::cout << "теоретическая относительная ошибка ~ " << theoreticalSigma * 100 << " %\n\n";

    for (size_t i = 0; i < steps.size(); ++i) {

        auto& vec = allEstimates[i];

        double mean =
            std::accumulate(vec.begin(), vec.end(), 0.0) / vec.size();

        double variance = 0;
        for (double v : vec) {
            variance += (v - mean) * (v - mean);
        }
        variance /= vec.size();

        double stddev = std::sqrt(variance);

        graph2 << steps[i]
               << "," << mean
               << "," << stddev
               << "\n";

        double relativeError =
            std::abs(mean - exactValues[i]) / exactValues[i] * 100.0;

        std::cout << "\n\n"
        << "Шаг (доля потока): " << steps[i] * 100 << " %\n"
        << "Размер обработанного префикса: "
        << (size_t)(STREAM_SIZE * steps[i]) << "\n"
        << "Точное число уникальных элементов (F_t0): "
        << exactValues[i] << "\n"
        << "оценка HyperLogLog E(N_t): "
        << mean << "\n"
        << "Абсолютное стандартное отклонение omega(N_t): "
        << stddev << "\n"
        << "Относительное стандартное отклонение: "
        << (stddev / mean) * 100 << " %\n"
        << "Относительная ошибка оценки: "
        << relativeError << " %\n";
    }

    return 0;
}
