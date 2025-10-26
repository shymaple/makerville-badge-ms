all:
	dts-linter --formatFixAll --file app/boards/esp32c3_devkitm.overlay
	west build -b esp32c3_devkitm app
flash:
	west flash
monitor:
	west espressif monitor
clean:
	rm -rf build
