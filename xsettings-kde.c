/*
 * Copyright (c) 2001 Red Hat, Inc.
 *           (c) 2005, 2006, 2007, 2008 Mandriva
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Red Hat not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Red Hat makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL RED HAT
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Owen Taylor, Red Hat, Inc.
 *           Frederic Crozat, Mandriva
 *
 */
#define _GNU_SOURCE 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <glib.h>
#include <gio/gio.h>

#include "xsettings-manager.h"

#define IMMODULE_SCHEMA       "org.gnome.desktop.interface"
#define IMMODULE_KEY          "gtk-im-module"

static XSettingsManager **managers;
static int max_display;
static int is_kde4;

void
terminate_cb (void *data)
{
}
		    
/*
 * X Settings existing
 * 
 * Net/DoubleClickTime => supported
 * Net/DoubleClickDistance => can't be configured in KDE 
 * Net/DndDragThreshold => supported
 * Net/CursorBlink => can't be configured in KDE
 * Net/CursorBlinkTime => can be only configured in QT, no KIPC involved, no real added value
 * Net/ThemeName => supported
 * Net/IconThemeName => supported
 * Net/FallbackIconTheme => supported
 * Gtk/CanChangeAccels => not supported
 * Gtk/ColorPalette => no added value
 * Gtk/CursorThemeName => supported
 * Gtk/FontName => supported
 * Gtk/IconSizes => not really useful
 * Gtk/KeyThemeName => not supported
 * Gtk/ToolbarStyle => supported
 * Gtk/ToolbarIconSize => implemented but disabled, doesn't seem to work in GTK+
 * Gtk/IMPreeditStyle => not supported
 * Gtk/IMStatusStyle => not supported
 * Gtk/IMModule => supported
 * Gtk/Modules => not supported
 * Gtk/FileChooserBackend => forced to gnome-vfs
 * Gtk/ButtonImages => supported
 * Gtk/MenuImages => can't be configured in KDE, always set to true
 * Gtk/MenuBarAccel => not supported
 * Xft/Antialias => KDE uses .fonts.conf directly
 * Xft/Hinting => KDE uses .fonts.conf directly
 * Xft/HintStyle => KDE uses .fonts.conf directly
 * Xft/RGBA => KDE uses .fonts.conf directly
 * Xft/DPI => supported
 */

static Atom* atoms[2];

static char * atom_names [2] = {
	"KIPC_COMM_ATOM",
	"KDE_DESKTOP_WINDOW"
	};

enum Section {
	Unknown = 0,
	General,
	Toolbar,
	KDE,
	Icons,
	MainToolbarIcons,
	Directories,
	Mouse
};

static char * file_name [3] = {
	"kdeglobals",
	"kcmfionts",
	"kcminputrc"
};

int readString (char *key, char * buffer, char *xsetting_key) {
	if (strncmp (buffer, key, strlen (key)) == 0) {
		char tmp_string[1024];
		char search_string[1024];

		sprintf(search_string, "%s=%%s", key);
		if (sscanf (buffer, search_string, tmp_string) >= 0) {
			int i;
			for (i = 0 ; i < max_display ; i++) {
 				xsettings_manager_set_string (managers[i], xsetting_key, tmp_string);
			}
			return 1;
		}
	}
	return 0;
}

int readInt (char *key, char * buffer, char *xsetting_key, int multiplier) {
	if (strncmp (buffer, key, strlen (key)) == 0) {
		int tmp_int;
		char search_string[1024];

		sprintf(search_string, "%s=%%d", key);
		if (sscanf (buffer, search_string, &tmp_int) >= 0) {
			int i;
			for (i = 0 ; i < max_display ; i++) {
				xsettings_manager_set_int (managers[i], xsetting_key, tmp_int*multiplier);
			}
			return 1;
		}
	}
	return 0;
}

