all:
	west build -b stamp_c3 app
flash:
	west flash
monitor:
	west espressif monitor
clean:
	rm -rf build
