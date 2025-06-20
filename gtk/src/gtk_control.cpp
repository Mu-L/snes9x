/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include <fcntl.h>

#include "SDL_joystick.h"
#include "fscompat.h"
#include "gtk_s9x.h"
#include "gtk_config.h"
#include "gtk_control.h"
#include "gtk_file.h"

#include "snes9x.h"
#include "controls.h"
#include "display.h"
#include "gfx.h"

const BindingLink b_links[] =
{
        /* Joypad-specific bindings. "Joypad# " will be prepended */
        { "b_up",        "Up"          },
        { "b_down",      "Down"        },
        { "b_left",      "Left"        },
        { "b_right",     "Right"       },
        { "b_start",     "Start"       },
        { "b_select",    "Select"      },
        { "b_a",         "A"           },
        { "b_b",         "B"           },
        { "b_x",         "X"           },
        { "b_y",         "Y"           },
        { "b_l",         "L"           },
        { "b_r",         "R"           },
        { "b_a_turbo",   "Turbo A"     },
        { "b_b_turbo",   "Turbo B"     },
        { "b_x_turbo",   "Turbo X"     },
        { "b_y_turbo",   "Turbo Y"     },
        { "b_l_turbo",   "Turbo L"     },
        { "b_r_turbo",   "Turbo R"     },
        { "b_a_sticky",  "Sticky A"    },
        { "b_b_sticky",  "Sticky B"    },
        { "b_x_sticky",  "Sticky X"    },
        { "b_y_sticky",  "Sticky Y"    },
        { "b_l_sticky",  "Sticky L"    },
        { "b_r_sticky",  "Sticky R"    },

        /* Emulator based bindings */
        { "b_open_rom",            "GTK_open_rom"      },
        { "b_enable_turbo",        "EmuTurbo"          },
        { "b_toggle_turbo",        "ToggleEmuTurbo"    },
        { "b_pause",               "GTK_pause"         },
        { "b_decrease_frame_rate", "DecFrameRate"      },
        { "b_increase_frame_rate", "IncFrameRate"      },
        { "b_decrease_frame_time", "DecFrameTime"      },
        { "b_increase_frame_time", "IncFrameTime"      },
        { "b_hardware_reset",      "Reset"             },
        { "b_soft_reset",          "SoftReset"         },
        { "b_quit",                "GTK_quit"          },
        { "b_bg_layer_0",          "ToggleBG0"         },
        { "b_bg_layer_1",          "ToggleBG1"         },
        { "b_bg_layer_2",          "ToggleBG2"         },
        { "b_bg_layer_3",          "ToggleBG3"         },
        { "b_sprites",             "ToggleSprites"     },
        { "toggle_backdrop",       "ToggleBackdrop"    },
        { "b_screenshot",          "Screenshot"        },
        { "b_fullscreen",          "GTK_fullscreen"    },
        { "b_state_save_current",  "GTK_state_save_current" },
        { "b_state_load_current",  "GTK_state_load_current" },
        { "b_state_increment_save","GTK_state_increment_save" },
        { "b_state_decrement_load","GTK_state_decrement_load" },
        { "b_state_increment",     "GTK_state_increment" },
        { "b_state_decrement",     "GTK_state_decrement" },
        { "b_save_0",              "QuickSave000"      },
        { "b_save_1",              "QuickSave001"      },
        { "b_save_2",              "QuickSave002"      },
        { "b_save_3",              "QuickSave003"      },
        { "b_save_4",              "QuickSave004"      },
        { "b_save_5",              "QuickSave005"      },
        { "b_save_6",              "QuickSave006"      },
        { "b_save_7",              "QuickSave007"      },
        { "b_save_8",              "QuickSave008"      },
        { "b_save_9",              "QuickSave009"      },
        { "b_load_0",              "QuickLoad000"      },
        { "b_load_1",              "QuickLoad001"      },
        { "b_load_2",              "QuickLoad002"      },
        { "b_load_3",              "QuickLoad003"      },
        { "b_load_4",              "QuickLoad004"      },
        { "b_load_5",              "QuickLoad005"      },
        { "b_load_6",              "QuickLoad006"      },
        { "b_load_7",              "QuickLoad007"      },
        { "b_load_8",              "QuickLoad008"      },
        { "b_load_9",              "QuickLoad009"      },
        { "b_sound_channel_0",     "SoundChannel0"     },
        { "b_sound_channel_1",     "SoundChannel1"     },
        { "b_sound_channel_2",     "SoundChannel2"     },
        { "b_sound_channel_3",     "SoundChannel3"     },
        { "b_sound_channel_4",     "SoundChannel4"     },
        { "b_sound_channel_5",     "SoundChannel5"     },
        { "b_sound_channel_6",     "SoundChannel6"     },
        { "b_sound_channel_7",     "SoundChannel7"     },
        { "b_all_sound_channels",  "SoundChannelsOn"   },
        { "b_save_spc",            "GTK_save_spc"      },
        { "b_begin_recording_movie", "BeginRecordingMovie" },
        { "b_stop_recording_movie", "EndRecordingMovie" },
        { "b_load_movie",          "LoadMovie" },
        { "b_seek_to_frame",       "GTK_seek_to_frame" },
        { "b_swap_controllers",    "GTK_swap_controllers" },
        { "b_rewind",              "GTK_rewind"        },
        { "b_grab_mouse",          "GTK_grab_mouse"    },

        { nullptr, nullptr }
};

