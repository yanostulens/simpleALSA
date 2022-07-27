/**
 * @file simpleALSA.h
 * @author Yano Stulens (yano.stulens00@gmail.com)
 * @author Daan Witters (daanwitters@gmail.com)
 * @brief An easy to use wrapper around the famously complicated ALSA API's for
 * audio playback
 * @version 0.1.1
 * @date 2022-07-26
 * @link https://github.com/yanostulens/simpleALSA @endlink
 * @copyright Copyright (c) 2022 Yano Stulens - Daan Witters
 *
 * For examples and use - check the examples on the github page listed above.
 * Have fun!
 */
/**
==========================================================================================
                                       LICENSE
==========================================================================================
Copyright © 2022 Yano Stulens
Copyright © 2022 Daan Witters

This software is release under the Creative Commons Attribution 4.0 International license.

For detailed information about the CC BY license please visit:
https://creativecommons.org/licenses/by/4.0/

In a short, human version this means that:

You are free to:
Share - copy and redistribute the material in any medium or format.
Adapt - remix, transform, and build upon the material for any purpose,
even commercially.

Under the following terms:
Attribution — You must give appropriate credit, provide a link to the license,
and indicate if changes were made. You may do so in any reasonable manner, but not in any
way that suggests the licensor endorses you or your use.
No additional restrictions — You may not apply legal terms or technological measures
that legally restrict others from doing anything the license permits.

You do not have to comply with the license for elements of the material in the public
domain or where your use is permitted by an applicable exception or limitation.
No warranties are given. The license may not give you all of the permissions necessary
for your intended use. For example, other rights such as publicity, privacy, or moral
rights may limit how you use the material.


==========================================================================================
                                      DISCLAIMER
==========================================================================================
THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
**/

#ifndef SIMPLEASLA_H_
    #define SIMPLEALSA_H_

    #include <alsa/asoundlib.h>

    #include "../config.h"

/** FUNCTIONS DEFINITIONS **/

/**
 * @brief creates an sa_device_config struct and sets it to default values
 * @param device - empty pointer into which the config struct is set
 * @return sa_return_status
 */
sa_result sa_init_device_config(sa_device_config **config);

/**
 * @brief initializes a new audio device
 * @param config - configuration used to initialize the device
 * @param device - pointer to the initialized audio device
 * @return sa_return_status
 */
sa_result sa_init_device(sa_device_config *config, sa_device **device);

/**
 * @brief starts the simple ALSA device - which starts the callback loop
 *
 * @param device - device to start
 * @return sa_return_status
 */
sa_result sa_start_device(sa_device *device);

/**
 * @brief pauses the simple ALSA device - which pauses the callback loop
 *
 * @param device - device to pause
 * @return sa_return_status
 */
sa_result sa_pause_device(sa_device *device);

/**
 * @brief stops a simple ALSA device - same as pause but in addation, all buffer data is dropped
 *
 * @param device - device to stop
 * @return sa_return_status
 */
sa_result sa_stop_device(sa_device *device);

/**
 * @brief destroys the device - device pointer is set to NULL
 *
 * @param device - the device to destroy
 * @return sa_return_status
 */
sa_result sa_destroy_device(sa_device *device);

#endif  // SIMPLEALSA_H_