int readBoolean (char *key, char * buffer, char *xsetting_key) {
	if (strncmp (buffer, key, strlen (key)) == 0) {
		char tmp_string[1024];
		char search_string[1024];

		sprintf(search_string, "%s=%%s", key);
		if (sscanf (buffer, search_string, &tmp_string) >= 0) {
			int i;

			if (strncmp(tmp_string, "1",1) == 0 || strncasecmp (tmp_string, "true", 4) == 0) {
				for (i = 0 ; i < max_display ; i++) {
					xsettings_manager_set_int (managers[i], xsetting_key, 1);
				}
			}
			else {
				for (i = 0 ; i < max_display ; i++) {
					xsettings_manager_set_int (managers[i], xsetting_key, 0);
				}
			}
			return 1;
		}
	}
	return 0;
}


int readDPI(char *buffer)
{
	const char *key = "forceFontDPI";
	const char *xsetting_key = "Xft/DPI";

	if (strncmp (buffer, key, strlen(key)) == 0) {
		int tmp_int;
		char search_string[1024];

		sprintf(search_string, "%s=%%d", key);
		if (sscanf (buffer, search_string, &tmp_int) >= 0) {
			int i;
			/* GTK+ expects this as 1024 * dots/inch */
			tmp_int *= 1024;

			for (i = 0 ; i < max_display ; i++) {
				if (tmp_int) {
					xsettings_manager_set_int (managers[i], xsetting_key, tmp_int);
				} else {
					xsettings_manager_delete_setting (managers[i], xsetting_key);
				}
			}
			return 1;
		}
	}
	return 0;
}

static gboolean
check_gsettings_schema (const gchar *schema)
{
  const gchar * const *schemas = g_settings_list_schemas ();
  gint i;

    for (i = 0; schemas[i] != NULL; i++) {
      if (g_strcmp0 (schemas[i], schema) == 0)
            return TRUE;
  }
  g_warning ("Settings schema '%s' is not installed.", schema);

    return FALSE;
}

static void
gsettings_changed (GSettings  *settings,
         const char *key,
         gpointer    user_data)
{
    gchar *val;
    gint i;

  if (g_strcmp0 (key, IMMODULE_KEY)) {
        /* deal with Gtk/IMModule only so far */
      return;
    }

  val = g_settings_get_string (settings, key);
    for (i = 0; i < max_display; i++) {                         
      xsettings_manager_set_string (managers[i], "Gtk/IMModule", val);
      xsettings_manager_notify (managers[i]);
  }
  g_free(val);
}

