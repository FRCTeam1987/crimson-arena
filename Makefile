ASSET_FILES = LICENSE README.md access_point_config.tar.gz fix_avatar_colors_for_overlay font schedules static switch_config.txt templates tunnel

all:
	rm -rf crimson-arena*
	go clean
	mkdir crimson-arena
	go build -o crimson-arena/
	cp -r $(ASSET_FILES) crimson-arena/

package:
	@./package