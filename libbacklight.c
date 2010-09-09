#define _GNU_SOURCE

#include <libbacklight.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>

static const char *output_names[] = { "None",
                                      "VGA",
                                      "DVI",
                                      "DVI",
                                      "DVI",
                                      "Composite",
                                      "TV",
                                      "LVDS",
                                      "CTV",
                                      "DIN",
                                      "DP",
                                      "HDMI",
                                      "HDMI",
};

static long backlight_get(struct backlight *backlight, char *node)
{
	char buffer[100];
	char *path;
	int fd;
	long value, ret;

	asprintf(&path, "%s/%s", backlight->path, node);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		ret = -1;
		goto out;
	}

	ret = read(fd, &buffer, sizeof(buffer));
	if (ret < 1) {
		ret = -1;
		goto out;
	}

	value = strtol(buffer, NULL, 10);
	ret = value;
out:
	close(fd);
	if (path)
		free(path);
	return ret;
}

long backlight_get_brightness(struct backlight *backlight)
{
	return backlight_get(backlight, "brightness");
}

long backlight_get_max_brightness(struct backlight *backlight)
{
	return backlight_get(backlight, "max_brightness");
}

long backlight_get_actual_brightness(struct backlight *backlight)
{
	return backlight_get(backlight, "actual_brightness");
}

long backlight_set_brightness(struct backlight *backlight, long brightness)
{
	char *path;
	char *buffer = NULL;
	int fd;
	long value, ret;

	asprintf(&path, "%s/%s", backlight->path, "brightness");
	fd = open(path, O_RDWR);
	if (fd < 0) {
		ret = -1;
		goto out;
	}

	ret = read(fd, &buffer, sizeof(buffer));
	if (ret < 1) {
		ret = -1;
		goto out;
	}

	asprintf(&buffer, "%ld", brightness);
	ret = write(fd, buffer, strlen(buffer));
	if (ret < 0) {
		ret = -1;
		goto out;
	}

	ret = backlight_get_brightness(backlight);
out:
	if (buffer)
		free(buffer);
	if (path)
		free(path);
	close(fd);
	return ret;
}

struct backlight *backlight_init(struct pci_device *dev, int card,
				 int connector_type, int connector_type_id)
{
	char *pci_name;
	char *drm_name;
	char *chosen_path = NULL;
	DIR *backlights;
	struct dirent *entry;
	enum backlight_type type = 0;
	char buffer[100];
	struct backlight *backlight;

	asprintf(&pci_name, "%04x:%02x:%02x.%d", dev->domain, dev->bus,
		 dev->dev, dev->func);
	asprintf(&drm_name, "card%d-%s-%d", card, output_names[connector_type],
		 connector_type_id);

	backlights = opendir("/sys/class/backlight");

	if (!backlights)
		return NULL;

	while ((entry = readdir(backlights))) {
		char *backlight_path;
		char *parent;
		char *path;
		enum backlight_type entry_type;
		int fd, ret;

		if (entry->d_name[0] == '.')
			continue;

		asprintf(&backlight_path, "%s/%s", "/sys/class/backlight",
			 entry->d_name);
		asprintf(&path, "%s/%s", backlight_path, "type");
		fd = open(path, O_RDONLY);

		if (fd < 0)
			goto out;

		ret = read (fd, &buffer, sizeof(buffer));
		close (fd);

		if (ret < 1)
			goto out;

		buffer[ret] = '\0';

		if (!strncmp(buffer, "raw\n", sizeof(buffer)))
			entry_type = BACKLIGHT_RAW;
		else if (!strncmp(buffer, "platform\n", sizeof(buffer)))
			entry_type = BACKLIGHT_PLATFORM;
		else if (!strncmp(buffer, "firmware\n", sizeof(buffer)))
			entry_type = BACKLIGHT_FIRMWARE;
		else
			goto out;

		free (path);
		asprintf(&path, "%s/%s", backlight_path, "device");
		ret = readlink(path, buffer, sizeof(buffer));

		if (ret < 0)
			goto out;

		parent = basename(buffer);

		if (entry_type == BACKLIGHT_RAW) {
			if (strcmp(drm_name, parent) &&
			    strcmp(pci_name, parent)) {
				goto out;
			}
		}

		if (entry_type == BACKLIGHT_FIRMWARE) {                   
			/* Older kernels won't provide a valid path here... */
			unsigned int domain, bus, device, function;
			ret = sscanf(parent, "%04x:%02x:%02x.%u", &domain, &bus,
				     &device, &function);
			if (ret == 4) {                   
				if (strcmp(pci_name, parent))
					goto out;
			}
		}

		if (entry_type < type)
			goto out;

		type = entry_type;

		if (chosen_path)
			free(chosen_path);
		chosen_path = strdup(backlight_path);

	out:
		free(backlight_path);
		free(path);
	}

	if (!chosen_path)
		return NULL;

	backlight = malloc(sizeof(struct backlight));

	if (!backlight)
		goto err;

	backlight->path = chosen_path;
	backlight->type = type;

	backlight->max_brightness = backlight_get_max_brightness(backlight);
	if (backlight->max_brightness < 0)
		goto err;

	backlight->brightness = backlight_get_actual_brightness(backlight);
	if (backlight->brightness < 0)
		goto err;

	return backlight;
err:
	if (chosen_path)
		free(chosen_path);
	free (backlight);
	return NULL;
}
