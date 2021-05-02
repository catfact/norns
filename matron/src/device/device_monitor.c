/*
 * device_monitor.c
 */

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <libudev.h>
#include <locale.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include "device.h"
#include "device_hid.h"
#include "device_list.h"
#include "device_monome.h"
//#include "device_tty.h"

#include "events.h"

#define DEVICE_MONITOR_DEBUG

#define SUB_NAME_SIZE 32
#define NODE_NAME_SIZE 128
#define WATCH_TIMEOUT_MS 100

// enumerate unix files to watch
enum {
    DEV_FILE_TTY   = 0,
    DEV_FILE_INPUT = 1,
    DEV_FILE_SOUND = 2,
    DEV_FILE_NONE  = 3,
};
#define DEV_FILE_COUNT 3

struct udev_monitor *mon[DEV_FILE_COUNT];

static const char *dev_file_name[] = {
    "tty",
    "input",
    "sound",
    "none"
};


/*
-static struct watch watches[DEV_TYPE_COUNT_PHYSICAL] = {
-    {.sub_name = "tty", .node_pattern = "/dev/ttyUSB*"},
-    {.sub_name = "input", .node_pattern = "/dev/input/event*"},
-    {.sub_name = "sound", .node_pattern = "/dev/snd/midiC*D*"},
-    {.sub_name = "crow", .node_pattern = "/dev/ttyACM*"}};
-
 // file descriptors to watch/poll
*/


//-------------------------
//----- static variables


// file descriptors to watch/poll
struct pollfd pfds[DEV_FILE_COUNT];
// thread for polling all the watched file descriptors
pthread_t watch_tid;

//--------------------------------
//--- static function declarations
static int rm_dev(struct udev_device *dev, int dev_file);
static int rm_dev_tty(struct udev_device *dev, const char *node);

static void add_dev(struct udev_device *dev, int dev_file);
static void add_dev_tty(struct udev_device *dev);
static void add_dev_input(struct udev_device *dev);
static void add_dev_sound(struct udev_device *dev);

static int is_dev_monome_grid(struct udev_device *dev);

static void *watch_loop(void *data);

//static device_t check_dev_type(struct udev_device *dev);

// try to get MIDI device name from ALSA
// returns a newly-allocated string (may be NULL)
static const char *get_alsa_midi_node(struct udev_device *dev);
// try to get product name from udev_device or its parents
// returns a newly-allocated string (may be NULL)
static const char *get_device_name(struct udev_device *dev);

static inline void print_watch_error(const char *msg, int file_idx) {
    fprintf(stderr, "error: %s on subsystem %s", msg, dev_file_name[file_idx]);
}

//--------------------------------
//---- extern function definitions
void dev_monitor_init(void) {
    struct udev *udev = NULL;
    pthread_attr_t attr;
    int s;

    udev = udev_new();
    assert(udev);

    for (int fidx = 0; fidx < DEV_FILE_COUNT; ++fidx) {
	mon[fidx] = NULL;
	struct udev_monitor *m = udev_monitor_new_from_netlink(udev, "udev");
        if (m == NULL) {
	    print_watch_error("couldn't create udev monitor", fidx);
            continue;
        }
        if (udev_monitor_filter_add_match_subsystem_devtype(m, dev_file_name[fidx], NULL) < 0) {
	    print_watch_error("couldn't add subsys filter", fidx);
            continue;
        }
        if (udev_monitor_enable_receiving(m) < 0) {
            print_watch_error("failed to enable monitor", fidx);
            continue;
        }
        pfds[fidx].fd = udev_monitor_get_fd(m);
        pfds[fidx].events = POLLIN;
	mon[fidx] = m;
    }

    s = pthread_attr_init(&attr);
    if (s) {
        fprintf(stderr, "error initializing thread attributes\n");
    }
    s = pthread_create(&watch_tid, &attr, watch_loop, NULL);
    if (s) {
        fprintf(stderr, "error creating thread\n");
    }
    pthread_attr_destroy(&attr);
}

void dev_monitor_deinit(void) {
    pthread_cancel(watch_tid);
    for (int fidx = 0; fidx < DEV_FILE_COUNT; ++fidx) {
	if (mon[fidx] != NULL) { 
	    free(mon[fidx]);
	}
    }
}