void readConfig () {
	FILE *file = NULL;
	char *buffer = NULL;
	size_t len = 0, read;
	enum Section section = Unknown;
	int notify = 0, buttonIconSet = 0;
	char filename[1024];
	char kdeprefix[1024];
	char *prefix;
	char color[1024];
	char style[1024];
	char themefilename[1024];
	struct passwd *password;
	int i;
	int user_file;
	int file_index;

	color[0]= '\0';
	kdeprefix[0]= '\0';
	filename[0]= '\0';
	style[0]='\0';
	prefix = NULL;

        file = fopen (is_kde4 ? "/etc/kde4rc" : "/etc/kderc", "r");
	if (file) {
		while ((read = getline (&buffer,&len, file)) != -1) {
			char tmp_string[1024];

			if (buffer[0] == '[') {
				if (strncmp(buffer, "[Directories-default]", 21) == 0)
					section = Directories;
				continue;
			}
			switch (section) {
				case Directories:
					if (sscanf (buffer, "prefixes=%s", tmp_string) >= 0) {
						strcpy(kdeprefix, tmp_string);
					}
					break;
				default:
					break;
			}

		}
		fclose (file);
		file = NULL;
	}

      for (file_index = 0 ; file_index < 3 ; file_index++) {

	user_file = 0;
	do {
		do {
			if (kdeprefix[0]) {
				if (prefix == NULL) {
					prefix = strtok (kdeprefix, ",");
				}
				else {
					prefix = strtok (NULL, ",");
				}
				if (prefix != NULL) {
					sprintf(filename, "%s/share/config/%s",prefix, file_name[file_index]);
					if (access (filename, F_OK) == 0) {
						file = fopen (filename, "r");
						if (file != NULL)
							break;
					}
				}
			}
		} while (prefix != NULL );

	if ((file == NULL) && !user_file) {

		user_file = 1;
		if ((password = getpwuid (geteuid()))) {
			sprintf(filename, "%s/%s/share/config/%s",password->pw_dir, is_kde4 ? ".kde4" : ".kde", file_name[file_index]);

			if (access (filename, F_OK) == 0) {
				file = fopen (filename, "r");
			}
		}
	}

	section = Unknown;

	while (file && (read = getline (&buffer,&len, file)) != -1) {
		if (buffer[0] == '[') {
			if (strncmp(buffer, "[General]", 9) == 0)
				section = General;
			else { if (strncmp(buffer, "[Toolbar style]", 15) == 0)
				section = Toolbar;
				else {
					if (strncmp(buffer, "[KDE]", 5) == 0)
						section = KDE;
					else {
						if (strncmp(buffer, "[Icons]", 7) == 0)
							section = Icons;
						else { 
							if (strncmp(buffer, "[MainToolbarIcons]", 18) == 0)
								section = MainToolbarIcons;
							else {
								if (strncmp(buffer, "[Mouse]", 7) == 0)
                               						section = Mouse;
                               					else
                               						section = Unknown;
							}
						}
					}
				}
			}
			continue;
		}
		switch (section) {
				/* doesn't seem to work in GTK */
			/*
			case MainToolbarIcons:
				if (strncmp (buffer, "Size=", 5) == 0) {
					int size;

					if (sscanf (buffer, "Size=%d", &size) >= 0) {
						char new_size[1024];
						sprintf(new_size, "%d",size);
						xsettings_manager_set_string(manager, "Gtk/ToolbarIconSize", "large-toolbar");
						notify = 1;
					}
				}
				break;
*/

			case Icons:
				notify |= readString ("Theme",buffer, "Net/IconThemeName");
				break;
				
			case KDE:
				notify |= readInt ("DoubleClickInterval", buffer, "Net/DoubleClickTime", 1);

				notify |= readInt ("StartDragDist", buffer, "Net/DndDragThreshold", 1);
				
				buttonIconSet |= readBoolean("ShowIconsOnPushButtons", buffer, "Gtk/ButtonImages");
				notify |= buttonIconSet;

				break;
			case General: 
				if (strncmp(buffer,"font=",5) == 0) {
					char font[1024];
					char size[1024];
					char fontname[1024];

					/* found font */
					sscanf(buffer,"font=%[^,],%[^,]", font, size);
					sprintf(fontname, "%s %s",font,size);
					for (i = 0 ; i < max_display ; i++) {
						xsettings_manager_set_string (managers[i], "Gtk/FontName", fontname);
					}
					notify = 1;
				}
				if (strncmp(buffer,"widgetStyle=",12) == 0) {

					/* found style */
					sscanf(buffer,"widgetStyle=%s", style);

				}
				if (strncmp(buffer,"ColorScheme=",12) == 0) {
					strcpy(color, buffer+12);
					color[strlen(color)-1]='\0';
				}
				notify |= readDPI(buffer);

				break;
			case Toolbar:
				if (strncmp (buffer, "IconText=", 9) == 0) {
					char toolbar[1024];
					int found;

					/* found toolbar */
					found = sscanf(buffer,"IconText=%s", toolbar);
					if (found && strcmp (toolbar, "IconOnly") == 0) {
						for (i = 0 ; i < max_display ; i++) {
							xsettings_manager_set_string (managers[i], "Gtk/ToolbarStyle", "icons");
						}
						notify = 1;
						found = 0;
					}
					if (found && strcmp (toolbar, "TextOnly") == 0) {
						for (i = 0 ; i < max_display ; i++) {
							xsettings_manager_set_string (managers[i], "Gtk/ToolbarStyle", "text");
						}
						notify = 1;
						found = 0;
					}
					if (found && strcmp (toolbar, "IconTextRight") == 0) {
						for (i = 0 ; i < max_display ; i++) {
							xsettings_manager_set_string (managers[i], "Gtk/ToolbarStyle", "both-horiz");
						}
						notify = 1;
						found = 0;
					}
					if (found && strcmp (toolbar, "IconTextBottom") == 0) {
						for (i = 0 ; i < max_display ; i++) {
							xsettings_manager_set_string (managers[i], "Gtk/ToolbarStyle", "both");
						}
						notify = 1;
						found = 0;
					}
				}
				break;
			case Mouse:
				notify |= readString ("cursorTheme", buffer, "Gtk/CursorThemeName");
				break;
			default: /* ignore the rest */
				break;
			}
	}
	if (file) {
		fclose (file);
		file = NULL;
	}

	} while (user_file == 0);
      }

	if (buffer) {
		free (buffer);
	}

	if ((style[0] == '\0' || (!is_kde4 && strcmp(style, "ia_ora") == 0) || (is_kde4 && strcmp(style, "iaora-qt") == 0) || (is_kde4 && strcmp(style, "iaorakde") == 0)) && strncmp("Ia Ora ", color, 7) == 0)
		       	{
		strcpy(style, color);
		if (!is_kde4) {
			style[strlen(style)-6] = '\0';
		}
	}
				
	if (style[0]) {
		int gtkrc_access = 1;
		sprintf(themefilename, "/usr/share/themes/%s/gtk-2.0/gtkrc", style);

		if (password) {
			char gtkrc[1024];
			sprintf(gtkrc,"%s/.gtkrc-2.0", password->pw_dir);
			gtkrc_access = access (gtkrc, F_OK);
			/* if not .gtkrc-2.0, try KDE variant */
			if (gtkrc_access) {
				char *gtkrc_env;
				char *gtkrc_file;
				
				gtkrc_env = getenv ("GTK2_RC_FILES");
				gtkrc_file = NULL;

				if (gtkrc_env) {
					do {
						if (gtkrc_file == NULL) {
							gtkrc_file = strtok (gtkrc_env, ":");
						}
						else {
							gtkrc_file = strtok (NULL, ":");
						}
						if (gtkrc_file != NULL) {
							if ((gtkrc_access = access (gtkrc_file, F_OK)) == 0) {
								break;
							}
						}
					} while (gtkrc_file != NULL );
				}
			}
		}

		/* do not set theme name if .gtkrc is being used */ 
		if (gtkrc_access && (access (themefilename, F_OK) == 0 )) {
			for (i = 0 ; i < max_display ; i++) {
				xsettings_manager_set_string (managers[i], "Net/ThemeName", style);
			}
		notify = 1;
		}
	}


	if (!buttonIconSet) {
		for (i = 0 ; i < max_display ; i++) {
			xsettings_manager_set_int(managers[i], "Gtk/ButtonImages", is_kde4);
		}
		notify = 1;
	}

	if (notify) {
		for (i = 0 ; i < max_display ; i++) {
			xsettings_manager_notify (managers[i]);
		}
	}
}

