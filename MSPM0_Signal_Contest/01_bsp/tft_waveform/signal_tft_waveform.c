#include "signal_tft_waveform.h"

#include <math.h>
#include <stddef.h>

static signal_result_t DataRange(
    const float *samples,
    size_t sample_count,
    float *minimum,
    float *maximum)
{
    size_t index;
    float low;
    float high;

    if ((samples == NULL) || (sample_count == 0U) ||
        (minimum == NULL) || (maximum == NULL) ||
        !isfinite(samples[0])) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    low = samples[0];
    high = samples[0];
    for (index = 1U; index < sample_count; ++index) {
        if (!isfinite(samples[index])) {
            return SIGNAL_RESULT_NUMERIC_ERROR;
        }
        if (samples[index] < low) {
            low = samples[index];
        }
        if (samples[index] > high) {
            high = samples[index];
        }
    }
    *minimum = low;
    *maximum = high;
    return SIGNAL_RESULT_OK;
}

static void ExpandFlatRange(float center, float *minimum, float *maximum)
{
    float margin = fabsf(center) * 0.01F;
    if (margin < 1.0e-6F) {
        margin = 1.0e-6F;
    }
    *minimum = center - margin;
    *maximum = center + margin;
}

static int32_t PlotX(
    const signal_tft_waveform_config_t *config,
    uint16_t column,
    uint16_t column_count)
{
    if (column_count <= 1U) {
        return config->x;
    }
    return config->x + (int32_t)(
        ((uint32_t)column * (uint32_t)(config->width - 1U)) /
        (uint32_t)(column_count - 1U));
}

