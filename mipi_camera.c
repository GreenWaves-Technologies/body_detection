#include "pmsis.h"
#include "mipi_camera.h"
#include "bsp/camera.h"

#ifdef OV5647
#include "bsp/camera/ov5647.h"
#else
#include "bsp/camera/ov9281.h"
#endif

#define PAD_GPIO_LED2    (PI_PAD_086)
pi_device_t gpio_led;

struct pi_device camera;
pi_task_t cam_wait_task;

static int __write_reg8(pi_device_t *dev, uint8_t addr, uint8_t value)
{
    uint8_t buffer[2] = {addr, value};
    if (pi_i2c_write(dev, buffer, 2, PI_I2C_XFER_START | PI_I2C_XFER_STOP))
    {
        printf("Error when writing \n");
        return -1;
    }
    return 0;
}

static uint8_t __read_reg8(pi_device_t *dev, uint8_t addr)
{
    uint8_t result;
    pi_i2c_write_read(dev, &addr, &result, 1, 1);
    return result;
}

// Setup FXL6408 GPIO Expander
// Datasheet https://www.onsemi.com/pdf/datasheet/fxl6408-d.pdf
static int fxl6408_setup()
{
    pi_device_t i2c = {0};
    struct pi_i2c_conf i2c_conf;
    pi_i2c_conf_init(&i2c_conf);
    i2c_conf.itf = 3;
    i2c_conf.max_baudrate = 100000;
    pi_i2c_conf_set_slave_addr(&i2c_conf, 0x88, 0);

    pi_open_from_conf(&i2c, &i2c_conf);

    if (pi_i2c_open(&i2c))
    {
        printf("*** TEST FAILED ***\n");
        return -1;
    }
    // Turn-on fxl6408
    __write_reg8(&i2c, 0x01, 0x01); // SW reset the GPIO expander
    __write_reg8(&i2c, 0x03, 0x01); // Direction: GPIO0 -> Output
    __write_reg8(&i2c, 0x05, 0x01); // Output state: GPIO0 -> High
    __write_reg8(&i2c, 0x07, 0x00); // Output state follow the 0x05 (above)

    return 0;
}

int open_camera_csi2()
{   
    struct pi_device gpio_ic_3v3;
    
    // Going to I2C3 to access IO expander
    pi_pad_set_function(PI_PAD_046, PI_PAD_FUNC2); // I2C3 SDA
    pi_pad_set_function(PI_PAD_047, PI_PAD_FUNC2); // I2C3 SCL

    struct pi_gpio_conf gpio_conf = {0};
    pi_gpio_e gpio_pin_o = PI_GPIO_A00;

    pi_gpio_pin_write(&gpio_ic_3v3, gpio_pin_o, 1);
    pi_time_wait_us(1000000);

    fxl6408_setup();

    // Coming back to I3C to open the camera
    pi_pad_set_function(PI_PAD_046, PI_PAD_FUNC0); // I3C SDA
    pi_pad_set_function(PI_PAD_047, PI_PAD_FUNC0); // I3C SCL

    #ifdef OV5647
	struct pi_ov5647_conf cam_conf;
	pi_ov5647_conf_init(&cam_conf);
    #else
	struct pi_ov9281_conf cam_conf;
	pi_ov9281_conf_init(&cam_conf);
    #endif

    cam_conf.format = CAM_FORMAT;
	pi_open_from_conf(&camera, &cam_conf);
	if (pi_camera_open(&camera))
    {
        return -1;
    }

    return 0;
}

void ov9281_capture(uint8_t *ImageInChar)
{
    pi_camera_capture_async(&camera, ImageInChar, Wcam*Hcam, pi_task_block(&cam_wait_task));           
    pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
    pi_task_wait_on(&cam_wait_task);
    pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);

}

int led2_init()
{
    struct pi_gpio_conf gpio_conf;

    pi_gpio_conf_init(&gpio_conf);

    gpio_conf.port = PAD_GPIO_LED2 / 32;

    pi_open_from_conf(&gpio_led, &gpio_conf);

    if (pi_gpio_open(&gpio_led))
    {
        return -1;
    }

    /* set pad to gpio mode */
    pi_pad_set_function(PAD_GPIO_LED2, PI_PAD_FUNC1);

    /* configure gpio output */
    pi_gpio_flags_e flags = PI_GPIO_OUTPUT;
    pi_gpio_pin_configure(&gpio_led, PAD_GPIO_LED2, flags);
    
    return 0;
}

void led2_toggle()
{
    pi_gpio_pin_toggle(&gpio_led, PAD_GPIO_LED2);
}
        
     
       