// FIXME
int dev_monitor_scan(void) {
    struct udev *udev;
    struct udev_device *dev;

    udev = udev_new();
    if (udev == NULL) {
        fprintf(stderr, "device_monitor_scan(): failed to create udev\n");
        return 1;
    }

    for (int fidx=0; fidx < DEV_FILE_COUNT; ++fidx) {
	struct udev_enumerate *ue;
        struct udev_list_entry *devices, *dev_list_entry;
	struct stat statbuf;
	
	ue = udev_enumerate_new(udev);		
	udev_enumerate_add_match_subsystem(ue, dev_file_name[fidx]);
	udev_enumerate_scan_devices(ue);
	        devices = udev_enumerate_get_list_entry(ue);

        udev_list_entry_foreach(dev_list_entry, devices) {
            const char *path;

            path = udev_list_entry_get_name(dev_list_entry);
	    
	    if (stat(path, &statbuf) < 0 || !S_ISCHR(statbuf.st_mode)) {
		fprintf(stderr, "dev_monitor_scan error: couldn't stat %s\n", path);

		return 0;
	    }

            dev = udev_device_new_from_devnum(udev, 'c', statbuf.st_rdev);

            if (dev != NULL) {
		add_dev(dev, fidx);
            }
	    udev_device_unref(dev);
        }
	
        udev_enumerate_unref(ue);
    }
    return 0;
}
/*
int dev_monitor_scan(void) {
    struct udev *udev;
    struct udev_device *dev;

    udev = udev_new();
    if (udev == NULL) {
        fprintf(stderr, "device_monitor_scan(): failed to create udev\n");
        return 1;
    }

    for (int i = 0; i < DEV_TYPE_COUNT_PHYSICAL; i++) {
        struct udev_enumerate *ue;
        struct udev_list_entry *devices, *dev_list_entry;

        ue = udev_enumerate_new(udev);
        udev_enumerate_add_match_subsystem(ue, watches[i].sub_name);
        udev_enumerate_scan_devices(ue);

        devices = udev_enumerate_get_list_entry(ue);

        udev_list_entry_foreach(dev_list_entry, devices) {
            const char *path;

            path = udev_list_entry_get_name(dev_list_entry);
            dev = udev_device_new_from_syspath(udev, path);

            if (dev != NULL) {
                if (udev_device_get_parent_with_subsystem_devtype(dev, "usb", NULL)) {
                    handle_device(dev);
                }
                udev_device_unref(dev);
            }
        }

        udev_enumerate_unref(ue);
    }
    return 0;
}
*/

//-------------------------------
//--- static function definitions
int rm_dev(struct udev_device *dev, int dev_file) {     
    const char *node = udev_device_get_devnode(dev);
    if (node == NULL ) {
	return 1;
    }
    switch(dev_file) {
    case DEV_FILE_TTY:
	rm_dev_tty(dev, node);
	break;
    case DEV_FILE_INPUT:
	dev_list_remove(DEV_TYPE_HID, node);
	break;
    case DEV_FILE_SOUND:
	dev_list_remove(DEV_TYPE_MIDI, node);
	break;
    case DEV_FILE_NONE:
    default:
	;;
    }
    return 0;
}
 
 int rm_dev_tty(struct udev_device *dev, const char *node) {
     // TODO
     (void)dev;
     (void)node;
     return 0;
 }


 void add_dev(struct udev_device *dev, int fidx) {
#ifdef DEVICE_MONITOR_DEBUG
     const char *node = udev_device_get_devnode(dev);
     fprintf(stderr, "adding device: %s\n", node);
#endif
     
     switch(fidx) {
     case DEV_FILE_TTY:	 
	 add_dev_tty(dev);
	 break;
     case DEV_FILE_INPUT:
	 add_dev_input(dev);
	 break;
     case DEV_FILE_SOUND:
	 add_dev_sound(dev);
	 break;
     case DEV_FILE_NONE:
     default:
	 break;			
     }

 }

 void add_dev_tty(struct udev_device *dev) {
     const char *node = udev_device_get_devnode(dev);

#ifdef DEVICE_MONITOR_DEBUG
     fprintf(stderr, "add_dev_tty: %s\n", node);
#endif

     if (fnmatch("/dev/ttyUSB*", node, 0) == 0) {
#ifdef DEVICE_MONITOR_DEBUG
	 fprintf(stderr, "got ttyUSB, assuming grid\n");
#endif
	 dev_list_add(DEV_TYPE_MONOME, node, get_device_name(dev));
	 return;
     }
     
     if (is_dev_monome_grid(dev)) {
#ifdef DEVICE_MONITOR_DEBUG
     fprintf(stderr, "tty appears to be grid-st\n");
#endif
     dev_list_add(DEV_TYPE_MONOME, node, get_device_name(dev));
     return;
     }

     
#ifdef DEVICE_MONITOR_DEBUG
     fprintf(stderr, "assuming this tty is a crow\n");
#endif
     dev_list_add(DEV_TYPE_CROW, node, get_device_name(dev));
    				
 }
 
 void add_dev_input(struct udev_device *dev) {
     const char *node = udev_device_get_devnode(dev);
     dev_list_add(DEV_TYPE_HID, node, get_device_name(dev));
 }
 
 void add_dev_sound(struct udev_device *dev) {
     // try to act according to
     // https://github.com/systemd/systemd/blob/master/rules/78-sound-card.rules
     const char *alsa_node = get_alsa_midi_node(dev);	 
     if (alsa_node != NULL) {
	 dev_list_add(DEV_TYPE_MIDI, alsa_node, get_device_name(dev));
     }
 }

 