void initial_init () {
	int i;
	const char *kde_version;

  GSettings *settings = NULL;
  gchar *vgtkimm = NULL;

    if (check_gsettings_schema (IMMODULE_SCHEMA)) {
      settings = g_settings_new (IMMODULE_SCHEMA);
      g_signal_connect (settings, "changed",
                G_CALLBACK (gsettings_changed), NULL);

        vgtkimm = g_settings_get_string (settings,
                         IMMODULE_KEY);
  }

	kde_version = getenv("KDE_SESSION_VERSION");

	if (kde_version && (kde_version[0] == '4'))
		is_kde4 = 1;
	else
		is_kde4 = 0;

	for (i = 0 ; i < max_display ; i++) {                         
		xsettings_manager_set_string(managers[i], "Gtk/FileChooserBackend", "gio");
		xsettings_manager_set_string(managers[i], "Net/FallbackIconTheme", "gnome");
       /* KDE always shows menu images, so make sure GTK+ does, too */
        xsettings_manager_set_int(managers[i], "Gtk/MenuImages", 1);
      if (vgtkimm)
            xsettings_manager_set_string (managers[i], "Gtk/IMModule", vgtkimm);
		xsettings_manager_notify (managers[i]);
	}
}

struct xevent_data {
    Atom KIPCAtom;
    Atom KDEDesktopAtom;
    Display *display;
};

