#include <drivers/pwm_dual.h>
#include <drivers/pwm.h>

int pwm_dual_set_usec(
        const struct device *dev, 
        struct pwm_dual_duty_cycle * setting)
{
    int err = -1;

    err = pwm_pin_set_usec(dev, setting->ch0.pin, setting->period,
    		       setting->ch0.pulse, 0);
    if(err)
        return err;

    err = pwm_pin_set_usec(dev, setting->ch1.pin, setting->period,
    		       setting->ch1.pulse, 0);

    return err;
}