void *watch_loop(void *p) {
    (void)p;
    struct udev_device *dev;

    while (1) {
        if (poll(pfds, DEV_FILE_COUNT, WATCH_TIMEOUT_MS) < 0) {
	    switch (errno) {
            case EINVAL:
                perror("error in poll()");
                exit(1);
            case EINTR:
            case EAGAIN:
                continue;
            }
        }

        for (int fidx = 0; fidx < DEV_FILE_COUNT; ++fidx) {
            if (pfds[fidx].revents & POLLIN) {
                dev = udev_monitor_receive_device(mon[fidx]);
                if (dev) {
		    const char *action = udev_device_get_action(dev);
		    if (action != NULL) {
			if (strcmp(action, "remove") == 0) {
			    rm_dev(dev, fidx);
			} else {
			    add_dev(dev, fidx);
			}			
		    } else {
			fprintf(stderr, "dev_monitor error: unknown device action\n");
		    }
                    udev_device_unref(dev);
                } else {
                    fprintf(stderr, "dev_monitor error: no device data\n");
                }
            }
        }
    }
}



 /*
void handle_device(struct udev_device *dev) {
    const char *action = udev_device_get_action(dev);
    const char *node = udev_device_get_devnode(dev);
    const char *subsys = udev_device_get_subsystem(dev);

    if (action == NULL) {
        // scan
        if (node != NULL) {
            device_t t = check_dev_type(dev);

            if (t >= 0 && t < DEV_TYPE_COUNT_PHYSICAL) {
                dev_list_add(t, node, get_device_name(dev));
            }
        }
    } else {
        // monitor
        if (strcmp(subsys, "sound") == 0) {
            // try to act according to
            // https://github.com/systemd/systemd/blob/master/rules/78-sound-card.rules
            if (strcmp(action, "change") == 0) {
                const char *alsa_node = get_alsa_midi_node(dev);

                if (alsa_node != NULL) {
                    dev_list_add(DEV_TYPE_MIDI, alsa_node, get_device_name(dev));
                }
            } else if (strcmp(action, "remove") == 0) {
                if (node != NULL) {
                    dev_list_remove(DEV_TYPE_MIDI, node);
                }
            }
        } else {
            device_t t = check_dev_type(dev);

            if (t >= 0 && t < DEV_TYPE_COUNT_PHYSICAL) {
                strcmp (if(action, "add") == 0) {
                    dev_list_add(t, node, get_device_name(dev));
                } else if (strcmp(action, "remove") == 0) {
                    dev_list_remove(t, node);
                }
            }
        }
    }
}

 */

 
/*
device_t check_dev_type(struct udev_device *dev) {
    device_t t = DEV_TYPE_INVALID;
    const char *node = udev_device_get_devnode(dev);

    if (node) {
        // for now, just get USB devices.
        // eventually we might want to use this same system for GPIO, &c...
        if (udev_device_get_parent_with_subsystem_devtype(dev, "usb", NULL)) {
            for (int i = 0; i < DEV_TYPE_COUNT_PHYSICAL; i++) {
                const char *node_pattern = watches[i].node_pattern;
                if (node_pattern[0] && fnmatch(node_pattern, node, 0) == 0) {
                    t = i;
                    break;
                }
            }
        }
    }
    return t;
}
*/
 
// try to get midi device node from udev_device
const char *get_alsa_midi_node(struct udev_device *dev) {
    const char *subsys;
    const char *syspath;
    DIR *sysdir;
    struct dirent *sysdir_ent;
    int alsa_card, alsa_dev;
    char *result = NULL;

    subsys = udev_device_get_subsystem(dev);

    if (strcmp(subsys, "sound") == 0) {
        syspath = udev_device_get_syspath(dev);
        sysdir = opendir(syspath);

        while ((sysdir_ent = readdir(sysdir)) != NULL) {
            if (sscanf(sysdir_ent->d_name, "midiC%uD%u", &alsa_card, &alsa_dev) == 2) {
                if (asprintf(&result, "/dev/snd/%s", sysdir_ent->d_name) < 0) {
                    fprintf(stderr, "failed to create alsa device path for %s\n", sysdir_ent->d_name);
                    return NULL;
                }
            }
        }
    }

    return result;
}

const char *get_device_name(struct udev_device *dev) {
    char *current_name = NULL;
    struct udev_device *current_dev = dev;

    while (current_name == NULL) {
        current_name = (char *)udev_device_get_sysattr_value(current_dev, "product");
        current_dev = udev_device_get_parent(current_dev);

        if (current_dev == NULL) {
            break;
        }
    }

    return strdup(current_name);
}

 int is_dev_monome_grid(struct udev_device *dev) {
	const char *vendor, *model;

	vendor = udev_device_get_property_value(dev, "ID_VENDOR");
	model = udev_device_get_property_value(dev, "ID_MODEL");

	printf("vendor: %s; model: %s\n\n", vendor, model);
	
	if (vendor != NULL && model != NULL) { 
	    return (strcmp(vendor,"monome")==0) && (strcmp(model,"grid")==0);
	} else {
	    return 0;
	}
 }