static void
xevent_handle (gpointer user_data)
{
    XEvent xevent;
    struct xevent_data* xev = (struct xevent_data*)user_data;
    int i;

    while (1) {
  if (!XPending(xev->display)) {
        sleep(2);
        continue;
    }
        XNextEvent (xev->display, &xevent);

	for (i = 0 ; i < max_display ; i++) {
	    if (xsettings_manager_process_event (managers[i], &xevent))
		    break;
	}

	if ((xevent.type == ClientMessage) && (xevent.xclient.message_type == xev->KIPCAtom)) {
	        int id = ((XClientMessageEvent *) &xevent)->data.l[0];

		switch (id) {
			case 0: /* palette changed */
			case 1: /* font changed */
			case 2: /* style changed */
			case 4: /* settings changed */
			case 5: /* icons style changed */
			case 6: /* toolbar style changed */
				readConfig();
				break;
			default:
				break;
		}
	}
    }
}


int main (int argc, char **argv) {
  long data = 1;
  Atom atoms_return[2];
  int i;
  int terminated = 0;
  struct xevent_data xev;
  GMainLoop *loop;

  g_thread_init(NULL);
  g_type_init();

  xev.display = XOpenDisplay (NULL);
  if (xev.display == NULL) {
	  fprintf(stderr, "unable to open display\n");
	  return 1;
  }
  
  if (xsettings_manager_check_running (xev.display, DefaultScreen (xev.display))) {
	  fprintf(stderr, "xsettings server already running\n");
	  XCloseDisplay (xev.display);
	  return 2;
  }

  max_display = ScreenCount (xev.display);

  managers = malloc (max_display * sizeof (XSettingsManager *));

  for (i = 0 ; i < max_display ; i++ ) {
	managers[i] = xsettings_manager_new (xev.display, i,
		  	           terminate_cb, &terminated);
	if (!managers[i]) {
		  fprintf (stderr, "Could not create manager for screen %d", i);
		  return 3;
	}
  }

  initial_init ();

  atoms[0] = &xev.KIPCAtom;
  atoms[1] = &xev.KDEDesktopAtom;

  XInternAtoms(xev.display, atom_names, 2, 0, atoms_return );

  *atoms[0] = atoms_return [0];
  *atoms[1] = atoms_return [1];

  XChangeProperty(xev.display, xsettings_manager_get_window (managers[0]),
	    xev.KDEDesktopAtom, xev.KDEDesktopAtom,
	    32, PropModeReplace, (unsigned char *)&data, 1);

  XChangeProperty(xev.display, xsettings_manager_get_window (managers[0]),
	    xev.KIPCAtom, xev.KIPCAtom,
	    32, PropModeReplace, (unsigned char *)&data, 1);

  readConfig();

  g_thread_create ((GThreadFunc)xevent_handle, &xev, FALSE, NULL);
  loop = g_main_loop_new (NULL, TRUE);
  g_main_loop_run (loop);

  for (i = 0; i < max_display; i++) {
		xsettings_manager_destroy (managers [i]);
  }
  free (managers);

  XCloseDisplay (xev.display);

  return 0;
}
