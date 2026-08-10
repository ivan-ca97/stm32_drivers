#pragma once

struct ReactorMeasurements
{
    using ValueType = float;

    ValueType reactivityLogarithmic;
    ValueType reactivityLinear;
    ValueType powerRateLogarithmic;
    ValueType powerRateLinear;
};