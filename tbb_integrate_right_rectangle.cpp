#include <iostream>
#include <cmath>

#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/tick_count.h>

using namespace std;
using namespace tbb;

/* Обчислення підінтегральної функції. */
class integrand
{
public:
    double operator()(double x) const
    {
        /* x^3 */
        return x*x*x;
    }
};

/* Перевірка правила Рунге. */
bool check_Runge(double I2, double I, double epsilon)
{
    return (fabs(I2 - I) / 3.) < epsilon;
}

/*
 * Об'єкт-функція, що виконує додавання елементів з проміжку r,
 * використовуючи значення start як початкове.
 */
template<typename Integrand>
class integrate_right_rectangle_func
{
public:
    integrate_right_rectangle_func(Integrand f_, double epsilon_):
            f(f_),
            epsilon(epsilon_)
    {}
    double operator()(const blocked_range<double> &r, double start) const
    {
        int num_iterations = 1; /* Початкова кількість ітерацій */
        double last_res = 0.;   /* Результат інтeгрування на попередньому кроці */
        double res = -1.;       /* Поточний результат інтегрування */
        while(!check_Runge(res, last_res, epsilon))
        {
            num_iterations *= 2;
            last_res = res;

            double a = r.begin();
            double b = r.end();
            long long n = num_iterations;

            double h = (b - a) / (double)n;

            double sum = 0;

            for (long long i = 1; i <= n; i++)
                sum += f(a + i * h);

            res =  h * sum;
        }
        return start + res;
    }
    Integrand f;
    double epsilon;
};

/*
 * Об'єкт-функція, що виконує додавання двох елементів
 */
class integrate_right_rectangle_reduction
{
public:
    double operator()(double a, double b) const
    {
        return a + b;
    }
};

template<typename Integrand>
double parallel_integrate_left_rectangle(Integrand f, double start, double end, double epsilon)
{
    return parallel_reduce(
            blocked_range<double>(start, end, 0),
            0.,
            integrate_right_rectangle_func<Integrand>(f, epsilon),
            integrate_right_rectangle_reduction());
}

int main()
{
    const int P_max = task_scheduler_init::default_num_threads();
    for(int P = 1; P <= P_max; P++)
    {
        task_scheduler_init init(P);
        tick_count t0 = tick_count::now();
        cout << parallel_integrate_left_rectangle(integrand(), 0, 70, 1e-5)
                  << endl;
        tick_count t1 = tick_count::now();
        double t = (t1 - t0).seconds();
        cout << "time = " << t << " with " << P << " threads" << endl;
    }
}
