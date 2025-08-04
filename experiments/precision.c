#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef struct
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
} abc_t;

typedef struct
{
    uint32_t p1;
    uint32_t p2;
    uint32_t p3;
} pll_parameters;

// Float version
void calculate_parameters_f(const abc_t *div, pll_parameters *result)
{
    float a = (float)div->a;
    float b = (float)div->b;
    float c = (float)div->c;

    uint32_t frac = (uint32_t)((128.0f * b) / c);

    result->p1 = (uint32_t)(128.0f * a + frac - 512.0f);
    result->p2 = (uint32_t)(128.0f * b - c * frac);
    result->p3 = div->c;
}

// Double version
void calculate_parameters_d(const abc_t *div, pll_parameters *result)
{
    double a = (double)div->a;
    double b = (double)div->b;
    double c = (double)div->c;

    uint32_t frac = (uint32_t)((128.0 * b) / c);

    result->p1 = (uint32_t)(128.0 * a + frac - 512.0);
    result->p2 = (uint32_t)(128.0 * b - c * frac);
    result->p3 = div->c;
}

#define max_c 1048575U

void decompose_ratio_f(float ratio, abc_t *result)
{
    result->a = (uint32_t)floorf(ratio);
    float fractional = ratio - result->a;
    result->c = max_c;
    result->b = (uint32_t)roundf(fractional * result->c);

    if (result->b >= result->c)
    {
        result->b = 0;
        result->a += 1;
    }
}

void decompose_ratio_d(double ratio, abc_t *result)
{
    result->a = (uint32_t)floor(ratio);
    double fractional = ratio - result->a;
    result->c = max_c;
    result->b = (uint32_t)round(fractional * result->c);

    if (result->b >= result->c)
    {
        result->b = 0;
        result->a += 1;
    }
}

double get_multiplier_ratio(pll_parameters *params)
{
    // Recover the floor(128 * b / c) part
    uint32_t frac = (params->p1 + 512) % 128;

    // Recover integer part
    uint32_t a = (params->p1 + 512) / 128;

    // Recover b
    uint32_t b = (params->p2 + params->p3 * frac) / 128;

    // Compute full ratio
    return (double)a + (double)b / (double)params->p3;
}

double do_float_chain(double input_frequency, double fixed_divider)
{
    double result = 0.0;
    abc_t decomposed = {0};
    pll_parameters pll_params = {0};

    float ratio = (float)input_frequency * (float)fixed_divider / 25.0f;
    double recovered_ratio = 0.0;

    decompose_ratio_f(ratio, &decomposed);
    calculate_parameters_f(&decomposed, &pll_params);

    recovered_ratio = get_multiplier_ratio(&pll_params);
    result = (double)recovered_ratio * 25.0 / fixed_divider;

    return result;
}

double do_double_chain(double input_frequency, double fixed_divider)
{
    double result = 0.0;
    abc_t decomposed = {0};
    pll_parameters pll_params = {0};

    double ratio = input_frequency * fixed_divider / 25.0;
    double recovered_ratio = 0.0;

    decompose_ratio_d(ratio, &decomposed);
    calculate_parameters_d(&decomposed, &pll_params);

    recovered_ratio = get_multiplier_ratio(&pll_params);
    result = recovered_ratio * 25.0 / fixed_divider;

    return result;
}

int main(int argc, char *argv[])
{

    double at_frequency_f = 0.0;
    double max_error_f = 0.0;

    double at_frequency_d = 0.0;
    double max_error_d = 0.0;

    for (int i = 0; i < 10000000; i++)
    {
        double input_frequency = 3.5 + ((double)i / 10000000.0);
        double fixed_divider = 120.0;

        double result_f = do_float_chain(input_frequency, fixed_divider);

        double error = fabs(result_f - input_frequency) * 1000000;
        if (error > max_error_f)
        {
            max_error_f = error;
            at_frequency_f = input_frequency;
        }

        double result_d = do_double_chain(input_frequency, fixed_divider);
        error = fabs(result_d - input_frequency) * 1000000;
        if (error > max_error_d)
        {
            max_error_d = error;
            at_frequency_d = input_frequency;
        }
    }

    printf("Max error for float: %f at frequency: %f\n", max_error_f, at_frequency_f);
    printf("Max error for double: %f at frequency: %f\n", max_error_d, at_frequency_d);

    return 0;
}