/* Where the page breaks occur in the preferences pane */
const int b_breaks[] =
{
        12, /* End of main buttons */
        24, /* End of turbo/sticky buttons */
        35, /* End of base emulator buttons */
        43, /* End of Graphic options */
        69, /* End of save/load states */
        78, /* End of sound buttons */
        86, /* End of miscellaneous buttons */
        -1
};

static int joystick_lock = 0;

bool S9xPollButton(uint32 id, bool *pressed)
{
    return true;
}

bool S9xPollAxis(uint32 id, int16 *value)
{
    return true;
}

static bool using_superscope()
{
    for (int i = 0; i < 2; i++)
    {
        enum controllers ctl;
        int8_t id1, id2, id3, id4;
        S9xGetController(i, &ctl, &id1, &id2, &id3, &id4);
        if (ctl == CTL_SUPERSCOPE)
            return true;
    }

    return false;
}

bool S9xPollPointer(uint32 id, int16 *x, int16 *y)
{
    if (using_superscope())
    {
        top_level->snes_mouse_x = std::clamp(top_level->snes_mouse_x, 0.0, 256.0);
        top_level->snes_mouse_y = std::clamp(top_level->snes_mouse_y, 0.0, 239.0);
    }

    *x = top_level->snes_mouse_x;
    *y = top_level->snes_mouse_y;

    return true;
}

bool S9xIsMousePluggedIn()
{
    enum controllers ctl;
    int8 id1, id2, id3, id4;

    for (int i = 0; i <= 1; i++)
    {
        S9xGetController(i, &ctl, &id1, &id2, &id3, &id4);
        if (ctl == CTL_MOUSE || ctl == CTL_SUPERSCOPE)
            return true;
    }

    return false;
}

bool S9xGrabJoysticks()
{
    if (joystick_lock)
        return false;

    joystick_lock++;

    return true;
}

void S9xReleaseJoysticks()
{
    joystick_lock--;
}

static void swap_controllers_1_2()
{
    JoypadBinding interrim = gui_config->pad[0];
    gui_config->pad[0] = gui_config->pad[1];
    gui_config->pad[1] = interrim;

    gui_config->rebind_keys();
}

