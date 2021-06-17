#ifndef _PWM_DUAL_H_
#define _PWM_DUAL_H_

#include <stdint.h>
#include <device.h>

struct pwm_dual_duty_cycle {
    uint32_t period;
    struct {
        uint32_t pin;
        uint32_t pulse;
    } ch0;
    struct {
        uint32_t pin;
        uint32_t pulse;
    } ch1;
};

int pwm_dual_set_usec(
        const struct device *dev, 
        struct pwm_dual_duty_cycle * setting);

#endif /* _PWM_DUAL_H_ */