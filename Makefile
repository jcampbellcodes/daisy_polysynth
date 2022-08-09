all: daisy_libs pluck

pluck: 
	$(MAKE) -C src

daisy_libs:
	./rebuild_daisy_libs.sh

clean:
	$(MAKE) -C src clean
	$(MAKE) -C libdaisy clean
	$(MAKE) -C DaisySP clean

program: 
	$(MAKE) -C src program

program-dfu: 
	$(MAKE) -C src program-dfu
     