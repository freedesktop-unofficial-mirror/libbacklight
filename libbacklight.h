#include <pciaccess.h>

enum backlight_type {
	BACKLIGHT_RAW,
	BACKLIGHT_PLATFORM,
	BACKLIGHT_FIRMWARE,
};

struct backlight {
	char *path;
	int max_brightness;
	int brightness;
	enum backlight_type type;
};

void backlight_destroy(struct backlight *backlight);
struct backlight *backlight_init(struct pci_device *dev, int card,
				 int connector_type, int connector_type_id);
long backlight_get_brightness(struct backlight *backlight);
long backlight_get_max_brightness(struct backlight *backlight);
long backlight_get_actual_brightness(struct backlight *backlight);
long backlight_set_brightness(struct backlight *backlight, long brightness);
