// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LIGHT

#include <pbdrv/light.h>
#include <pbdrv/pwm.h>
#include <pbio/color.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, const pbdrv_light_raw_rgb_t *raw) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Timers have period of 10000 and we want period / 5 as max brightness.

    pbdrv_pwm_dev_t *dev;
    if (pbdrv_pwm_get_dev(1, &dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(dev, 2, raw->r * 2000 / 256);
    }
    if (pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(dev, 4, raw->g * 2000 / 256);
    }
    if (pbdrv_pwm_get_dev(2, &dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(dev, 1, raw->b * 2000 / 256);
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_color_t color,
    pbdrv_light_raw_rgb_t *raw) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    pbio_color_rgb_t rgb;
    pbio_color_to_rgb(color, &rgb);

    // Adjust for chromacity
    uint32_t r = rgb.r * 1000;
    uint32_t g = rgb.g * 270;
    uint32_t b = rgb.b * 200;

    // Adjust for apparent brightness
    // + 1 protects against division by zero
    uint32_t Y = ((174 * r + 1590 * g + 327 * b) >> 16) + 1;
    raw->r = r / Y;
    raw->g = g / Y;
    raw->b = b / Y;

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_LIGHT
