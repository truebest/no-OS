
INCS += $(PLATFORM_DRIVERS)/$(PLATFORM)_i2c.h		\
	$(PLATFORM_DRIVERS)/$(PLATFORM)_uart.h		\
	$(PLATFORM_DRIVERS)/$(PLATFORM)_platform.h	\

SRCS += $(PLATFORM_DRIVERS)/$(PLATFORM)_delay.c		\
        $(PLATFORM_DRIVERS)/$(PLATFORM)_i2c.c		\
	$(PLATFORM_DRIVERS)/$(PLATFORM)_uart.c		\
	$(PLATFORM_DRIVERS)/$(PLATFORM)_platform.c	\

