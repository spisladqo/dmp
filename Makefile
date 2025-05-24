KDIR := /usr/src/kernels/$(shell uname -r)

all: build

build:
	$(MAKE) -j -C $(KDIR) M=$(PWD)

clean:
	$(MAKE) -j -C $(KDIR) M=$(PWD) clean
