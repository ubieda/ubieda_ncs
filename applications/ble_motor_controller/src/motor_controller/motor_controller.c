#include <motor_controller.h>
#include <drivers/pwm_dual.h>

#define PERIOD_USEC	20000U

static const char * device_name = "PWM_0";
static const struct device * pwm;

int motor_controller_set(int8_t speed)
{
    int err = 0;

    struct pwm_dual_duty_cycle duty_cycle = {
        .period = PERIOD_USEC,
        .ch0 = {
            .pin = 24,
            .pulse = 0U,
        },
        .ch1 = {
            .pin = 44,
            .pulse = 0U,
        },
    };
    
    uint32_t pulse_width = 0;
    if(speed > 0 && speed <= 100){
        pulse_width = PERIOD_USEC *speed/100;
        duty_cycle.ch0.pulse = pulse_width;
    }
    else if(speed < 0 && speed >= -100){
        pulse_width = PERIOD_USEC *(256 - ((uint8_t)speed) )/100;        
        duty_cycle.ch1.pulse = pulse_width;
    }
    else{
        err = -1;        
    }

    pwm_dual_set_usec(pwm,&duty_cycle);

    return err;
}

int motor_controller_init(void)
{
    int err = -1;

    pwm = device_get_binding(device_name);
    if(!pwm)
        return err;

    err = motor_controller_set(0);

    return 0;
}