static void change_slot(int difference)
{
    gui_config->current_save_slot += difference;
    gui_config->current_save_slot %= 1000;
    if (gui_config->current_save_slot < 0)
        gui_config->current_save_slot += 1000;
    if (!gui_config->rom_loaded)
        return;

    char extension_string[5];
    snprintf(extension_string, 5, ".%03d", gui_config->current_save_slot);
    auto filename = S9xGetFilename(extension_string, SNAPSHOT_DIR);
    struct stat info{};
    std::string exists = "empty";
    if (stat(filename.c_str(), &info) == 0)
        exists = "used";

    auto info_string = "State Slot: " + std::to_string(gui_config->current_save_slot) + " [" + exists + "]";
    S9xSetInfoString(info_string.c_str());
    GFX.InfoStringTimeout = 60;
}

void S9xHandlePortCommand(s9xcommand_t cmd, int16 data1, int16 data2)
{
    static bool quit_binding_down = false;

    if (data1 == true)
    {
        if (cmd.port[0] == PORT_QUIT)
            quit_binding_down = true;
        else if (cmd.port[0] == PORT_REWIND)
            Settings.Rewinding = true;
    }

    if (data1 == false) /* Release */
    {
        if (cmd.port[0] != PORT_QUIT)
        {
            quit_binding_down = false;
        }
        if (cmd.port[0] == PORT_COMMAND_FULLSCREEN)
        {
            top_level->toggle_fullscreen_mode();
        }
        else if (cmd.port[0] == PORT_COMMAND_SAVE_SPC)
        {
            top_level->save_spc_dialog();
        }
        else if (cmd.port[0] == PORT_OPEN_ROM)
        {
            top_level->open_rom_dialog();
        }
        else if (cmd.port[0] == PORT_PAUSE)
        {
            if (!(top_level->user_pause))
                top_level->pause_from_user();
            else
                top_level->unpause_from_user();
        }
        else if (cmd.port[0] == PORT_REWIND)
        {
            Settings.Rewinding = false;
        }
        else if (cmd.port[0] == PORT_SEEK_TO_FRAME)
        {
            top_level->movie_seek_dialog();
        }
        else if (cmd.port[0] == PORT_SWAP_CONTROLLERS)
        {
            swap_controllers_1_2();
        }
        else if (cmd.port[0] == PORT_QUIT)
        {
            if (quit_binding_down)
                S9xExit();
        }
        else if (cmd.port[0] >= PORT_QUICKLOAD0 && cmd.port[0] <= PORT_QUICKLOAD9)
        {
            S9xQuickLoadSlot(cmd.port[0] - PORT_QUICKLOAD0);
        }
        else if (cmd.port[0] == PORT_SAVESLOT)
        {
            S9xQuickSaveSlot(gui_config->current_save_slot);
        }
        else if (cmd.port[0] == PORT_LOADSLOT)
        {
            S9xQuickLoadSlot(gui_config->current_save_slot);
        }
        else if (cmd.port[0] == PORT_INCREMENTSAVESLOT)
        {
            change_slot(1);
            S9xQuickSaveSlot(gui_config->current_save_slot);
        }
        else if (cmd.port[0] == PORT_DECREMENTLOADSLOT)
        {
            change_slot(-1);
            S9xQuickLoadSlot(gui_config->current_save_slot);
        }
        else if (cmd.port[0] == PORT_INCREMENTSLOT)
        {
            change_slot(1);
        }
        else if (cmd.port[0] == PORT_DECREMENTSLOT)
        {
            change_slot(-1);
        }
        else if (cmd.port[0] == PORT_GRABMOUSE)
        {
            top_level->toggle_grab_mouse();
        }
    }
}

Binding S9xGetBindingByName(const char *name)
{
    for (int i = 0; i < NUM_EMU_LINKS; i++)
    {
        if (!strcasecmp(b_links[i + NUM_JOYPAD_LINKS].snes9x_name, name))
        {
            return gui_config->shortcut[i];
        }
    }

    return {};
}

