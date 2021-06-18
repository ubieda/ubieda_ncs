#ifndef _MOTOR_CONTROLLER_H_
#define _MOTOR_CONTROLLER_H_

#include <device.h>

int motor_controller_init(void);
int motor_controller_set(int8_t speed); /**< Range: -100 to 100 */

#endif /* _MOTOR_CONTROLLER_H_ */
