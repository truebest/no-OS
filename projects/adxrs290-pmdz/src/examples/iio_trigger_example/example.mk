IIOD=y

CFLAGS +=-DIIO_IGNORE_BUFF_OVERRUN_ERR

INCS += $(DRIVERS)/gyro/adxrs290/iio_adxrs290.h
SRCS += $(DRIVERS)/gyro/adxrs290/iio_adxrs290.c \
	$(DRIVERS)/gyro/adxrs290/iio_adxrs290_trig.c