s9xcommand_t S9xGetPortCommandT(const char *name)
{
    s9xcommand_t cmd;

    cmd.type = S9xButtonPort;
    cmd.multi_press = 0;
    cmd.button_norpt = 0;
    cmd.port[0] = 0;
    cmd.port[1] = 0;
    cmd.port[2] = 0;
    cmd.port[3] = 0;

    if (!strcasecmp(name, "GTK_fullscreen"))
    {
        cmd.port[0] = PORT_COMMAND_FULLSCREEN;
    }
    else if (!strcasecmp(name, "GTK_save_spc"))
    {
        cmd.port[0] = PORT_COMMAND_SAVE_SPC;
    }
    else if (!strcasecmp(name, "GTK_open_rom"))
    {
        cmd.port[0] = PORT_OPEN_ROM;
    }
    else if (!strcasecmp(name, "GTK_pause"))
    {
        cmd.port[0] = PORT_PAUSE;
    }
    else if (!strcasecmp(name, "GTK_seek_to_frame"))
    {
        cmd.port[0] = PORT_SEEK_TO_FRAME;
    }
    else if (!strcasecmp(name, "GTK_quit"))
    {
        cmd.port[0] = PORT_QUIT;
    }
    else if (!strcasecmp(name, "GTK_swap_controllers"))
    {
        cmd.port[0] = PORT_SWAP_CONTROLLERS;
    }
    else if (!strcasecmp(name, "GTK_rewind"))
    {
        cmd.port[0] = PORT_REWIND;
    }
    else if (strstr(name, "QuickLoad000"))
    {
        cmd.port[0] = PORT_QUICKLOAD0;
    }
    else if (strstr(name, "QuickLoad001"))
    {
        cmd.port[0] = PORT_QUICKLOAD1;
    }
    else if (strstr(name, "QuickLoad002"))
    {
        cmd.port[0] = PORT_QUICKLOAD2;
    }
    else if (strstr(name, "QuickLoad003"))
    {
        cmd.port[0] = PORT_QUICKLOAD3;
    }
    else if (strstr(name, "QuickLoad004"))
    {
        cmd.port[0] = PORT_QUICKLOAD4;
    }
    else if (strstr(name, "QuickLoad005"))
    {
        cmd.port[0] = PORT_QUICKLOAD5;
    }
    else if (strstr(name, "QuickLoad006"))
    {
        cmd.port[0] = PORT_QUICKLOAD6;
    }
    else if (strstr(name, "QuickLoad007"))
    {
        cmd.port[0] = PORT_QUICKLOAD7;
    }
    else if (strstr(name, "QuickLoad008"))
    {
        cmd.port[0] = PORT_QUICKLOAD8;
    }
    else if (strstr(name, "QuickLoad009"))
    {
        cmd.port[0] = PORT_QUICKLOAD9;
    }
    else if (strstr(name, "GTK_state_save_current"))
    {
        cmd.port[0] = PORT_SAVESLOT;
    }
    else if (strstr(name, "GTK_state_load_current"))
    {
        cmd.port[0] = PORT_LOADSLOT;
    }
    else if (strstr(name, "GTK_state_increment_save"))
    {
        cmd.port[0] = PORT_INCREMENTSAVESLOT;
    }
    else if (strstr(name, "GTK_state_decrement_load"))
    {
        cmd.port[0] = PORT_DECREMENTLOADSLOT;
    }
    else if (strstr(name, "GTK_state_increment"))
    {
        cmd.port[0] = PORT_INCREMENTSLOT;
    }
    else if (strstr(name, "GTK_state_decrement"))
    {
        cmd.port[0] = PORT_DECREMENTSLOT;
    }
    else if (strstr(name, "GTK_grab_mouse"))
    {
        cmd.port[0] = PORT_GRABMOUSE;
    }
    else
    {
        cmd = S9xGetCommandT(name);
    }

    return cmd;
}


void S9xProcessEvents(bool8 block)
{
    if (S9xGrabJoysticks())
    {
        gui_config->joysticks.poll_events();
        for (auto &j : gui_config->joysticks)
        {
            JoyEvent event;
            while (j.second->get_event(&event))
            {
                Binding binding(j.second->joynum, event.parameter, 0);
                S9xReportButton(binding.hex(), event.state == JOY_PRESSED);
                gui_config->screensaver_needs_reset = true;
            }
        }

        S9xReleaseJoysticks();
    }
}