signal_result_t SignalTFTWaveform_MapY(
    float value,
    float scale_minimum,
    float scale_maximum,
    int32_t plot_y,
    uint16_t plot_height,
    int32_t *screen_y)
{
    float clipped;
    float ratio;

    if ((screen_y == NULL) || (plot_height == 0U) || !isfinite(value) ||
        !isfinite(scale_minimum) || !isfinite(scale_maximum) ||
        !(scale_maximum > scale_minimum)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    clipped = value;
    if (clipped < scale_minimum) {
        clipped = scale_minimum;
    } else if (clipped > scale_maximum) {
        clipped = scale_maximum;
    }
    ratio = (scale_maximum - clipped) /
        (scale_maximum - scale_minimum);
    *screen_y = plot_y + (int32_t)(ratio * (float)(plot_height - 1U) + 0.5F);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalTFTWaveform_GetEnvelopeColumn(
    const float *samples,
    size_t sample_count,
    uint16_t column_count,
    uint16_t column,
    float *minimum,
    float *maximum)
{
    size_t base;
    size_t remainder;
    size_t start;
    size_t count;

    if ((samples == NULL) || (sample_count == 0U) ||
        (column_count == 0U) || (column >= column_count) ||
        ((size_t)column_count > sample_count) ||
        (minimum == NULL) || (maximum == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    base = sample_count / (size_t)column_count;
    remainder = sample_count % (size_t)column_count;
    start = (size_t)column * base +
        (((size_t)column < remainder) ? (size_t)column : remainder);
    count = base + (((size_t)column < remainder) ? 1U : 0U);
    return DataRange(&samples[start], count, minimum, maximum);
}

static signal_result_t DrawDecorations(
    tft_ili9341_t *tft,
    const signal_tft_waveform_config_t *config,
    float scale_minimum,
    float scale_maximum)
{
    signal_result_t status;
    uint8_t division;
    int32_t coordinate;

    if (config->clear_background) {
        status = TFT_ILI9341_FillRect(tft, config->x, config->y,
            (int32_t)config->width, (int32_t)config->height,
            config->background_color);
        if (status != SIGNAL_RESULT_OK) {
            return status;
        }
    }
    if (config->draw_grid) {
        if (config->vertical_grid_divisions > 1U) {
            for (division = 1U;
                 division < config->vertical_grid_divisions; ++division) {
                coordinate = config->x + (int32_t)(
                    ((uint32_t)division * (uint32_t)(config->width - 1U)) /
                    (uint32_t)config->vertical_grid_divisions);
                status = TFT_ILI9341_DrawLine(tft, coordinate, config->y,
                    coordinate, config->y + (int32_t)config->height - 1,
                    config->grid_color);
                if (status != SIGNAL_RESULT_OK) {
                    return status;
                }
            }
        }
        if (config->horizontal_grid_divisions > 1U) {
            for (division = 1U;
                 division < config->horizontal_grid_divisions; ++division) {
                coordinate = config->y + (int32_t)(
                    ((uint32_t)division * (uint32_t)(config->height - 1U)) /
                    (uint32_t)config->horizontal_grid_divisions);
                status = TFT_ILI9341_DrawLine(tft, config->x, coordinate,
                    config->x + (int32_t)config->width - 1, coordinate,
                    config->grid_color);
                if (status != SIGNAL_RESULT_OK) {
                    return status;
                }
            }
        }
    }
    if (config->draw_baseline &&
        (config->baseline_value >= scale_minimum) &&
        (config->baseline_value <= scale_maximum)) {
        status = SignalTFTWaveform_MapY(config->baseline_value,
            scale_minimum, scale_maximum, config->y, config->height,
            &coordinate);
        if (status != SIGNAL_RESULT_OK) {
            return status;
        }
        status = TFT_ILI9341_DrawLine(tft, config->x, coordinate,
            config->x + (int32_t)config->width - 1, coordinate,
            config->baseline_color);
        if (status != SIGNAL_RESULT_OK) {
            return status;
        }
    }
    if (config->draw_border) {
        return TFT_ILI9341_DrawRect(tft, config->x, config->y,
            (int32_t)config->width, (int32_t)config->height,
            config->grid_color);
    }
    return SIGNAL_RESULT_OK;
}

static signal_result_t DrawDecimated(
    tft_ili9341_t *tft,
    const float *samples,
    size_t sample_count,
    const signal_tft_waveform_config_t *config,
    uint16_t columns,
    float scale_minimum,
    float scale_maximum)
{
    signal_result_t status;
    uint16_t column;
    size_t sample_index;
    int32_t x;
    int32_t y;
    int32_t previous_x = 0;
    int32_t previous_y = 0;

    for (column = 0U; column < columns; ++column) {
        if (columns <= 1U) {
            sample_index = 0U;
        } else {
            sample_index = (size_t)(
                ((uint64_t)column * (uint64_t)(sample_count - 1U)) /
                (uint64_t)(columns - 1U));
        }
        x = PlotX(config, column, columns);
        status = SignalTFTWaveform_MapY(samples[sample_index],
            scale_minimum, scale_maximum, config->y, config->height, &y);
        if (status != SIGNAL_RESULT_OK) {
            return status;
        }
        if (column == 0U) {
            status = TFT_ILI9341_DrawPixel(tft, x, y,
                config->waveform_color);
        } else {
            status = TFT_ILI9341_DrawLine(tft, previous_x, previous_y,
                x, y, config->waveform_color);
        }
        if (status != SIGNAL_RESULT_OK) {
            return status;
        }
        previous_x = x;
        previous_y = y;
    }
    return SIGNAL_RESULT_OK;
}

static signal_result_t DrawEnvelope(
    tft_ili9341_t *tft,
    const float *samples,
    size_t sample_count,
    const signal_tft_waveform_config_t *config,
    uint16_t columns,
    float scale_minimum,
    float scale_maximum)
{
    signal_result_t status;
    uint16_t column;
    float low;
    float high;
    int32_t x;
    int32_t y_low;
    int32_t y_high;

    for (column = 0U; column < columns; ++column) {
        status = SignalTFTWaveform_GetEnvelopeColumn(samples, sample_count,
            columns, column, &low, &high);
        if (status != SIGNAL_RESULT_OK) {
            return status;
        }
        status = SignalTFTWaveform_MapY(low, scale_minimum, scale_maximum,
            config->y, config->height, &y_low);
        if (status != SIGNAL_RESULT_OK) {
            return status;
        }
        status = SignalTFTWaveform_MapY(high, scale_minimum, scale_maximum,
            config->y, config->height, &y_high);
        if (status != SIGNAL_RESULT_OK) {
            return status;
        }
        x = PlotX(config, column, columns);
        status = TFT_ILI9341_DrawLine(tft, x, y_high, x, y_low,
            config->waveform_color);
        if (status != SIGNAL_RESULT_OK) {
            return status;
        }
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalTFTWaveform_Draw(
    tft_ili9341_t *tft,
    const float *samples,
    size_t sample_count,
    const signal_tft_waveform_config_t *config,
    signal_tft_waveform_result_t *result)
{
    signal_result_t status;
    float data_minimum;
    float data_maximum;
    float scale_minimum;
    float scale_maximum;
    uint16_t columns;

    if ((tft == NULL) || (samples == NULL) || (sample_count == 0U) ||
        (config == NULL) || (result == NULL) ||
        (config->width < 2U) || (config->height < 2U) ||
        (config->mode > SIGNAL_TFT_WAVEFORM_MIN_MAX_ENVELOPE) ||
        (config->scale_mode > SIGNAL_TFT_WAVEFORM_AUTO_SCALE)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    status = DataRange(samples, sample_count, &data_minimum, &data_maximum);
    if (status != SIGNAL_RESULT_OK) {
        return status;
    }
    if (config->scale_mode == SIGNAL_TFT_WAVEFORM_AUTO_SCALE) {
        scale_minimum = data_minimum;
        scale_maximum = data_maximum;
        if (!(scale_maximum > scale_minimum)) {
            ExpandFlatRange(data_minimum, &scale_minimum, &scale_maximum);
        }
    } else {
        scale_minimum = config->minimum_value;
        scale_maximum = config->maximum_value;
        if (!isfinite(scale_minimum) || !isfinite(scale_maximum) ||
            !(scale_maximum > scale_minimum)) {
            return SIGNAL_RESULT_INVALID_ARGUMENT;
        }
    }

    columns = (sample_count < (size_t)config->width) ?
        (uint16_t)sample_count : config->width;
    status = DrawDecorations(tft, config, scale_minimum, scale_maximum);
    if (status != SIGNAL_RESULT_OK) {
        return status;
    }
    if (config->mode == SIGNAL_TFT_WAVEFORM_MIN_MAX_ENVELOPE) {
        status = DrawEnvelope(tft, samples, sample_count, config, columns,
            scale_minimum, scale_maximum);
    } else {
        status = DrawDecimated(tft, samples, sample_count, config, columns,
            scale_minimum, scale_maximum);
    }
    if (status != SIGNAL_RESULT_OK) {
        return status;
    }

    result->data_minimum = data_minimum;
    result->data_maximum = data_maximum;
    result->scale_minimum = scale_minimum;
    result->scale_maximum = scale_maximum;
    result->plotted_columns = columns;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalTFTWaveform_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
