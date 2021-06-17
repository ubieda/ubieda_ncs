#include <zephyr.h>
#include <drivers/pwm.h>
#include <drivers/pwm_dual.h>
#include <device.h>

const char * pwm_name = "PWM_0";

#include <logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

/*
 * This period should be fast enough to be above the flicker fusion
 * threshold. The steps should also be small enough, and happen
 * quickly enough, to make the output fade change appear continuous.
 */
#define PERIOD_USEC	20000U
#define NUM_STEPS	50U
#define STEP_USEC	(PERIOD_USEC / NUM_STEPS)
#define SLEEP_MSEC	25U

void main(void)
{
    LOG_INF("Basic PWM Sample started now!");
    
    int err = -1;

    const struct device * pwm;
	uint32_t pulse_width = 0U;
	uint8_t dir = 1U;
	bool direction_fwd = false;

    pwm = device_get_binding(pwm_name);
    LOG_INF("PWM binding result: %s",pwm? "ok":"failed");
    if(!pwm)
        return;

	while (1) {
		struct pwm_dual_duty_cycle duty_cycle = {
			.period = PERIOD_USEC,
			.ch0 = {
				.pin = 24,
				.pulse = 0,
			},
			.ch1 = {
				.pin = 44,
				.pulse = 0,
			},
		};
		if(direction_fwd){
			duty_cycle.ch0.pulse = pulse_width;
		}
		else{
			duty_cycle.ch1.pulse = pulse_width;
		}
		LOG_INF("Setting - dir: %s, pulse_width: %d",dir?"y":"n",pulse_width);
		err = pwm_dual_set_usec(pwm,&duty_cycle);
		// err = pwm_pin_set_usec(pwm, PWM_CHANNEL, PERIOD_USEC,
		// 		       pulse_width, PWM_FLAGS);
		if (err) {
			LOG_INF("Error %d: failed to set pulse width\n", err);
			return;
		}

		if (dir) {
			pulse_width += STEP_USEC;
			if (pulse_width >= PERIOD_USEC) {
				pulse_width = PERIOD_USEC;
				dir = 0U;
			}
		} else {
			if (pulse_width >= STEP_USEC) {
				pulse_width -= STEP_USEC;
			} else {
				pulse_width = 0;
				dir = 1U;
				direction_fwd = !direction_fwd;
			}
		}

		k_sleep(K_MSEC(SLEEP_MSEC));
	}

}