void S9xInitInputDevices()
{
    SDL_Init(SDL_INIT_JOYSTICK);
    size_t num_joysticks = SDL_NumJoysticks();

    for (size_t i = 0; i < num_joysticks; i++)
    {
        gui_config->joysticks.add(i);
    }

    //First plug in both, they'll change later as needed
    S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
    S9xSetController(1, CTL_JOYPAD, 1, 0, 0, 0);
}

void S9xDeinitInputDevices()
{
    gui_config->joysticks.clear();
    SDL_Quit();
}

JoyDevice::JoyDevice()
{
    enabled = false;
    filedes = nullptr;
    mode = JOY_MODE_INDIVIDUAL;
}

JoyDevice::~JoyDevice()
{
    if (filedes)
    {
        SDL_JoystickClose(filedes);
    }
}

bool JoyDevice::set_sdl_joystick(unsigned int sdl_device_index, int new_joynum)
{
    if ((int)sdl_device_index >= SDL_NumJoysticks())
    {
        enabled = false;
        return false;
    }

    filedes = SDL_JoystickOpen(sdl_device_index);

    if (!filedes)
        return false;

    enabled = true;
    instance_id = SDL_JoystickInstanceID(filedes);
    joynum = new_joynum;

    int num_axes = SDL_JoystickNumAxes(filedes);
    int num_hats = SDL_JoystickNumHats(filedes);
    axis.resize(num_axes);
    hat.resize(num_hats);
    calibration.resize(num_axes);

    for (int i = 0; i < num_axes; i++)
    {
        calibration[i].min = -32767;
        calibration[i].max = 32767;
        calibration[i].center = 0;
    }

    description = SDL_JoystickName(filedes);
    description += ": ";
    description += std::to_string(SDL_JoystickNumButtons(filedes));
    description += " buttons, ";
    description += std::to_string(num_axes);
    description += " axes, ";
    description += std::to_string(num_hats);
    description += " hats\n";

    for (auto &i : axis)
        i = 0;

    return true;
}

void JoyDevice::add_event(unsigned int parameter, unsigned int state)
{
    JoyEvent event = { parameter, state };

    queue.push(event);
}

void JoyDevice::register_centers()
{
    for (size_t i = 0; i < axis.size(); i++)
    {
        calibration[i].center = SDL_JoystickGetAxis(filedes, i);

        /* Snap centers to specific target points */
        if (calibration[i].center < -24576)
            calibration[i].center = -32768;
        else if (calibration[i].center < -8192)
            calibration[i].center = -16384;
        else if (calibration[i].center < 8192)
            calibration[i].center = 0;
        else if (calibration[i].center < 24576)
            calibration[i].center = 16383;
        else
            calibration[i].center = 32767;
    }
}

void JoyDevice::handle_event(SDL_Event *event)
{
    if (event->type == SDL_JOYAXISMOTION)
    {
        int cal_min = calibration[event->jaxis.axis].min;
        int cal_max = calibration[event->jaxis.axis].max;
        int cal_cen = calibration[event->jaxis.axis].center;
        int t = gui_config->joystick_threshold;
        int ax_min = (cal_min - cal_cen) * t / 100 + cal_cen;
        int ax_max = (cal_max - cal_cen) * t / 100 + cal_cen;

        if (mode == JOY_MODE_INDIVIDUAL)
        {
            for (int i = 0; i < NUM_JOYPADS; i++)
            {
                Binding *pad = (Binding *)&(gui_config->pad[i]);

                for (int j = 0; j < NUM_JOYPAD_LINKS; j++)
                {
                    if (pad[j].get_axis() == event->jaxis.axis &&
                        pad[j].get_device() == (unsigned int)(joynum + 1))
                    {
                        t = pad[j].get_threshold();

                        if (pad[j].is_positive())
                        {
                            ax_max = (cal_max - cal_cen) * t / 100 + cal_cen;
                        }
                        else if (pad[j].is_negative())
                        {
                            ax_min = (cal_min - cal_cen) * t / 100 + cal_cen;
                        }
                    }
                }
            }

            for (int i = 0; i < NUM_EMU_LINKS; i++)
            {
                if (gui_config->shortcut[i].get_axis() == event->jaxis.axis &&
                    gui_config->shortcut[i].get_device() ==
                        (unsigned int)(joynum + 1))
                {
                    t = gui_config->shortcut[i].get_threshold();
                    if (gui_config->shortcut[i].is_positive())
                    {
                        ax_max = (cal_max - cal_cen) * t / 100 + cal_cen;
                    }
                    else if (gui_config->shortcut[i].is_negative())
                    {
                        ax_min = (cal_min - cal_cen) * t / 100 + cal_cen;
                    }
                }
            }
        }
        else if (mode == JOY_MODE_CALIBRATE)
        {
            if (event->jaxis.value < calibration[event->jaxis.axis].min)
                calibration[event->jaxis.axis].min = event->jaxis.value;
            if (event->jaxis.value > calibration[event->jaxis.axis].max)
                calibration[event->jaxis.axis].min = event->jaxis.value;
        }

        /* Sanity Check */
        if (ax_min >= cal_cen)
            ax_min = cal_cen - 1;
        if (ax_max <= cal_cen)
            ax_max = cal_cen + 1;

        if (event->jaxis.value <= ax_min &&
            axis[event->jaxis.axis] > ax_min)
        {
            add_event(JOY_AXIS(event->jaxis.axis, AXIS_NEG), 1);
        }

        if (event->jaxis.value > ax_min &&
            axis[event->jaxis.axis] <= ax_min)
        {
            add_event(JOY_AXIS(event->jaxis.axis, AXIS_NEG), 0);
        }

        if (event->jaxis.value >= ax_max &&
            axis[event->jaxis.axis] < ax_max)
        {
            add_event(JOY_AXIS(event->jaxis.axis, AXIS_POS), 1);
        }

        if (event->jaxis.value < ax_max &&
            axis[event->jaxis.axis] >= ax_max)
        {
            add_event(JOY_AXIS(event->jaxis.axis, AXIS_POS), 0);
        }

        axis[event->jaxis.axis] = event->jaxis.value;
    }

    else if (event->type == SDL_JOYBUTTONUP ||
             event->type == SDL_JOYBUTTONDOWN)
    {
        add_event(event->jbutton.button,
                  event->jbutton.state == SDL_PRESSED ? 1 : 0);
    }

    else if (event->type == SDL_JOYHATMOTION)
    {
        if ((event->jhat.value & SDL_HAT_UP) &&
            !(hat[event->jhat.hat] & SDL_HAT_UP))
        {
            add_event(JOY_AXIS(axis.size() + (event->jhat.hat * 2), AXIS_POS), 1);
        }

        if (!(event->jhat.value & SDL_HAT_UP) &&
            (hat[event->jhat.hat] & SDL_HAT_UP))
        {
            add_event(JOY_AXIS(axis.size() + (event->jhat.hat * 2), AXIS_POS), 0);
        }

        if ((event->jhat.value & SDL_HAT_DOWN) &&
            !(hat[event->jhat.hat] & SDL_HAT_DOWN))
        {
            add_event(JOY_AXIS(axis.size() + (event->jhat.hat * 2), AXIS_NEG), 1);
        }

        if (!(event->jhat.value & SDL_HAT_DOWN) &&
            (hat[event->jhat.hat] & SDL_HAT_DOWN))
        {
            add_event(JOY_AXIS(axis.size() + (event->jhat.hat * 2), AXIS_NEG), 0);
        }

        if ((event->jhat.value & SDL_HAT_LEFT) &&
            !(hat[event->jhat.hat] & SDL_HAT_LEFT))
        {
            add_event(JOY_AXIS(axis.size() + (event->jhat.hat * 2) + 1, AXIS_NEG), 1);
        }

        if (!(event->jhat.value & SDL_HAT_LEFT) &&
            (hat[event->jhat.hat] & SDL_HAT_LEFT))
        {
            add_event(JOY_AXIS(axis.size() + (event->jhat.hat * 2) + 1, AXIS_NEG), 0);
        }

        if ((event->jhat.value & SDL_HAT_RIGHT) &&
            !(hat[event->jhat.hat] & SDL_HAT_RIGHT))
        {
            add_event(JOY_AXIS(axis.size() + (event->jhat.hat * 2) + 1, AXIS_POS), 1);
        }

        if (!(event->jhat.value & SDL_HAT_RIGHT) &&
            (hat[event->jhat.hat] & SDL_HAT_RIGHT))
        {
            add_event(JOY_AXIS(axis.size() + (event->jhat.hat * 2) + 1, AXIS_POS), 0);
        }

        hat[event->jhat.hat] = event->jhat.value;
    }
}

int JoyDevice::get_event(JoyEvent *event)
{
    if (queue.empty())
        return 0;

    event->parameter = queue.front().parameter;
    event->state = queue.front().state;

    queue.pop();

    return 1;
}

void JoyDevice::flush()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
    }

    while (!queue.empty())
        queue.pop();
}

void JoyDevices::clear()
{
    joysticks.clear();
}

bool JoyDevices::add(int sdl_device_index)
{
    std::array<bool, NUM_JOYPADS> joynums{};
    joynums.fill(false);
    for (auto &j : joysticks)
    {
        joynums[j.second->joynum] = true;
    }

    // New joystick always gets the lowest available joynum
    int joynum = 0;
    for (; joynum < NUM_JOYPADS && joynums[joynum]; ++joynum) {};

    if (joynum == NUM_JOYPADS)
    {
        printf("Joystick slots are full, cannot add joystick (device index %d)\n", sdl_device_index);
        return false;
    }

    auto ujd = std::make_unique<JoyDevice>();
    ujd->set_sdl_joystick(sdl_device_index, joynum);
    printf("Joystick %d, %s", ujd->joynum+1, ujd->description.c_str());
    joysticks[ujd->instance_id] = std::move(ujd);
    return true;
}

bool JoyDevices::remove(SDL_JoystickID instance_id)
{
    if (!joysticks.contains(instance_id))
    {
        printf("joystick_remove: invalid instance id %d", instance_id);
        return false;
    }
    printf("Removed joystick %d, %s", joysticks[instance_id]->joynum+1, joysticks[instance_id]->description.c_str());
    joysticks.erase(instance_id);
    return true;
}

JoyDevice *JoyDevices::get_joystick(SDL_JoystickID instance_id)
{
    if (joysticks.contains(instance_id)){
        return joysticks[instance_id].get();
    }
    printf("BUG: Event for unknown joystick instance id: %d", instance_id);
    return nullptr;
}

void JoyDevices::register_centers()
{
    for (auto &j : joysticks)
        j.second->register_centers();
}

void JoyDevices::flush_events()
{
    for (auto &j : joysticks)
        j.second->flush();
}

void JoyDevices::set_mode(int mode)
{
    for (auto &j : joysticks)
        j.second->mode = mode;
}

void JoyDevices::poll_events()
{
    SDL_Event event;
    JoyDevice *jd{};

    while (SDL_PollEvent(&event))
    {
        switch(event.type) {
            case SDL_JOYAXISMOTION:
                jd = get_joystick(event.jaxis.which);
                break;
            case SDL_JOYHATMOTION:
                jd = get_joystick(event.jhat.which);
                break;
            case SDL_JOYBUTTONUP:
            case SDL_JOYBUTTONDOWN:
                jd = get_joystick(event.jbutton.which);
                break;
            case SDL_JOYDEVICEADDED:
                add(event.jdevice.which);
                continue;
            case SDL_JOYDEVICEREMOVED:
                remove(event.jdevice.which);
                continue;
        }
        
        if (jd)
        {
            jd->handle_event(&event);
        }
    }
}

